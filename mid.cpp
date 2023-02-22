#include "pch.h"
#include "Track.h"
#include "mid.h"

/// <summary>
/// 最上位ビットをフラグとする可変長のデータを読み込む
/// </summary>
/// <param name="buf">読み込むデータの先頭</param>
/// <returns>読み込んだ数値</returns>
unsigned long long calc_variable_length(char** buf) {
    unsigned long long result = 0;
    char val = *((*buf)++);
    result = val & 0x7F;
    while ((val >> 7) & 0x01)   //最上位ビットが立っているなら次のバイトも読む
    {
        val = *((*buf)++);
        result <<= 7;
        result |= val & 0x7F;
    }
    return result;
}

/// <summary>
/// 初期化
/// </summary>
MIDI_HEADER::MIDI_HEADER() {
    header = 0;
    header_len = 0;
    format = 0;
    track_num = 0;
    resolution = 0;
}

/// <summary>
/// midファイルのヘッダ部分を読み込む
/// </summary>
/// <param name="in">midファイルのファイルポインタ</param>
/// <returns>format=1のmidファイルならばtrueが返る</returns>
bool MIDI_HEADER::ReadHeader(FILE* in)
{
    char header_buf[HEADER_SIZE];
    if (in == 0 || feof(in) || ferror(in)) {
        cerr << "invalid stream" << endl;
        return false;
    }
    fread(header_buf, HEADER_SIZE, 1, in);
    if (feof(in) || ferror(in)) {
        fprintf(stderr, "invalid stream\n");
        return false;
    }

    //  各種変数を読み込む
    header = read_big_endian_int(header_buf);
    header_len = read_big_endian_int(header_buf + 0x04);
    format = read_big_endian_short(header_buf + 0x08);
    track_num = read_big_endian_short(header_buf + 0x0a);
    resolution = read_big_endian_short(header_buf + 0x0c);

    if (header != MIDI_DECLARATION) {   //  ヘッダがmidファイルのものでないならエラー
        cerr << "This file is not MIDI file." << endl;
        return false;
    }
    if (format != 1) {                  //  format=0は非対応なのでエラー
        cerr << "format " << format << " is not supported." << endl;
        return false;
    }
    return true;
}

/// <summary>
/// テンポをtemposに追加する
/// </summary>
/// <param name="t">テンポ</param>
void AUDIODATA::Add_tempo(tempo* t) {
    tempos.push_back(t);
    return;
}

/// <summary>
/// プレイするトラックを設定する
/// </summary>
/// <param name="track">トラック</param>
void AUDIODATA::Add_Track(TRACK* track) {
    play_track = *track;
    return;
}

void AUDIODATA::Print_Tempo() const {
    ofstream ofs("./log_tempo.txt");
    int size = tempos.size();
    for (int i = 0; i < size; i++) {
        ofs << "tick[" << dec << i << "] = " << dec << (tempos[i]->tick)+1-1 << endl;
        ofs << "tempo[" << dec << i << "]=" << dec << (tempos[i]->tempo)+1-1 << endl;
    }
    return;
}

void AUDIODATA::Print_notes() const {
    play_track.Print_notes();
    return;
}

/// <summary>
/// midファイルから指定されたトラックの情報を読み取る
/// 読み込んだ情報はtempos, play_trackに保存される
/// </summary>
/// <param name="filename">ファイル名</param>
/// <param name="target_track">読み込むトラック番号</param>
/// <returns></returns>
int AUDIODATA::Read_Mid(char filename[], unsigned int target_track)
{
    FILE* fs;
    fopen_s(&fs, filename, "rb");
    if (!(hmid.ReadHeader(fs))) { return -1; };

    short track_num = hmid.Get_track(); //  トラック数を取得

    for (unsigned int i = 0; i < (unsigned int)track_num; i++)
    {
        TRACK* T = new TRACK;
        if (!(T->Read_Track(fs))) { return -1; };
        for (int j = 0; j < T->Get_tempos_size(); j++)  //  読み込んだトラックにテンポに関するデータがあれば保存
        {
            tempo* t = T->Get_tempo(j);
            if (tempos.size() == 0 && t->tick != 0) { t->tick = 0; }
            Add_tempo(t);
        }
        if (length < T->Get_len()) { length = T->Get_len(); }
        if (i == target_track) {                        //  目的とするトラックなら保存
            Add_Track(T);
        }
        else if (T->Get_tempos_size() == 0) {
            delete(T);
        }
    }
    return 0;
}

//  ノーツのレーンを割り振る
void AUDIODATA::Decision_Lane() {
    play_track.Decision_Lane();
    return;
}

/// <summary>
/// notesのTickを(micro sec)に変換する
/// </summary>
/// <param name="notes_speed">ノーツの速さ</param>
void AUDIODATA::Get_sec(float notes_speed) {
    play_track.Get_Sec(hmid.Get_Resolution(), tempos, notes_speed);
    totaltime = play_track.Get_Sec(hmid.Get_Resolution(), tempos, notes_speed, length);
}

void AUDIODATA::Print_Sec() const {
    play_track.Print_Sec();
}

AUDIODATA::AUDIODATA() {
    hmid = MIDI_HEADER();
    tempos.clear();
    play_track = TRACK();
}

AUDIODATA::~AUDIODATA() {
    tempos.clear();
    tempos.shrink_to_fit();
}

//  初期化
void AUDIODATA::Init() {
    hmid = MIDI_HEADER();
    tempos.clear();
    tempos.shrink_to_fit();
    play_track.Init();
    length = 0;
}

/// <summary>
/// midファイルのトラック数と音符の数を取得する
/// </summary>
/// <param name="filename">ファイル名</param>
/// <returns>トラック番号に対応する要素番号に音符の数が入った配列</returns>
vector<int> AUDIODATA::Get_Tracknum(char filename[]) {
    FILE* fs;
    vector<int> result;
    fopen_s(&fs, filename, "rb");
    if (!(hmid.ReadHeader(fs))) { return result; };

    short track_num = hmid.Get_track();
    for (unsigned int i = 0; i < (unsigned int)track_num; i++)
    {
        TRACK* T = new TRACK;
        if (!(T->Read_Track(fs))) { return result; };
        result.push_back(T->Get_Notes().size());
        delete(T);
    }
    return result;
}