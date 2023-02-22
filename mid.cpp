#include "pch.h"
#include "Track.h"
#include "mid.h"

/// <summary>
/// �ŏ�ʃr�b�g���t���O�Ƃ���ϒ��̃f�[�^��ǂݍ���
/// </summary>
/// <param name="buf">�ǂݍ��ރf�[�^�̐擪</param>
/// <returns>�ǂݍ��񂾐��l</returns>
unsigned long long calc_variable_length(char** buf) {
    unsigned long long result = 0;
    char val = *((*buf)++);
    result = val & 0x7F;
    while ((val >> 7) & 0x01)   //�ŏ�ʃr�b�g�������Ă���Ȃ玟�̃o�C�g���ǂ�
    {
        val = *((*buf)++);
        result <<= 7;
        result |= val & 0x7F;
    }
    return result;
}

/// <summary>
/// ������
/// </summary>
MIDI_HEADER::MIDI_HEADER() {
    header = 0;
    header_len = 0;
    format = 0;
    track_num = 0;
    resolution = 0;
}

/// <summary>
/// mid�t�@�C���̃w�b�_������ǂݍ���
/// </summary>
/// <param name="in">mid�t�@�C���̃t�@�C���|�C���^</param>
/// <returns>format=1��mid�t�@�C���Ȃ��true���Ԃ�</returns>
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

    //  �e��ϐ���ǂݍ���
    header = read_big_endian_int(header_buf);
    header_len = read_big_endian_int(header_buf + 0x04);
    format = read_big_endian_short(header_buf + 0x08);
    track_num = read_big_endian_short(header_buf + 0x0a);
    resolution = read_big_endian_short(header_buf + 0x0c);

    if (header != MIDI_DECLARATION) {   //  �w�b�_��mid�t�@�C���̂��̂łȂ��Ȃ�G���[
        cerr << "This file is not MIDI file." << endl;
        return false;
    }
    if (format != 1) {                  //  format=0�͔�Ή��Ȃ̂ŃG���[
        cerr << "format " << format << " is not supported." << endl;
        return false;
    }
    return true;
}

/// <summary>
/// �e���|��tempos�ɒǉ�����
/// </summary>
/// <param name="t">�e���|</param>
void AUDIODATA::Add_tempo(tempo* t) {
    tempos.push_back(t);
    return;
}

/// <summary>
/// �v���C����g���b�N��ݒ肷��
/// </summary>
/// <param name="track">�g���b�N</param>
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
/// mid�t�@�C������w�肳�ꂽ�g���b�N�̏���ǂݎ��
/// �ǂݍ��񂾏���tempos, play_track�ɕۑ������
/// </summary>
/// <param name="filename">�t�@�C����</param>
/// <param name="target_track">�ǂݍ��ރg���b�N�ԍ�</param>
/// <returns></returns>
int AUDIODATA::Read_Mid(char filename[], unsigned int target_track)
{
    FILE* fs;
    fopen_s(&fs, filename, "rb");
    if (!(hmid.ReadHeader(fs))) { return -1; };

    short track_num = hmid.Get_track(); //  �g���b�N�����擾

    for (unsigned int i = 0; i < (unsigned int)track_num; i++)
    {
        TRACK* T = new TRACK;
        if (!(T->Read_Track(fs))) { return -1; };
        for (int j = 0; j < T->Get_tempos_size(); j++)  //  �ǂݍ��񂾃g���b�N�Ƀe���|�Ɋւ���f�[�^������Εۑ�
        {
            tempo* t = T->Get_tempo(j);
            if (tempos.size() == 0 && t->tick != 0) { t->tick = 0; }
            Add_tempo(t);
        }
        if (length < T->Get_len()) { length = T->Get_len(); }
        if (i == target_track) {                        //  �ړI�Ƃ���g���b�N�Ȃ�ۑ�
            Add_Track(T);
        }
        else if (T->Get_tempos_size() == 0) {
            delete(T);
        }
    }
    return 0;
}

//  �m�[�c�̃��[��������U��
void AUDIODATA::Decision_Lane() {
    play_track.Decision_Lane();
    return;
}

/// <summary>
/// notes��Tick��(micro sec)�ɕϊ�����
/// </summary>
/// <param name="notes_speed">�m�[�c�̑���</param>
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

//  ������
void AUDIODATA::Init() {
    hmid = MIDI_HEADER();
    tempos.clear();
    tempos.shrink_to_fit();
    play_track.Init();
    length = 0;
}

/// <summary>
/// mid�t�@�C���̃g���b�N���Ɖ����̐����擾����
/// </summary>
/// <param name="filename">�t�@�C����</param>
/// <returns>�g���b�N�ԍ��ɑΉ�����v�f�ԍ��ɉ����̐����������z��</returns>
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