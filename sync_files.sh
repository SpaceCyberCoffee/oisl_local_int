#!/bin/bash

SOURCE="/mnt/extras/SSD/NOS3_RBT/nos3_luca_OISL/Backward_Sat"
DEST="/mnt/extras/SSD/NOS3_RBT/nos3_luca_OISL/42_backward"
FORWARD="/mnt/extras/SSD/NOS3_RBT/nos3_luca_OISL/nos3_rbt/components/oisl/fsw/src/fileInput/ireceive.txt"
BACKWARD="/mnt/extras/SSD/NOS3_RBT/nos3_luca_OISL/Sat_1_23/components/oisl/fsw/src/fileInput/ireceive.txt"
INTERVAL=2 # Check every 10 seconds (you can adjust this)

while true; do
    if [ -f "$SOURCE/F.txt" ]; then
        if [ ! -f "$DEST/F.txt" ]; then 
            cp "$SOURCE/F.txt" "$DEST/F.txt" 
        fi
        # Create the FORWARD file if F.txt exists
        touch "$FORWARD"
    else
        # If the source file doesn't exist, delete the destination file and FORWARD file
        if [ -f "$DEST/F.txt" ]; then
            rm "$DEST/F.txt"
        fi
        if [ -f "$FORWARD" ]; then
            rm "$FORWARD"
        fi
    fi

    if [ -f "$SOURCE/B.txt" ]; then
        if [ ! -f "$DEST/B.txt" ]; then 
            cp "$SOURCE/B.txt" "$DEST/B.txt" 
        fi
        # Create the BACKWARD file if B.txt exists
        touch "$BACKWARD"
    else
        # If the source file doesn't exist, delete the destination file and BACKWARD file
        if [ -f "$DEST/B.txt" ]; then
            rm "$DEST/B.txt"
        fi
        if [ -f "$BACKWARD" ]; then
            rm "$BACKWARD"
        fi
    fi

    sleep $INTERVAL
done
