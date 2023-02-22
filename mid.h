#pragma once

#include <stdio.h>
#include <string.h>
#include <tchar.h>
#include <cstring>
#include "Track.h"


#define HEADER_SIZE 0x08+0x06

#define MIDI_DECLARATION  0x4d546864    //  .mid�t�@�C���̃w�b�_

//header
class MIDI_HEADER
{
    unsigned int header      = 0;   //  �w�b�_
    unsigned int header_len  = 0;   //  �w�b�_��
    unsigned short format    = 0;   //  �t�H�[�}�b�g 0-2
    unsigned short track_num = 0;   //  �g���b�N��
    unsigned short resolution= 0;   //  ����\�i1���q�ŉ�tick�i�ނ��j
public:
    MIDI_HEADER();
    ~MIDI_HEADER() {};
    bool ReadHeader(FILE* in);
    unsigned short Get_track() const { return track_num; };
    unsigned short Get_Resolution() const { return resolution; };
};

class AUDIODATA
{
    MIDI_HEADER hmid;
    vector<tempo*> tempos;
    TRACK play_track;
    unsigned long long length = 0;
    double totaltime;

    void Add_tempo(tempo* tempo);
    void Add_Track(TRACK* track);
public:
    AUDIODATA();
    ~AUDIODATA();
    void Init();
    void Print_Tempo() const;
    void Print_notes() const;
    int Read_Mid(char filename[], unsigned int target_track);
    void Decision_Lane();
    unsigned int Get_Resolution() const { return hmid.Get_Resolution(); };
    void Get_sec(float notes_speed);
    void Print_Sec() const;
    vector<note*> Get_Notes() const { return play_track.Get_Notes(); };
    vector<int>   Get_Tracknum(char filename[]);
    double TotalTime() { return totaltime; };
};

