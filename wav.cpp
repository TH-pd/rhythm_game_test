
#include <pch.h>
#include "wav.h"

/// <summary>
/// ハミング窓を作成する
/// </summary>
/// <param name="n">配列の長さ</param>
/// <param name="wintbl">ハミング窓を格納する配列</param>
void make_hamming_wintbl(int n, float wintbl[]) {
    int i;

    for (i = 0; i < n; i++) {
        wintbl[i] = 0.54f - 0.46f * (float)cos((2.0f * PI * i) / ((double)n - 1));
    }
}

/// <summary>
/// 高速フーリエ変換(FFT)を行う
/// </summary>
/// <param name="a">フーリエ変換を行う配列</param>
/// <param name="inverse">trueならば逆フーリエ変換を行う</param>
/// <returns>フーリエ変換を行った配列</returns>
vector<complex<double>> fft(vector<complex<double>> a, bool inverse = false) {
    int n = a.size();
    int h = 0; // h = log_2(n)
    for (int i = 0; 1 << i < n; i++) h++;

    for (int i = 0; i < n; i++) {
        int j = 0;
        for (int k = 0; k < h; k++) j |= (i >> k & 1) << (h - 1 - k);
        if (i < j) swap(a[i], a[j]);
    }

    for (int b = 1; b < n; b *= 2) {

        for (int j = 0; j < b; j++) {

            complex<double> w =
                polar(1.0, (2 * PI) / (2 * (double)b) * j * (inverse ? 1 : -1));
            for (int k = 0; k < n; k += b * 2) {

                complex<double> s = a[j + k];
                complex<double> t = a[j + k + b] * w;
                a[j + k] = s + t;
                a[j + k + b] = s - t;
            }
        }
    }

    if (inverse)
        for (int i = 0; i < n; i++) a[i] /= n;
    return a;
}

/// <summary>
/// 高速フーリエ変換(FFT)を行う
/// </summary>
/// <param name="a">フーリエ変換を行う配列</param>
/// <param name="inverse">trueならば逆フーリエ変換を行う</param>
/// <returns>フーリエ変換を行った配列</returns>
vector<complex<double>> fft(vector<double> a, bool inverse = false) {
    unsigned int vector_size = 1;
    unsigned int n = a.size();
    for (unsigned int i = 0; (unsigned int)1 << i < n; i++) vector_size <<= 1;
    vector<complex<double>> a_complex(vector_size);
    unsigned int i, step = (vector_size - n) / 2;
    //0ß
    for (i = 0; i < step; i++)
    {
        a_complex[i] = complex<double>(0, 0);
    }
    for (; i < n + step; i++)
    {
        a_complex[i] = complex<double>(a[i - step], 0);
    }
    //0ß
    for (; i < vector_size; i++)
    {
        a_complex[i] = complex<double>(0, 0);
    }
    return fft(a_complex, inverse);
}

/// <summary>
/// wavファイルを読み込み、データを構造体に格納する
/// </summary>
/// <param name="fileName">ファイル名</param>
/// <returns>正常にロードをできたならばtrueが返る</returns>
bool WAV_LOADER::Load(const char* fileName) {
    FILE* file = nullptr;
    if (fopen_s(&file, fileName, "rb") != 0) { return false; }
    fread(&riff_chunk, sizeof(riff_chunk), 1, file);
    if (riff_chunk.id != WAV_RIFF || riff_chunk.type != WAV_WAVE) { return false; }

    while (1) {
        fread(&(data_chunk.id), sizeof(data_chunk.id), 1, file);
        fread(&(data_chunk.size), sizeof(data_chunk.size), 1, file);
        if (data_chunk.id == WAV_DATA) { break; }
        else if (data_chunk.id == WAV_FMT) {
            fseek(file, (-1) * (long)(sizeof(data_chunk.id) + sizeof(data_chunk.size)), SEEK_CUR);
            fread(&fmt_chunk, sizeof(fmt_chunk), 1, file);

            if (fmt_chunk.id != WAV_FMT) { return false; }

            char* extend;
            extend = (char*)malloc(sizeof(char) * (fmt_chunk.size - (sizeof(fmt_chunk) - sizeof(fmt_chunk.id) - sizeof(fmt_chunk.size))));
            if (extend == NULL) { return false; }
            fread(extend, fmt_chunk.size - (sizeof(fmt_chunk) - sizeof(fmt_chunk.id) - sizeof(fmt_chunk.size)), 1, file);
            free(extend);
        }
        else { fseek(file, data_chunk.size, SEEK_CUR); }
    }
    data_chunk.data = (double*)malloc(sizeof(double) * data_chunk.size / fmt_chunk.channel / fmt_chunk.bit * 8);
    if (data_chunk.data == NULL) { return false; }
    ZeroMemory(data_chunk.data, data_chunk.size / fmt_chunk.channel / fmt_chunk.bit * 8 + 1);
    unsigned int ch = fmt_chunk.channel;

    switch (fmt_chunk.bit) {
    case 8:
        if (data_chunk.size < sizeof(char)*2) { return false; }
        data_chunk.data8 = (char*)malloc(data_chunk.size);
        if (data_chunk.data8 == NULL) { return false; }
        data_chunk.arraysize = data_chunk.size / sizeof(char);
        fread(data_chunk.data8, sizeof(char), data_chunk.arraysize, file);

        for (unsigned int i = 0; i < data_chunk.arraysize; i++) {
            data_chunk.data[i / ch] = (double)(data_chunk.data8[i]) / 128.0;
        }
        free(data_chunk.data8);
        break;
    case 16:
        if (data_chunk.size < sizeof(short)*2) { return false; }
        data_chunk.data16 = (short*)malloc(data_chunk.size);
        if (data_chunk.data16 == NULL) { return false; }
        data_chunk.arraysize = data_chunk.size / sizeof(short);
        fread(data_chunk.data16, sizeof(short), data_chunk.arraysize, file);

        for (unsigned int i = 0; i < data_chunk.arraysize; i++) {
            data_chunk.data[i / ch] += (double)(data_chunk.data16[i]) / 32768.0;
        }
        free(data_chunk.data16);
        break;
    case 32:
        if (data_chunk.size < sizeof(double)) { return false; }
        data_chunk.data32 = (float*)malloc(data_chunk.size);
        if (data_chunk.data32 == NULL) { return false; }
        data_chunk.arraysize = data_chunk.size / sizeof(float);
        fread(data_chunk.data32, sizeof(float), data_chunk.arraysize, file);

        for (unsigned int i = 0; i < data_chunk.arraysize; i++) {
            data_chunk.data[i / ch] += (double)(data_chunk.data32[i]) / 3.4E+38;
        }
        free(data_chunk.data32);
        break;
    default:
        cout << "Error : This .wav file's bit is " << fmt_chunk.bit << " bit.";
        return false;
        break;

    }
    data_chunk.arraysize = data_chunk.arraysize / fmt_chunk.channel;
    fclose(file);
    return true;
}

/// <summary>
/// RIFFチャンクの中身を表示する
/// </summary>
void WAV_LOADER::Print_Riff() const {
    ofstream ofs("./wavlog2.txt");
    ofs << "------------------------------  RIFF  ------------------------------" << endl;
    ofs << std::hex << "id      =" << riff_chunk.id << std::endl;
    ofs << std::hex << "size    =" << riff_chunk.filesize << std::endl;
    ofs << std::hex << "type    =" << riff_chunk.type << std::endl;
    return;
}

/// <summary>
/// FMTチャンクの中身を表示する
/// </summary>
void WAV_LOADER::Print_Fmt() const {
    ofstream ofs("./wavlog3.txt");
    ofs << "------------------------------  FMT  ------------------------------" << endl;
    ofs << std::hex << "id      =" << fmt_chunk.id << std::endl;
    ofs << std::dec << "size    =" << fmt_chunk.size << std::endl;
    ofs << std::dec << "type    =" << fmt_chunk.type << std::endl;
    ofs << std::dec << "channel =" << fmt_chunk.channel << std::endl;
    ofs << std::dec << "sample  =" << fmt_chunk.sample << std::endl;
    ofs << std::dec << "byte    =" << fmt_chunk.byte << std::endl;
    ofs << std::dec << "block   =" << fmt_chunk.block << std::endl;
    ofs << std::dec << "bit     =" << fmt_chunk.bit << std::endl;
    return;
}

/// <summary>
/// DATAチャンクの中身を表示する
/// </summary>
void WAV_LOADER::Print_Data() const {
    ofstream ofs("./wavlog4.txt");
    ofs << "------------------------------  DATA  ------------------------------" << endl;
    ofs << std::hex << "id      =" << data_chunk.id << std::endl;
    ofs << std::dec << "size    =" << data_chunk.size << std::endl;
    return;
}

/// <summary>
/// DATAチャンクが使用しているメモリを解放する
/// </summary>
void WAV_LOADER::Clear() {
    free(data_chunk.data);
}

/// <summary>
/// 構造体を初期化する
/// </summary>
/// <param name="vector_size">処理回数(再生時間/単位時間)</param>
/// <param name="ut">単位時間(sec)</param>
/// <param name="d">難易度(0-3)</param>
void WAV_AUDIO::Init(unsigned int vector_size, double ut, int d) {
    lane_magnis         = vector<vector<double>>(vector_size, vector<double>(4, 0));    //LANE_NUM=4
    magnitude_class     = vector<unsigned long>(WAV_MAGNI_STEP + 1, 0);             
    magnis              = vector<vector<double>>(vector_size, vector<double>(MAX_FREQ+1, 0));
    hzs                 = vector<vector<unsigned int>>(vector_size, vector<unsigned int>(4, 0));
    switch (d)
    {
    case 0:
        difficulty = WAV_EASY;
        break;
    case 1:
        difficulty = WAV_NORMAL;
        break;
    case 2:
        difficulty = WAV_HARD;
        break;
    case 3:
        difficulty = WAV_VERYHARD;
        break;
    default:
        break;
    }
    magnitude_max = 0;
    magnitude_min = 1.0E+30;
    unit_time = (1.0E+6) * ut;  //  micro secに変換
}

/// <summary>
/// 周波数をレーンに変換する
/// </summary>
/// <param name="hz">周波数</param>
/// <returns>レーン(0-3)</returns>
int WAV_AUDIO::Decision_Lane(unsigned int hz) const {
    return min((int(hz) - 220) / 220, 3);
    /*
    if (hz < 440) { return 0; }
    if (hz < 660) { return 1; }
    if (hz < 880) { return 2; }
    return 3;
    */
}

/// <summary>
/// 直前の時間における対数パワースペクトルとの差をとる
/// </summary>
/// <param name="t">現在の処理の回数（経過時間/ut）</param>
/// <param name="spec">現在の時間におけるパワースペクトル</param>
void WAV_AUDIO::Get_Diff_LogSpec(unsigned int t, vector<double> spec) {
    unsigned int hz, lane;
    double magni, diff;
    for (hz = MIN_FREQ; hz <= MAX_FREQ; hz++) {
        if (spec[hz] == 0) { continue; }

        magni = log10(spec[hz]);                                                        //対数パワースペクトル
        lane = Decision_Lane(hz);

        if (t < 1) {
            magnis[0][hz] = magni;
            diff = magni;
        }
        else {
            if (magnis[t - 1][hz] < 0) { magnis[t - 1][hz] = 0; }

            magnis[t][hz] = magni;
            if (magni - magnis[t - 1][hz] < 0) { magnis[t - 1][hz] = magni; continue; }
            //        diff = magni * pow(magni - old_magnitude[hz], 2);
            diff = 1.0 * magni - magnis[t - 1][hz];
        }

        hzs[t][lane] = hz;
        if (magnitude_max < diff) { magnitude_max = diff; }
        if (magnitude_min > diff) { magnitude_min = diff; }
        if (lane_magnis[t][lane] < diff) { lane_magnis[t][lane] = diff; }               //レーン内で一番大きければ更新
    }
}

/// <summary>
/// lane_magnisからノーツを生成する
/// </summary>
/// <param name="notes_speed">ノーツの速さ</param>
void WAV_AUDIO::MakeNotes(float notes_speed) {
    //overlap:直前のノーツとの距離の最低保証
    unsigned int vector_size1 = lane_magnis.size(), vector_size2 = lane_magnis[0].size(), latest_notes[4] = { 0 }, overlap = (unsigned int)(NOTES_OVERLAP / unit_time * notes_speed);
    double border = 1.0, max_min = magnitude_max - magnitude_min, long_border = 7.50 * overlap;
    unsigned int sum = 0, notes_num = int(length * difficulty);
    int i;

    //  lane_magnisを正規化し、音が大きい順にn個のノーツを選択しやすくする
    for (unsigned int x = 0; x < vector_size1; x++) {
        for (unsigned j = 0; j < vector_size2; j++) {
            magnitude_class[int(WAV_MAGNI_STEP * lane_magnis[x][j] / max_min)]++;
        }
    }

    //  上から(notes_num)個程度のノーツを得られるようにborderを設定
    for (i = WAV_MAGNI_STEP; (sum < notes_num && 0 <= i); i--) { sum += magnitude_class[i]; }
    if (i < WAV_MAGNI_STEP) { border = max_min * double(i) / WAV_MAGNI_STEP; }

    for (unsigned int x = 0; x < vector_size1; x++) {
        for (unsigned int l = 0; l < vector_size2; l++) {
            if ((lane_magnis[x][l] > border) && x >= latest_notes[l] && x - latest_notes[l] >= overlap) {
                note* n = new note;
                n->is_beaten = false;
                n->lane = l;
                n->sec = unit_time * x;
                unsigned int note_len = 1, hz = hzs[x][l];
                double magni = lane_magnis[x][l];
                while (note_len + x < vector_size1) //  同一レーン内でのノーツの継続時間（overlapが存在しなかった場合に連続して現れるノーツの数）を調べる
                {
                    if      (abs(magnis[x + note_len][hz]     - magni) < max_min/4) { note_len++; }
                    else if (abs(magnis[x + note_len][hz + 1] - magni) < max_min/4) { note_len++; hz++; }
                    else if (abs(magnis[x + note_len][hz - 1] - magni) < max_min/4) { note_len++; hz--; }
                    else { break; }
                }
                if (note_len >= long_border) { n->length_sec = note_len * unit_time; latest_notes[l] = x + note_len; }  //  音が十分に連続していればロングノーツとする
                else { n->length_sec = 0; latest_notes[l] = x; }
                notes.push_back(n);
                
            }
        }
    }
}

/// <summary>
/// wavファイルからノーツを生成する
/// </summary>
/// <param name="filename">ファイル名</param>
/// <param name="notes_speed">ノーツの速度</param>
/// <param name="difficulty">難易度(0-3)</param>
/// <returns>ノーツ</returns>
vector<note*> wav2notes(char* filename, float notes_speed, int difficulty) {
    WAV_LOADER wavfile;
    wavfile.Load(filename);

    const unsigned int wavsize = wavfile.Get_Arraysize();
    unsigned int fps = min(8 * wavfile.Get_Byte() / wavfile.Get_Bit() / wavfile.Get_Channel() / 60, wavfile.Get_Samplerate() / 2); //   1/60(sec)

    static float* win;

    win = (float*)malloc(sizeof(float) * fps);
    if (win == NULL) { return vector<note*>(); }
    make_hamming_wintbl(fps, win);              //ハミング窓を生成

    WAV_AUDIO wavaudio;
    
    wavaudio.Init(int(wavsize / fps), 1.0 * wavfile.Get_Bit() * fps / wavfile.Get_Byte() / 8 * wavfile.Get_Channel(), difficulty);
    wavaudio.length = wavfile.Get_Datasize() / wavfile.Get_Byte();

    for (unsigned int i = 0; (i + 1) * fps < wavsize; i++)
    {
        vector<double> x;
        for (unsigned int j = 0; j < fps; j++)
        {
            x.push_back(double(win[j]) * wavfile.Get_Data(i * fps + j)); //  ハミング窓を適用
        }
        vector<complex<double>> fft_x = fft(x);                         //  フーリエ変換
        vector<double> result(MAX_FREQ + 1, 0);
        unsigned int hz = 0;

        for (unsigned int j = 0; j < fps; j++) {                        //  各周波数毎に纏める
            hz = int(0.5 + wavfile.Get_Samplerate() * j / fft_x.size());
            if (hz < MIN_FREQ) { continue; }
            if (hz > MAX_FREQ) { break; }
            result[hz] += abs(fft_x[j]);
        }

        wavaudio.Get_Diff_LogSpec(i, result);

        vector<double>().swap(x);
        vector<complex<double>>().swap(fft_x);
        vector<double>().swap(result);
    }
    free(win);
    wavaudio.MakeNotes(notes_speed);
    wavfile.Clear();

    return wavaudio.notes;
}

