#pragma once
#include "pch.h"
#include <string>
#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fstream>
#include <complex.h>
#include <vector>
#include <cstring>
#include "note.h"

#define PI 3.14159265358979323846
#define WAV_WAVE 0x45564157         //  WAVEファイルのヘッダ
#define WAV_RIFF 0x46464952         //  RIFFチャンクのヘッダ
#define WAV_FMT  0x20746D66         //  FMT チャンクのヘッダ
#define WAV_DATA 0x61746164         //  DATAチャンクのヘッダ
#define MAX_FREQ 2000               //  ノーツにする最大の周波数
#define MIN_FREQ 400                //  ノーツにする最小の周波数
#define WAV_MAGNI_STEP 1024         //  ノーツを正規化する際にいくつの階級に分けるか

//  各難易度について、1秒あたりいくつ程度のノーツ数とするか
#define WAV_EASY     2.0
#define WAV_NORMAL   2.5
#define WAV_HARD     3.0
#define WAV_VERYHARD 3.5

using namespace std;

void make_hamming_wintbl(int n, float wintbl[]);

vector<complex<double>> fft(vector<complex<double>> a, bool inverse);

vector<complex<double>> fft(vector<double> a, bool inverse);

namespace wav {
    struct RIFF {
        int id;
        int filesize;
        int type;
    };
    struct FMT {
        int id;
        int size;
        unsigned short type;
        unsigned short channel;
        unsigned int sample;
        unsigned int byte;
        unsigned short block;
        unsigned short bit;
    };
    struct DATA {
        int id;
        unsigned int size;
        unsigned int arraysize;
        char* data8;
        short* data16;
        float* data32;
        double* data;
    };
    struct NOTE {
        unsigned int time;
        unsigned int hz;
        double relative_magnitude;
        int lane;
    };
}


class WAV_LOADER {
    wav::RIFF riff_chunk = wav::RIFF();
    wav::FMT   fmt_chunk = wav::FMT();
    wav::DATA data_chunk = wav::DATA();
public:
    WAV_LOADER() {};
    ~WAV_LOADER() {};
    bool Load(const char* fileName);
    void Clear();
    void Print_Riff() const;
    void Print_Fmt() const;
    void Print_Data() const;
    unsigned int Get_Datasize() const { return data_chunk.size; };
    double Get_Data(unsigned int i) const { return data_chunk.data[i]; };
    unsigned int Get_Samplerate() const { return fmt_chunk.sample; };
    unsigned int Get_Byte() const { return fmt_chunk.byte; };
    unsigned int Get_Bit() const { return (unsigned int)fmt_chunk.bit; };
    unsigned int Get_Arraysize() const { return data_chunk.arraysize; };
    unsigned int Get_Channel() const { return (unsigned int)fmt_chunk.channel; };
};

class WAV_AUDIO {
public:
    //  length:曲の長さ(sec)
    //  unit_time:処理1回あたりの曲の長さ(micro sec)
    double magnitude_max = 0, magnitude_min = 1.0E+30, length=0, unit_time = 1.0;
    vector<vector<double>>          lane_magnis;
    vector<unsigned long>           magnitude_class;
    vector<vector<double>>          magnis;
    vector<vector<unsigned int>>    hzs;
    vector<note*> notes;
    float difficulty = 0;
    WAV_AUDIO() {};
    ~WAV_AUDIO() {};
    void Init(unsigned int vector_size, double ut, int d);
    void Clear() {};
    void Get_Diff_LogSpec(unsigned int t, vector<double> spec);
    void MakeNotes(float notes_speed);
    int Decision_Lane(unsigned int hz) const;
};

vector<note*> wav2notes(char* filename, float notes_speed, int difficulty);

