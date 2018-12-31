#!/usr/bin/python
# -*- coding: utf-8 -*-
# pylint: disable=line-too-long,star-args
# pychecker: disable=line-too-long disable=star-args

"""Tool for setting up playlist on info-beamer
Developed by Joseph T. Foley <foley at RU dot IS>
Based upon conversation with Florian Wesch
https://community.infobeamer.com/t/hd-player-with-large-files/241/13
Started 2018-12-30
Project home https://project.cs.ru.is/projects/raspi-art/

You should first create a info_beamer.ini file from the info_beamer-template.ini
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
import argparse
import logging
import ConfigParser #ConfigParser in py2 configparser in py3

import requests
import InfoBeamer

if __name__ == '__main__':
    # http://stackoverflow.com/questions/8299270/ultimate-answer-to-relative-python-imports
    # relative imports do not work when we run this module directly
    PACK_DIR = os.path.dirname(os.path.join(os.getcwd(), __file__))
    ADDTOPATH = os.path.normpath(os.path.join(PACK_DIR, '..'))
    # add more .. depending upon levels deep
    sys.path.append(ADDTOPATH)

    SCRIPTPATH = os.path.dirname(os.path.realpath(__file__))
    DEVNULL = open(os.devnull, 'wb')

if __name__ == '__main__':
    PARSER = argparse.ArgumentParser(
        description='Tool for setting up info-beamer playlists')
    PARSER.add_argument('--version', action="version", version="%(prog)s 0.1")  #version init was depricated
    PARSER.add_argument('setup',#required!
                        help='which setup to connect with')
    PARSER.add_argument('file_regexp',
                        help='File regexp for the playlist')
    PARSER.add_argument('-f', '--configfile',
                        help='configuration file location (override)')
    ARGS = PARSER.parse_args()

    IBP = InfoBeamer.InfoBeamer(ARGS)
    
    PLAYLIST = IBP.build_playlist(ARGS.file_regexp)
    IBP.upload_playlist(ARGS.setup, PLAYLIST)
