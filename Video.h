#ifndef __VIDEO_H
#define __VIDEO_H

void VideoInit( char * path, int chapters );

void PowerOn();
void PowerOff();
void Play();
void Pause();
void Unpause();
int SeekToChapter( unsigned int chapter );
void Stop();

int GetChapter();
int IsPowered();
int IsPlaying();

#endif
