#! /bin/bash

HOMEDIR=$(readlink -f ~)
mkdir ${HOMEDIR}/videos

for device in /sys/block/*/device; do
    if echo $(readlink -f "$device")  | egrep -q "usb" ; then
        disk=$(echo "$device" | cut -f4 -d/)
        # Try the disk firs
        if sudo mount -t auto /dev/$disk ${HOMEDIR}/videos 2> /dev/null ; then
            echo "Mounted /dev/$disk to ${HOMEDIR}/videos"
            exit
        fi
        # Try the first partition
        if sudo mount -t auto /dev/${disk}1 ${HOMEDIR}/videos 2> /dev/null ; then
            echo "Mounted /dev/${disk}1 to ${HOMEDIR}/videos"
            exit
        fi
    fi
done
echo "Could not find a USB drive to mount!"
