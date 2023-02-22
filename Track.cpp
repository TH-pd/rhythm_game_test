#include "pch.h"
#include "Track.h"

TRACK::TRACK() {
    header = 0;
    _len = 0;
    max_scale = 0;
    min_scale = 0xff;
    notes.clear();
    tempos.clear();
}

TRACK::~TRACK() {
    notes.clear();
    notes.shrink_to_fit();
    tempos.clear();
    tempos.shrink_to_fit();
}

//  初期化
void TRACK::Init() {
    header = 0;
    _len = 0;
    max_scale = 0;
    min_scale = 0xff;
    notes.clear();
    notes.shrink_to_fit();
    tempos.clear();
    tempos.shrink_to_fit();
}

/// <summary>
/// 現在のファイルポインタからトラック情報を読み込む
/// </summary>
/// <param name="in">ファイルポインタ</param>
/// <returns>正常に読み込めればtrue</returns>
bool TRACK::Read_Track(FILE* in)
{
    char len_str[4], header_str[4];

    fread(header_str, 4, 1, in);
    fread(len_str, 4, 1, in);
    header = read_big_endian_int(header_str);
    int len = read_big_endian_int(len_str);

    char* buf = (char*)malloc(sizeof(char) * len), * buf_p;
    buf_p = buf;

    if ((header != TRACK_DECLARATION) || (buf == NULL)) { return false; };  //  ヘッダが間違っていればfalse

    if (fread(buf, len, 1, in) == 0) { return false; };
    char* end = buf + len;
    bool endflg = false;
    unsigned long long deltatime = 0;
    unsigned long long notes_start[0xff] = { 0 };  //  音階ごとに各音符の長さ(tick)を記録する

    for (; buf < end; )
    {
        deltatime += calc_variable_length(&buf);                //  イベント発生のタイミングであるデルタタイム(tick)を更新
        unsigned char flg = *(buf++);                           //  flg=イベントのタイプ
        if (flg == META_IVENT) {                                //  メタイベントなら、テンポの変更とトラックの終了だけを検出し、残りは無視して進める
            char type = *(buf++);                               //  type = メタイベントのタイプ
            long long meta_len = calc_variable_length(&buf);    //  メタイベントのデータ長
            switch (type)
            {
            case SET_TEMPO:
            {
                tempo* t = new tempo;
                t->tempo = read_big_endian_3byte(buf);
                t->tick = deltatime;
                tempos.push_back(t);
                break;
            }
            case END_OF_TRACK:
                _len = deltatime;
                endflg = true;
                break;
            default:
                break;
            }
            buf += meta_len;
            if (endflg) { break; }                              //  トラックの最後であれば終了
        }
        else {
            switch (flg & 0xF0)                                 //  flgの下位4ビットはチャンネルであるのでここでは無視する
            {
            case NOTES_ON:                                      //  音符の先頭であるなら記録
            {
                char scale = *(buf++);
                char volume = *(buf++);
                if (volume != 0) {                              //  発音開始
                    notes_start[scale] = deltatime;
                    if (scale > max_scale) {
                        max_scale = scale;
                    }
                    else if (scale < min_scale) {
                        min_scale = scale;
                    }
                }
                else {                                          //  音符の終端を大きさ0の音を発音することで表現することがあるのでこれも終端とする
                    note* n = new note;
                    n->scale = scale;
                    n->tick = notes_start[scale];
                    n->length_tick = deltatime - n->tick;
                    notes.push_back(n);
                }
                break;
            }
            case NOTES_OFF:                                     //  音符の終端
            {
                char scale = *(buf++);
                buf++;                                          //  char volume = *(buf++);
                note* n = new note;
                n->scale = scale;
                n->tick = notes_start[scale];
                n->length_tick = deltatime - n->tick;
                notes.push_back(n);
                break;
            }
            case KEY_PRESSURE:                                  //  音符の先頭と終端のイベント以外は適宜無視して進める
            case CTRL_CHANGE:
            case PITCH_CHANGE:
                buf += 2;
                break;
            case PROG_CHANGE:
            case CHANNEL_PRESSURE:
                buf++;
                break;
            case SYSEX_EVENT:
                buf += *(buf++);
                break;
            default:
                cerr << "Unknown Event:" << hex << flg + 1 - 1 << endl;
                return false;
                break;
            }
        }
    }
    free(buf_p);
    return true;

}

void TRACK::Print_notes() const {
    ofstream ofs("./log_Notes.txt");
    int size = notes.size();
    for (int i = 0; i < size; i++) {
        ofs << "[" << dec << i << "] : Tick = " << float(notes[i]->tick);// / 480 / 4 + 1;
        ofs << ", scale = " << dec << (int)(notes[i]->scale);
        ofs << ", length = " << dec << (notes[i]->length_sec);
        ofs << ", lane = " << dec << (int)(notes[i]->lane) << endl;
    }
    return;
}

void TRACK::Print_Sec() const {
    ofstream ofs("./log_sec.txt");
    int size = notes.size();
    for (int i = 0; i < size; i++) {
        ofs << "[" << dec << i << "] :";
        ofs << " sec = " << dec << notes[i]->sec << " (mms) " << endl;
    }
    return;
}

/// <summary>
/// notesのレーンを振り分ける
/// </summary>
void TRACK::Decision_Lane() {
    int size = notes.size();
    //  トラック中の全ての音符の中で最も高い音から低い音までを4等分して振り分ける
    int param1 = (3 * min_scale + max_scale) / 4;
    int param2 = (min_scale + max_scale) / 2;
    int param3 = (min_scale + 3 * max_scale) / 4;

    for (int i = 0; i < size; i++) {
        char scale = notes[i]->scale;
        if (scale < param1) { notes[i]->lane = 0; }
        else if (scale < param2) { notes[i]->lane = 1; }
        else if (scale < param3) { notes[i]->lane = 2; }
        else { notes[i]->lane = 3; }
    }
    return;
}

/// <summary>
/// ノーツのtickを秒に変換する
/// </summary>
/// <param name="resolution">分解能</param>
/// <param name="tmp">テンポ</param>
/// <param name="notes_speed">ノーツの速さ</param>
void TRACK::Get_Sec(unsigned short resolution, vector<tempo*> tmp, float notes_speed) {
    int tmp_size = int(tmp.size());
    int tick_cnt = 1;
    double current_time = 0.0;
    float overlap = NOTES_OVERLAP * notes_speed;    //  ノーツ間の距離の最低保証
    double latest_notes[4] = { -1.0 * double(notes_speed), -1.0 * double(notes_speed), -1.0 * double(notes_speed), -1.0 * double(notes_speed) };    //各レーンの一番最後のノーツ
    unsigned long long longnotes_border = (unsigned long long)(double(resolution) * 3.5); //  4/4拍子で1小節程度からをロングノーツとする

    for (unsigned int i = 0; i < notes.size();) {
        while (tick_cnt < tmp_size) {                       //  テンポの変更があれば、そこまでにかかる時間を先に計算しておく
            if (notes[i]->tick >= tmp[tick_cnt]->tick) {
                current_time += tick2sec(tmp[tick_cnt]->tick - tmp[tick_cnt - 1]->tick, resolution, tmp[tick_cnt - 1]->tempo);
                tick_cnt++;
            }
            else {
                break;
            }
        }

        notes[i]->sec = double(current_time + tick2sec(notes[i]->tick - tmp[tick_cnt - 1]->tick, resolution, tmp[tick_cnt - 1]->tempo));    //  notes[i]->sec=テンポが最後に変わった時間 + 現在のテンポで経過した時間
        
        if (notes[i]->sec - latest_notes[notes[i]->lane] < overlap) {   //  ノーツ間の最低保証距離より近ければ削除
            notes.erase(notes.begin()+i);
        }
        else {
            if (notes[i]->length_tick >= longnotes_border) {            //  ロングノーツにするか否か
                notes[i]->length_sec = tick2sec(notes[i]->length_tick, resolution, tmp[tick_cnt - 1]->tempo);
            }
            else {
                notes[i]->length_sec = 0;
            }
            latest_notes[notes[i]->lane] = notes[i]->sec + notes[i]->length_sec;               //  そうでなければ最後のノーツを更新
            i++;
        }
    }
    Decision_Lane();
    return;
}

/// <summary>
/// トラック長のtickを秒に変換する
/// </summary>
/// <param name="resolution">分解能</param>
/// <param name="tmp">テンポ</param>
/// <param name="len">トラック長(tick)</param>
/// <param name="notes_speed">ノーツの速さ</param>
double TRACK::Get_Sec(unsigned short resolution, vector<tempo*> tmp, float notes_speed, unsigned long long len) {
    int tmp_size = int(tmp.size());
    int tick_cnt = 1;
    double current_time = 0.0;
    float latest_notes[4] = { -1.f * notes_speed, -1.f * notes_speed, -1.f * notes_speed, -1.f * notes_speed };
    ;
    while (tick_cnt < tmp_size) {
        current_time += tick2sec(tmp[tick_cnt]->tick - tmp[tick_cnt - 1]->tick, resolution, tmp[tick_cnt - 1]->tempo);
        tick_cnt++;
    }

    double result = double(current_time + tick2sec(len - tmp[tick_cnt - 1]->tick, resolution, tmp[tick_cnt - 1]->tempo));

    return result;
}
