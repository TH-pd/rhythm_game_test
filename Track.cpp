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

//  ������
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
/// ���݂̃t�@�C���|�C���^����g���b�N����ǂݍ���
/// </summary>
/// <param name="in">�t�@�C���|�C���^</param>
/// <returns>����ɓǂݍ��߂��true</returns>
bool TRACK::Read_Track(FILE* in)
{
    char len_str[4], header_str[4];

    fread(header_str, 4, 1, in);
    fread(len_str, 4, 1, in);
    header = read_big_endian_int(header_str);
    int len = read_big_endian_int(len_str);

    char* buf = (char*)malloc(sizeof(char) * len), * buf_p;
    buf_p = buf;

    if ((header != TRACK_DECLARATION) || (buf == NULL)) { return false; };  //  �w�b�_���Ԉ���Ă����false

    if (fread(buf, len, 1, in) == 0) { return false; };
    char* end = buf + len;
    bool endflg = false;
    unsigned long long deltatime = 0;
    unsigned long long notes_start[0xff] = { 0 };  //  ���K���ƂɊe�����̒���(tick)���L�^����

    for (; buf < end; )
    {
        deltatime += calc_variable_length(&buf);                //  �C�x���g�����̃^�C�~���O�ł���f���^�^�C��(tick)���X�V
        unsigned char flg = *(buf++);                           //  flg=�C�x���g�̃^�C�v
        if (flg == META_IVENT) {                                //  ���^�C�x���g�Ȃ�A�e���|�̕ύX�ƃg���b�N�̏I�����������o���A�c��͖������Đi�߂�
            char type = *(buf++);                               //  type = ���^�C�x���g�̃^�C�v
            long long meta_len = calc_variable_length(&buf);    //  ���^�C�x���g�̃f�[�^��
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
            if (endflg) { break; }                              //  �g���b�N�̍Ō�ł���ΏI��
        }
        else {
            switch (flg & 0xF0)                                 //  flg�̉���4�r�b�g�̓`�����l���ł���̂ł����ł͖�������
            {
            case NOTES_ON:                                      //  �����̐擪�ł���Ȃ�L�^
            {
                char scale = *(buf++);
                char volume = *(buf++);
                if (volume != 0) {                              //  �����J�n
                    notes_start[scale] = deltatime;
                    if (scale > max_scale) {
                        max_scale = scale;
                    }
                    else if (scale < min_scale) {
                        min_scale = scale;
                    }
                }
                else {                                          //  �����̏I�[��傫��0�̉��𔭉����邱�Ƃŕ\�����邱�Ƃ�����̂ł�����I�[�Ƃ���
                    note* n = new note;
                    n->scale = scale;
                    n->tick = notes_start[scale];
                    n->length_tick = deltatime - n->tick;
                    notes.push_back(n);
                }
                break;
            }
            case NOTES_OFF:                                     //  �����̏I�[
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
            case KEY_PRESSURE:                                  //  �����̐擪�ƏI�[�̃C�x���g�ȊO�͓K�X�������Đi�߂�
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
/// notes�̃��[����U�蕪����
/// </summary>
void TRACK::Decision_Lane() {
    int size = notes.size();
    //  �g���b�N���̑S�Ẳ����̒��ōł�����������Ⴂ���܂ł�4�������ĐU�蕪����
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
/// �m�[�c��tick��b�ɕϊ�����
/// </summary>
/// <param name="resolution">����\</param>
/// <param name="tmp">�e���|</param>
/// <param name="notes_speed">�m�[�c�̑���</param>
void TRACK::Get_Sec(unsigned short resolution, vector<tempo*> tmp, float notes_speed) {
    int tmp_size = int(tmp.size());
    int tick_cnt = 1;
    double current_time = 0.0;
    float overlap = NOTES_OVERLAP * notes_speed;    //  �m�[�c�Ԃ̋����̍Œ�ۏ�
    double latest_notes[4] = { -1.0 * double(notes_speed), -1.0 * double(notes_speed), -1.0 * double(notes_speed), -1.0 * double(notes_speed) };    //�e���[���̈�ԍŌ�̃m�[�c
    unsigned long long longnotes_border = (unsigned long long)(double(resolution) * 3.5); //  4/4���q��1���ߒ��x����������O�m�[�c�Ƃ���

    for (unsigned int i = 0; i < notes.size();) {
        while (tick_cnt < tmp_size) {                       //  �e���|�̕ύX������΁A�����܂łɂ����鎞�Ԃ��Ɍv�Z���Ă���
            if (notes[i]->tick >= tmp[tick_cnt]->tick) {
                current_time += tick2sec(tmp[tick_cnt]->tick - tmp[tick_cnt - 1]->tick, resolution, tmp[tick_cnt - 1]->tempo);
                tick_cnt++;
            }
            else {
                break;
            }
        }

        notes[i]->sec = double(current_time + tick2sec(notes[i]->tick - tmp[tick_cnt - 1]->tick, resolution, tmp[tick_cnt - 1]->tempo));    //  notes[i]->sec=�e���|���Ō�ɕς�������� + ���݂̃e���|�Ōo�߂�������
        
        if (notes[i]->sec - latest_notes[notes[i]->lane] < overlap) {   //  �m�[�c�Ԃ̍Œ�ۏ؋������߂���΍폜
            notes.erase(notes.begin()+i);
        }
        else {
            if (notes[i]->length_tick >= longnotes_border) {            //  �����O�m�[�c�ɂ��邩�ۂ�
                notes[i]->length_sec = tick2sec(notes[i]->length_tick, resolution, tmp[tick_cnt - 1]->tempo);
            }
            else {
                notes[i]->length_sec = 0;
            }
            latest_notes[notes[i]->lane] = notes[i]->sec + notes[i]->length_sec;               //  �����łȂ���΍Ō�̃m�[�c���X�V
            i++;
        }
    }
    Decision_Lane();
    return;
}

/// <summary>
/// �g���b�N����tick��b�ɕϊ�����
/// </summary>
/// <param name="resolution">����\</param>
/// <param name="tmp">�e���|</param>
/// <param name="len">�g���b�N��(tick)</param>
/// <param name="notes_speed">�m�[�c�̑���</param>
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
