#! /bin/bash

HOMEDIR=$(readlink -f ~)
if [ -f ${HOMEDIR}/autostart ];
then
    # Attempt to auto-start the DVD program if we should.
    cd ${HOMEDIR}
    echo "Autostart file found in ${HOMEDIR}, starting emulator."
    ./mount_first_usb.sh
    sudo ./dvdemu /dev/ttyUSB0 ${HOMEDIR}/videos/
else
    # Drop to console.
    echo "No autostart file in ${HOMEDIR}, going to terminal."
fi
