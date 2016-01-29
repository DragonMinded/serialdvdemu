#!/bin/bash

#set -x

OMXPLAYER_DBUS_ADDR="/tmp/omxplayerdbus.${USER}"
OMXPLAYER_DBUS_PID="/tmp/omxplayerdbus.${USER}.pid"
export DBUS_SESSION_BUS_ADDRESS=`cat $OMXPLAYER_DBUS_ADDR`
export DBUS_SESSION_BUS_PID=`cat $OMXPLAYER_DBUS_PID`

[ -z "$DBUS_SESSION_BUS_ADDRESS" ] && { echo "Must have DBUS_SESSION_BUS_ADDRESS" >&2; exit 1; }

case $1 in
status)

playstatus=`dbus-send --print-reply=literal --session --reply-timeout=500 --dest=org.mpris.MediaPlayer2.omxplayer /org/mpris/MediaPlayer2 org.freedesktop.DBus.Properties.PlaybackStatus`
[ $? -ne 0 ] && exit 1
playstatus="$(sed 's/^ *//;s/ *$//;' <<< "$playstatus")"

paused="true"
[ "$playstatus" == "Playing" ] && paused="false"
echo "Paused: $paused"
;;

pause)
dbus-send --print-reply=literal --session --dest=org.mpris.MediaPlayer2.omxplayer /org/mpris/MediaPlayer2 org.mpris.MediaPlayer2.Player.Action int32:16 >/dev/null
;;

stop)
dbus-send --print-reply=literal --session --dest=org.mpris.MediaPlayer2.omxplayer /org/mpris/MediaPlayer2 org.mpris.MediaPlayer2.Player.Action int32:15 >/dev/null
;;

*)
echo "usage: $0 status|pause|stop" >&2
exit 1
;;
esac
