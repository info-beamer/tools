Splitting Videos for use on info-beamer pi hosted

This is a set of small scripts to help play large videos in a synchronized manner on a set of screens

The key things are that the source videos must be exactly the same length or synchronization does not work.

File | Purpose
----------|--------------------
[ib_playlist.py](ib_playlist.py) | Put a playlist in a "setup" from the asset library based upon a regexp
[info_beamer-template.ini](info_beamer-template.ini) | Template for configuration file to save authentication keys and setups
[InfoBeamer.py](InfoBeamer.py) | Python abstraction for interfacing with the info-beamer REST api
[trim-videos.sh](trim-videos.sh) | Bash script for using ffmpeg to trim a list of videos to the same length
