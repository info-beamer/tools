#!/bin/bash
# first trim the files that are not the same
TRIMBASES=("Video3" "Video5")
TRIMTIME=00:20:25.6
for Basename in ${TRIMBASES[*]}
do
    if [ ! -r $Basename-orig.mp4 ]; then
        echo "Backing up $Basename.mp4 to $Basename-orig.mp4"
        cp $Basename.mp4 $Basename-orig.mp4
    else
        echo "$Basename-orig.mp4 exists, skipping backup"
    fi

    echo "Trimming $Basename-orig.mp4 and putting in $Basename.mp4"
    ffmpeg -y -i $Basename-orig.mp4 -c copy -ss 00:00:00 -t $TRIMTIME $Basename.mp4
done

