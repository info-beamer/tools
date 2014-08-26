# This program is licensed under the BSD 2-Clause License:
# 
# Copyright (c) 2013, Florian Wesch <fw@dividuum.de>
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

import socket

class InfoBeamerQueryException(Exception):
    pass
 
class InfoBeamerQuery(object):
    def __init__(self, host, port=4444):
        self.sock = None
        self.host = host
        self.port = port
        self.timeout = 2

    def reconnect(self):
        if self.sock is not None:
            return
        try:
            sock = socket.create_connection((self.host, self.port), self.timeout)
            self.sock = sock.makefile()
            intro = self.sock.readline()
        except socket.timeout:
            raise InfoBeamerQueryException("timeout while reopening connection")
        except socket.error, err:
            raise InfoBeamerQueryException("cannot connect to %s:%s: %s" % (
                self.host, self.port, err))
        if not intro.startswith("Info Beamer PI"):
            raise InfoBeamerQueryException("invalid remote handshake header")

    def do_query(self, cmd):
        for retry in (1, 2):
            self.reconnect()
            try:
                self.sock.write("*query/" + cmd + "\n")
                self.sock.flush()
                response = self.sock.readline().strip()
                if not response:
                    self.close()
                    continue
                return response
            except socket.error:
                self.close()
                continue
            except socket.timeout:
                self.close()
                raise InfoBeamerQueryException("timeout waiting for response")
            except Exception:
                self.close()
                continue
        raise InfoBeamerQueryException("failed to get a response")

    def close(self):
        self.sock.close()
        self.sock = None

    @property 
    def version(self):
        return self.do_query("*version")

    @property 
    def fps(self):
        return float(self.do_query("*fps"))

    @property 
    def uptime(self):
        return int(self.do_query("*uptime"))

    @property 
    def resources(self):
        return map(int, self.do_query("*resources").split(','))

    @property
    def addr(self):
        return "%s:%s" % (self.host, self.port)
 
if __name__ == "__main__":
    ib = InfoBeamerQuery("127.0.0.1")
    print "%s is running %s. current fps: %d, uptime: %dsec" % (ib.addr, ib.version, ib.fps, ib.uptime)
