#include <stdio.h>
#include <unistd.h>
#include "Video.h"
#include "DVDEmu.h"

/* Emulator state */
struct video_state
{
    unsigned int power_state;
    unsigned int playing_state;
    unsigned int paused_state;
    unsigned int chapter;
    unsigned int max_chapter;
    char * dvd_path;
} video_state;

void VideoInit( char * path, int chapters )
{
    video_state.power_state = 0;
    video_state.playing_state = 0;
    video_state.paused_state = 0;
    video_state.chapter = 1;
    video_state.dvd_path = path;
    video_state.max_chapter = chapters;

    verbose_printf( "Disc has %d chapters.\n", video_state.max_chapter );
}

void PowerOn()
{
    if( !video_state.power_state )
    {
        verbose_printf( "* Exit standby mode.\n" );

        video_state.power_state = 1;
        Stop();
    }
}

void PowerOff()
{
    if( video_state.power_state )
    {
        Stop();

        verbose_printf( "* Enter standby mode.\n" );

        video_state.power_state = 0;
    }
}

void Play()
{
    if( !video_state.power_state )
    {
        PowerOn();
    }

    if( !video_state.playing_state )
    {
        verbose_printf( "* Enter playing state.\n" );

        SeekToChapter(video_state.chapter);

        video_state.playing_state = 1;
        video_state.paused_state = 0;
    }
    else
    {
        /* Unpause only if we need to */
        Unpause();
    }
}

void Pause()
{
    if( video_state.power_state &&
        video_state.playing_state &&
        !video_state.paused_state )
    {
        verbose_printf( "* Enter paused state.\n" );

        exec_shell("./control.sh pause &");

        video_state.playing_state = 1;
        video_state.paused_state = 1;
    }
}

void Unpause()
{
    if( video_state.power_state &&
        video_state.playing_state &&
        video_state.paused_state )
    {
        verbose_printf( "* Exit paused state.\n" );

        exec_shell("./control.sh pause &");

        video_state.playing_state = 1;
        video_state.paused_state = 0;
    }
}

int SeekToChapter( unsigned int chapter )
{
    if( video_state.power_state )
    {
        /* Is the chapter in bounds? */
        if( chapter >= 1 && chapter <= video_state.max_chapter )
        {
            verbose_printf( "* Seek to chapter %d.\n", chapter );

            /* Kill player */
            exec_shell("./control.sh stop 2>/dev/null &");
            sleep( 1 );
            exec_shell("killall dbus-daemon");

            /* Seek */
            char syscall[256];
            sprintf(syscall, "omxplayer -b --no-osd %s%02d.m4v >/dev/null &", video_state.dvd_path, chapter);
            exec_shell(syscall);
            video_state.playing_state = 1;
            sleep( 1 );

            /* Repause if needed */
            if( video_state.paused_state ) { exec_shell("./control.sh pause 2>/dev/null &"); }

            video_state.chapter = chapter;
            return 1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        /* Nothing seek-wise while off */
        return 0;
    }
}

void Stop()
{
    if( video_state.power_state && 
        ( video_state.playing_state ||
          video_state.paused_state ) )
    {
        /* If we are paused, exit the pause state */
        Unpause();

        verbose_printf( "* Enter idle state.\n" );

        exec_shell("./control.sh stop &");
        sleep(1);
        exec_shell("killall dbus-daemon");

        video_state.playing_state = 0;
        video_state.paused_state = 0;
        video_state.chapter = 1;
    }
}

int GetChapter()
{
    return video_state.chapter;
}

int IsPowered()
{
    return video_state.power_state;
}

int IsPlaying()
{
    return video_state.playing_state;
}
