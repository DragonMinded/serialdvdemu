#! /bin/bash

for device in /sys/block/*/device; do
    if echo $(readlink -f "$device")  | egrep -q "usb" ; then
        disk=$(echo "$device" | cut -f4 -d/)
        # Try the disk firs
        if sudo mount -t auto /dev/$disk /home/pi/videos 2> /dev/null ; then
            echo "Mounted /dev/$disk to /home/pi/videos"
            exit
        fi
        # Try the first partition
        if sudo mount -t auto /dev/${disk}1 /home/pi/videos 2> /dev/null ; then
            echo "Mounted /dev/${disk}1 to /home/pi/videos"
            exit
        fi
    fi
done
echo "Could not find a USB drive to mount!"
