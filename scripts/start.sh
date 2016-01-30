# Attempt to auto-start the DVD program if we should.
if [ -f /home/pi/autostart ];
then
    cd /home/pi/
    ./mount_first_usb.sh
    sudo ./dvdemu /dev/ttyUSB0 /home/pi/videos/
else
    echo "No autostart file in /home/pi, going to terminal."
fi
