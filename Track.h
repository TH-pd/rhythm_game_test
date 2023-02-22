#pragma once
#include <iostream>
#include <vector>
#include <fstream>
#include "note.h"
using namespace std;

#define TRACK_DECLARATION 0x4D54726B    //  トラックのヘッダ

//  各種イベント/システムイベント
#define META_IVENT        0xFF
#define SET_TEMPO         0x51
#define END_OF_TRACK      0x2F
#define NOTES_ON          0x90
#define NOTES_OFF         0x80
#define KEY_PRESSURE      0xA0
#define CTRL_CHANGE       0xB0
#define PROG_CHANGE       0xC0
#define CHANNEL_PRESSURE  0xD0
#define PITCH_CHANGE      0xE0
#define SYSEX_EVENT       0xF0

/// <summary>
/// ビッグエンディアンで4バイト読み込む
/// </summary>
/// <param name="buf">データの先頭</param>
/// <returns>読み込んだ値</returns>
inline int read_big_endian_int(const char* buf)
{
    int ret;
    int value = read_little_endian_int(buf);
    ret = value << 24;
    ret |= (value << 8) & 0x00FF0000;
    ret |= (value >> 8) & 0x0000FF00;
    ret |= (value >> 24) & 0x000000FF;
    return ret;
}

/// <summary>
/// ビッグエンディアンで2バイト読み込む
/// </summary>
/// <param name="buf">データの先頭</param>
/// <returns>読み込んだ値</returns>
inline short read_big_endian_short(const char* buf)
{
    short ret;
    short value = read_little_endian_short(buf);
    ret = value << 8;
    ret |= (value >> 8) & 0xFF;
    return ret;
}

/// <summary>
/// ビッグエンディアンで3バイト読み込む
/// </summary>
/// <param name="buf">データの先頭</param>
/// <returns>読み込んだ値</returns>
inline int read_big_endian_3byte(const char* buf) {
    return (read_big_endian_int(buf) >> 8) & 0x00FFFFFF;
}

/// <summary>
/// tickを(micro sec)に変換する
/// </summary>
/// <param name="tick">tick</param>
/// <param name="resolution">分解能</param>
/// <param name="tempo">テンポ</param>
/// <returns>micro sec</returns>
inline double tick2sec(unsigned long long tick, unsigned short resolution, unsigned int tempo) {
    return double(1.0 * tick * tempo / resolution);
}

unsigned long long calc_variable_length(char** buf);

//  テンポ
struct tempo
{
    unsigned int tempo;         //  テンポ（1拍子が何マイクロ秒で再生されるか）
    unsigned long long tick;    //  テンポが変わるタイミング
};

class TRACK
{
    unsigned int header = 0;                 //MTrk
    unsigned long long _len = 0;             //トラック長(tick)
    unsigned char max_scale = 0;             //最大の音階
    unsigned char min_scale = 0xff;          //最小の音階
    vector<note*> notes;
    vector<tempo*> tempos;
public:
    TRACK();
    ~TRACK();
    void Init();
    bool Read_Track(FILE* in);
    int Get_tempos_size() const { return tempos.size(); };
    tempo* Get_tempo(int i) const { return tempos[i]; }
    void Print_notes() const;
    void Decision_Lane();
    void Get_Sec(unsigned short resolution, vector<tempo*> tmp, float notes_speed);
    double Get_Sec(unsigned short resolution, vector<tempo*> tmp, float notes_speed, unsigned long long len);
    void Print_Sec() const;
    vector<note*> Get_Notes() const { return notes; };
    unsigned long long Get_len() const { return _len; };
};
