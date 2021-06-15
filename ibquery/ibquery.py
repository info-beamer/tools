# This program is licensed under the BSD 2-Clause License:
#
# Copyright (c) 2015-2021, Florian Wesch <fw@dividuum.de>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#
#     Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the
#     distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import re
import json
import socket
from collections import namedtuple

class InfoBeamerQueryException(Exception):
    pass

class InfoBeamerQuery(object):
    def __init__(self, host='127.0.0.1', port=4444):
        self._sock = None
        self._conn = None
        self._host = host
        self._port = port
        self._timeout = 2
        self._version = None

    def _reconnect(self):
        if self._conn is not None:
            return
        try:
            self._sock = socket.create_connection((self._host, self._port), self._timeout)
            self._conn = self._sock.makefile(mode='rwb')
            intro = self._conn.readline().decode('utf8')
        except socket.timeout:
            self._reset()
            raise InfoBeamerQueryException("Timeout while reopening connection")
        except socket.error as err:
            self._reset()
            raise InfoBeamerQueryException("Cannot connect to %s:%s: %s" % (
                self._host, self._port, err))
        m = re.match("^Info Beamer PI ([^ ]+)", intro)
        if not m:
            self._reset()
            raise InfoBeamerQueryException("Invalid handshake. Not info-beamer?")
        self._version = m.group(1)

    def _parse_line(self):
        line = self._conn.readline().decode('utf8')
        if not line:
            return None
        return line.rstrip()

    def _parse_multi_line(self):
        lines = []
        while 1:
            line = self._conn.readline().decode('utf8')
            if not line:
                return None
            line = line.rstrip()
            if not line:
                break
            lines.append(line)
        return '\n'.join(lines)

    def _send_cmd(self, min_version, cmd, multiline=False):
        for retry in (1, 2):
            self._reconnect()
            if self._version <= min_version:
                raise InfoBeamerQueryException(
                    "This query is not implemented in your version of info-beamer. "
                    "%s or higher required, %s found" % (min_version, self._version)
                )
            try:
                self._conn.write(cmd.encode('utf8') + b"\n")
                self._conn.flush()
                response = self._parse_multi_line() if multiline else self._parse_line()
                if response is None:
                    self._reset()
                    continue
                return response
            except socket.error:
                self._reset()
                continue
            except socket.timeout:
                self._reset()
                raise InfoBeamerQueryException("Timeout waiting for response")
            except Exception:
                self._reset()
                continue
        raise InfoBeamerQueryException("Failed to get a response")

    def _reset(self, close=True):
        if close:
            try:
                if self._conn: self._conn.close()
                if self._sock: self._sock.close()
            except:
                pass
        self._conn = None
        self._sock = None

    @property
    def addr(self):
        return "%s:%s" % (self._host, self._port)

    def close(self):
        self._reset()

    @property
    def ping(self):
        "tests if info-beamer is reachable"
        return self._send_cmd(
            "0.6", "*query/*ping",
        ) == "pong"

    @property
    def uptime(self):
        "returns the uptime in seconds"
        return int(self._send_cmd(
            "0.6", "*query/*uptime",
        ))

    @property
    def objects(self):
        "returns the number of allocated info-beamer objects"
        return int(self._send_cmd(
            "0.9.4", "*query/*objects",
        ))

    @property
    def version(self):
        "returns the running info-beamer version"
        return self._send_cmd(
            "0.6", "*query/*version",
        )

    @property
    def fps(self):
        "returns the FPS of the top level node"
        return float(self._send_cmd(
            "0.6", "*query/*fps",
        ))

    @property
    def display(self):
        "returns the display configuration"
        return json.loads(self._send_cmd(
            "1.0", "*query/*display",
        ))

    ResourceUsage = namedtuple("ResourceUsage", "user_time system_time memory")
    @property
    def resources(self):
        "returns information about used resources"
        return self.ResourceUsage._make(int(v) for v in self._send_cmd(
            "0.6", "*query/*resources",
        ).split(','))

    ScreenSize = namedtuple("ScreenSize", "width height")
    @property
    def screen(self):
        "returns the native screen size"
        return self.ScreenSize._make(int(v) for v in self._send_cmd(
            "0.8.1", "*query/*screen",
        ).split(','))

    @property
    def runid(self):
        "returns a unique run id that changes with every restart of info-beamer"
        return self._send_cmd(
            "0.9.0", "*query/*runid",
        )

    @property
    def nodes(self):
        "returns a list of nodes"
        nodes = self._send_cmd(
            "0.9.3", "*query/*nodes",
        ).split(',')
        return [] if not nodes[0] else nodes

    class Node(object):
        def __init__(self, ib, path):
            self._ib = ib
            self._path = path

        @property
        def mem(self):
            "returns the Lua memory usage of this node"
            return int(self._ib._send_cmd(
                "0.6", "*query/*mem/%s" % self._path
            ))

        @property
        def fps(self):
            "returns the framerate of this node"
            return float(self._ib._send_cmd(
                "0.6", "*query/*fps/%s" % self._path
            ))

        def io(self, raw=True):
            "creates a tcp connection to this node"
            status = self._ib._send_cmd(
                "0.6", "%s%s" % ("*raw/" if raw else '', self._path),
            )
            if status != 'ok!':
                raise InfoBeamerQueryException("Cannot connect to node %s" % self._path)
            sock = self._ib._sock
            sock.settimeout(None)
            return self._ib._conn

        @property
        def has_error(self):
            "queries the error flag"
            return bool(int(self._ib._send_cmd(
                "0.8.2", "*query/*has_error/%s" % self._path,
            )))

        @property
        def error(self):
            "returns the last Lua traceback"
            return self._ib._send_cmd(
                "0.8.2", "*query/*error/%s" % self._path, multiline=True
            )

        def __repr__(self):
            return "%s/%s" % (self._ib, self._path)

    def node(self, node):
        return self.Node(self, node)

    def __repr__(self):
        return "<info-beamer@%s>" % self.addr

if __name__ == "__main__":
    import sys
    ib = InfoBeamerQuery(sys.argv[1] if len(sys.argv) > 1 else "127.0.0.1")
    print("%s is running %s. current fps: %d, uptime: %dsec" % (ib.addr, ib.version, ib.fps, ib.uptime))
