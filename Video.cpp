#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <memory.h>
#include "Video.h"
#include "DVDEmu.h"

#define OPCODE_STOP 1
#define OPCODE_PLAY 2
#define OPCODE_PAUSE 3
#define OPCODE_UNPAUSE 4

/* Emulator state */
struct video_state
{
    pthread_t thread;
    pthread_mutex_t lock;
    unsigned char queue[256];
    unsigned int queue_pos;
    unsigned int power_state;
    unsigned int playing_state;
    unsigned int paused_state;
    unsigned int pause_delay;
    unsigned int chapter;
    unsigned int max_chapter;
    char * dvd_path;
} video_state;

void *VideoThread( void *state )
{
    /* Get state to read from */
    struct video_state *private_state = (struct video_state *)state;
    int loaded = 0;

    while( true )
    {
        /* Process video commands, safe to read this because
         * if it is non-zero we will try locking anyway. */
        if( private_state->queue_pos > 0 )
        {
            pthread_mutex_lock(&private_state->lock);
            unsigned int amount = 0;
            unsigned int opcode = private_state->queue[0];
            unsigned int arg = private_state->queue[1];
            switch( opcode )
            {
                case OPCODE_STOP:
                    /* Stop */
                    amount = 1;
                    break;
                case OPCODE_PAUSE:
                    /* Pause */
                    amount = 1;
                    break;
                case OPCODE_UNPAUSE:
                    /* Pause */
                    amount = 1;
                    break;
                case OPCODE_PLAY:
                    /* Seek */
                    amount = 2;
                    break;
            }

            /* Remove if needed */
            if( amount < private_state->queue_pos )
            {
                memmove(&private_state->queue[0], &private_state->queue[amount], private_state->queue_pos - amount);
            }
            private_state->queue_pos -= amount;

            pthread_mutex_unlock(&private_state->lock);

            /* Perform operation */
            switch( opcode )
            {
                case OPCODE_STOP:
                    /* Stop */
                    if( loaded )
                    {
                        exec_shell("./control.sh stop > /dev/null 2> /dev/null");
                        exec_shell("killall dbus-daemon > /dev/null 2> /dev/null");
                        exec_shell("killall omxplayer > /dev/null 2> /dev/null");
                        exec_shell("killall omxplayer.bin > /dev/null 2> /dev/null");
                        loaded = 0;
                    }
                    break;
                case OPCODE_PAUSE:
                    /* Pause */
                    if( loaded )
                    {
                        exec_shell("./control.sh pause > /dev/null 2> /dev/null");
                    }
                    break;
                case OPCODE_UNPAUSE:
                    /* Pause */
                    if( loaded )
                    {
                        if( private_state->pause_delay )
                        {
                            usleep( private_state->pause_delay * 1000 );
                        }
                        exec_shell("./control.sh pause > /dev/null 2> /dev/null");
                    }
                    break;
                case OPCODE_PLAY:
                    /* Play */
                    if( !loaded )
                    {
                        /* TODO: Search for videos based on prepended zero or not.
                         * Also accept any extension that we support.
                         */
                        char syscall[256];
                        sprintf(syscall, "omxplayer -b --no-osd %s%02d.m4v > /dev/null 2> /dev/null &", private_state->dvd_path, arg);
                        exec_shell(syscall);
                        /* Wait for it to start */
                        while( 1 )
                        {
                            int exitcode = exec_shell("./control.sh status > /dev/null 2> /dev/null");
                            if( exitcode == 0 )
                            {
                                break;
                            }
                        }
                        loaded = 1;
                    }
                    break;
            }
        }
        else
        {
            /* Sleep for a bit */
            usleep( 10000 );
        }
    }
}

void VideoThreadAction( unsigned int opcode, unsigned int argument )
{
    pthread_mutex_lock(&video_state.lock);

    switch(opcode)
    {
        case OPCODE_STOP:
            /* Stop */
            video_state.queue[video_state.queue_pos] = OPCODE_STOP;
            video_state.queue_pos++;
            break;
        case OPCODE_PAUSE:
            /* Pause */
            video_state.queue[video_state.queue_pos] = OPCODE_PAUSE;
            video_state.queue_pos++;
            break;
        case OPCODE_UNPAUSE:
            /* Pause */
            video_state.queue[video_state.queue_pos] = OPCODE_UNPAUSE;
            video_state.queue_pos++;
            break;
        case OPCODE_PLAY:
            /* Seek */
            video_state.queue[video_state.queue_pos] = OPCODE_PLAY;
            video_state.queue_pos++;
            video_state.queue[video_state.queue_pos] = argument;
            video_state.queue_pos++;
            break;
    }

    pthread_mutex_unlock(&video_state.lock);
}

void VideoInit( char * path, int chapters )
{
    video_state.power_state = 0;
    video_state.playing_state = 0;
    video_state.paused_state = 0;
    video_state.chapter = 1;
    video_state.dvd_path = path;
    video_state.max_chapter = chapters;
    video_state.pause_delay = 0;

    verbose_printf( "Disc has %d chapters.\n", video_state.max_chapter );

    /* Start video playback thread */
    memset( video_state.queue, 0, sizeof(video_state.queue) );
    video_state.queue_pos = 0;
    pthread_mutex_init(&video_state.lock, NULL);
    pthread_create(&video_state.thread, NULL, VideoThread, &video_state);
}

void VideoSetPauseDelay( unsigned int milliseconds )
{
    video_state.pause_delay = milliseconds;
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

        VideoThreadAction( OPCODE_PAUSE, 0 );

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

        VideoThreadAction( OPCODE_UNPAUSE, 0 );

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
            VideoThreadAction( OPCODE_STOP, 0 );

            /* Seek */
            VideoThreadAction( OPCODE_PLAY, chapter );
            video_state.playing_state = 1;

            /* Repause if needed */
            if( video_state.paused_state )
            {
                VideoThreadAction( OPCODE_PAUSE, 0 );
            }

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

        VideoThreadAction( OPCODE_STOP, 0 );

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
