# serialdvdemu

DVD serial interface emulator for various DVD players. This started as an
emulator for a Victor XV-D701 to restore background video functionality
to Beatmania IIDX cabinets running Twinkle mixes. I've generalized it a
bit and am extending it to cover basic functionality of a Toshiba SD-B100
in order to restore DVD functionality to ParaParaParadise 2nd Mix. This
is a work in progress meant to help people who need to replace obsolete
players in various formats. Note that this hasn't been tested with sound,

## Operating Environment

This player uses omxplayer as the actual video player. It should be able
to play any file that omxplayer can, though I've hardcoded it for m4v files.
I haven't tested sound as the uses for this only require video, but there's
no reason why it wouldn't work. This is meant to be run on a Raspberry Pi 2
but should also be portable to any other Linux system.

## Current State

### Victor XV-D701

I've emulated the base JLIP protocol. Proper responses are returned for
recognized and unrecognized commands as well as commands with bad CRC.
Basic functions like play/pause/stop and seeking such as chapter commands
are fully supported. Basic query parameters and changing JLIP ID through
commands is fully supported. The rest of the spec is unimplemented as the
emulator is accurate enough to work with Beatmania IIDX.

### Toshiba SD-B100

I've emulated only the bare essentials here. I don't know much about the
protocol as I've reverse engineered everything I know out of the PPP 2nd
game binary and don't own a DVD player itself to experiment with. I'm not
sure what other appliations this will be useful for.
