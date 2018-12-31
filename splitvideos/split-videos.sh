#!/bin/bash
QUALITY=$1
SEGMENT_TIME=60

# now split the videos
for Infile in `ls *.mp4`
do
    Basename=${Infile%.*}
    infile=`echo "$Infile" | tr '[:upper:]' '[:lower:]'`
    basename=${infile%.*}
    ffmpeg -y -i $Basename.mp4 -c copy -map 0 -segment_time $SEGMENT_TIME -f segment -reset_timestamps 1 $basename-$QUALITY-%03d.mp4

done
