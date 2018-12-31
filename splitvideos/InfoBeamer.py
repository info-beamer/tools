#!/usr/bin/python
# -*- coding: utf-8 -*-
# pylint: disable=line-too-long,star-args
# pychecker: disable=line-too-long disable=star-args

"""Python Class for interacting with the info-beamer REST API
Developed by Joseph T. Foley <foley at RU dot IS>
Based upon conversation with Florian Wesch
https://community.infobeamer.com/t/hd-player-with-large-files/241/13
Started 2018-12-30
Project home https://project.cs.ru.is/projects/raspi-art/
"""
##
## Package dependencies
##   sudo apt install python-requests
import os
import os.path
import sys
import re
import json
from operator import itemgetter
import logging
import ConfigParser #ConfigParser in py2 configparser in py3

import requests


class InfoBeamer(object):
    """Abstraction for interacting with info-beamer REST api"""
    logfd = None
    log = logging.getLogger("infobeamer")

    def __init__(self, args):
        """Find and setup configuration information
        """
        self.args = args
        # setup logger
        logpath = 'info_beamer.log'
        floglevel = logging.DEBUG
        cloglevel = logging.INFO
        self.log.setLevel(floglevel)
        self.log.addHandler(logging.FileHandler(logpath))
        console_handler = logging.StreamHandler(sys.stderr)
        console_handler.setLevel(cloglevel)
        self.log.addHandler(console_handler)
        self.log.info("Logging to %s", logpath)
        self.log.debug("File logging set at %s", floglevel)
        self.log.debug("Console logging level at %s", cloglevel)

        # setup the config file
        configpaths = [args.configfile, "./info_beamer.ini", "~/.info_beamer.ini"]
        configparser = ConfigParser.SafeConfigParser()
        configfile = None
        def checkfile(filepath):
            return os.path.isfile(filepath) and os.access(filepath, os.R_OK)
        for configpath in configpaths:
            self.log.debug("Looking for config file in %s", configpath)
            if configpath is None:
                continue
            configpath = os.path.expanduser(configpath)  #deal with ~
            if not checkfile(configpath):
                continue
            configfile = configparser.read([configpath])
            self.log.info("Using configuration file at %s", configpath)
            break

        if configfile is None:
            self.log.error("Error:  Couldn't find a configuration file.")
            sys.exit()

        def config2dict(section):
            """convert a parsed configuration into a dict for storage"""
            config = {setarg: setval for setarg, setval
                in configparser.items(section)}
            for setarg, setval in config.items():
                self.log.debug('setting: %s = %s', setarg, setval)
            return config
        config = {}
        for section in ['account', 'setups']:
            config[section] = config2dict(section)
        self.config = config
        self.configfile = configfile
        
        self.log.debug("Setting session parameters")
        session = requests.Session()
        session.auth = '', self.config['account']['api_key']
        self.session = session

        self.log.debug("Fetch all assets from your account")
        req = self.session.get('https://info-beamer.com/api/v1/asset/list')
        assets = req.json()['assets']

        # Sort all assets by filename
        assets.sort(key=itemgetter('filename'))
        self.assets = assets
    def build_playlist(self, regexp='^videos/.*mp4'):
        """Compose a playlist for a setup based upon a regexp match on the assets"""    
        # create a playlist based on the assets
        playlist = []
        total_duration = 0
        self.log.info("Searching assets for regexp \'%s\'", regexp)
        for asset in self.assets:
            # skip any asset not matching the required filename pattern
            if not re.match(regexp, asset['filename']):
                continue
            self.log.info("asset match: %s", asset['filename'])
            
            # create a playlist item based on the assets. See
            # https://github.com/info-beamer/package-hd-player/blob/master/node.json#L80-L98
            # to see the "shape" of each item.
            playlist.append({
                'file': asset['id'],
                'duration': asset['metadata']['duration']
            })
            
            # calculate the total duration as a sanity check
            total_duration += asset['metadata']['duration']

        self.log.info("Total duration: %f", total_duration)
        return playlist

    def upload_playlist(self, setup_name, playlist):        
        """Update the HD Player based setup with the playlist"""
        # First convert the setup_name into a numerical ID
        setup_id = int(self.config['setups'][setup_name])
        self.log.info("setup_name: %s, setup_id: %d", setup_name, setup_id)

        req = self.session.post(
            'https://info-beamer.com/api/v1/setup/%d' % setup_id,
            data = {
                'config': json.dumps({'': {'playlist': playlist}}),
                'mode': 'update',
            }   
        )
        if req.json()['ok']:
            self.log.info("Update successful")
        else:
            self.log.info("Update failed!")


