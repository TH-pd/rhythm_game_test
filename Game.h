//
// Game.h
//

#pragma once
#pragma comment(lib,"winmm.lib")

#include <pch.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "StepTimer.h"
#include <mmsystem.h>
#include <iostream>
#include <fstream>
#include "Track.h"
#include <Keyboard.h>
#include <SpriteFont.h>
#include "Audiofiles.h"
#include "mid.h"
#include "wav.h"
#include <chrono>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#include <direct.h>

#define NOTES_SPEED 1.f         //  ノーツの速さ（1.0のときy座標が1msあたり1進む）
#define BORDER 700.f            //  判定のライン
#define LANE_NUM 4              //  レーン数
#define MAX_SCORE 1000000       //  最大スコア（の下限）
#define EFF_TIME 200000.f       //  エフェクトの表示時間（micro sec）
#define GRACE_TIME 3000000.f;   //  プレイ開始から曲が流れ始めるまでの待機時間(micro sec)

#define GREAT 100.f             //  この時間(ms)以内のズレであればGREATの判定となる
#define GOOD  200.f             //  この時間(ms)以内のズレであればGOODTの判定となる
#define MISS  250.f             //  この時間(ms)以内のズレであればMISSの判定となる（これ以上はノーツを叩いたことにならない）

#define JUDGMENT_GREAT 0        //  配列judgementの中のGREATの位置
#define JUDGMENT_GOOD  1        //  配列judgementの中のGOODの位置
#define JUDGMENT_MISS  2        //  配列judgementの中のMISSの位置

enum class GAMESTATE { SCR_TITLE, SCR_TRACK, SCR_PLAYING, SCR_RESULT, SCR_PAUSE, SCR_EDIT, SCR_CHART_SAVE, SCR_CONFIG, WAIT_KEY }; //現在の画面
enum class AUDIO_EXT { MID, WAV };                                                                          //音楽ファイルの拡張子

class KeyConfig {
public:
    vector<DirectX::Keyboard::Keys> keys;

    KeyConfig() { keys = vector<DirectX::Keyboard::Keys>(LANE_NUM + 6, (DirectX::Keyboard::Keys)0); }
    DirectX::Keyboard::Keys Get_Up() const { return keys[0]; };
    DirectX::Keyboard::Keys Get_Down() const { return keys[1]; };

    DirectX::Keyboard::Keys Get_Pause() const { return keys[2]; };
    DirectX::Keyboard::Keys Get_Enter() const { return keys[3]; };
    DirectX::Keyboard::Keys Get_Back() const { return keys[4]; };
    DirectX::Keyboard::Keys Get_Config() const { return keys[5]; };

    DirectX::Keyboard::Keys Get_Lane(unsigned int lane) const { return keys[6 + lane]; };

    void Set_Up(DirectX::Keyboard::Keys k) { keys[0] = k; };
    void Set_Down(DirectX::Keyboard::Keys k) { keys[1] = k; };

    void Set_Pause(DirectX::Keyboard::Keys k) { keys[2] = k; };
    void Set_Enter(DirectX::Keyboard::Keys k) { keys[3] = k; };
    void Set_Back(DirectX::Keyboard::Keys k) { keys[4] = k; };
    void Set_Config(DirectX::Keyboard::Keys k) { keys[5] = k; };

    void Set_Lane(unsigned int lane, DirectX::Keyboard::Keys k) { keys[6 + lane] = k; };

    bool Set_Key(unsigned int index, DirectX::Keyboard::Keys k);

};

// A basic game implementation that creates a D3D11 device and
// provides a game loop.

class Game
{
public:

    Game() noexcept;
    ~Game() = default;

    Game(Game&&) = default;
    Game& operator= (Game&&) = default;

    Game(Game const&) = delete;
    Game& operator= (Game const&) = delete;

    // Initialization and management
    void Initialize(HWND window, int width, int height);
    bool SetTilte();
    void Load_Config();
    void Save_Config();

    // Basic game loop
    void Tick();

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;


private:

    void Update();
    void EditUpdate();
    void TitleUpdate();
    void Select_TrackUpdate();
    void PauseUpdate();
    void ResultUpdate();
    void ConfigUpdate();
    void WaitKeyUpdate();

    void Render();
    void TitleRender();
    void TrackRender();
    void ResultRender();
    void ConfigRender();
    void WaitKeyRender();

    bool Load_Audiodata();
    bool Get_MidiTracks();
    bool Get_WavData();

    bool Select_File();
    bool Load_Wav();
    bool Load_Midi();

    bool SaveScore();
    bool LoadScore();
    bool SaveChart();
    bool LoadChart();

    bool Ready_Mci();
    bool Play_Mci();

    void Reset();

    void Clear();
    void Present();

    void CreateDevice();
    void CreateResources();

    void OnDeviceLost();



    GAMESTATE states;

    // Device resources.
    HWND                                            m_window;
    int                                             m_outputWidth;
    int                                             m_outputHeight;

    D3D_FEATURE_LEVEL                               m_featureLevel;
    Microsoft::WRL::ComPtr<ID3D11Device1>           m_d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext1>    m_d3dContext;

    Microsoft::WRL::ComPtr<IDXGISwapChain1>         m_swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  m_renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>  m_depthStencilView;

    // Rendering loop timer.
    DX::StepTimer                                   m_timer;

    wchar_t                                         difficulty_mode[4][1024] = { L"EASY", L"NORMAL", L"HARD", L"VERY HARD" };

    unsigned int                                     base_score;            //  基礎スコア
    unsigned int                                     combo_bonus;           //  コンボボーナス（10コンボ毎に基礎スコアに加算される）
    unsigned int                                     combo, max_combo;      //  現在のコンボ数, 最大コンボ数
    double                                           score;                 //  スコア（表示する時にはint型にキャストする）
    unsigned int                                     highscore;             //  ハイスコア
    vector<unsigned int>                             highscores;            //  各トラック/難易度のハイスコア
    wchar_t                                          judgment[3][8];        //  判定の文字列（GREAT, GOOD, MISS）を格納するための文字列
    unsigned int                                     judgment_result[3] = { 0,0,0 };         //  各判定の数
    int                                              is_long[4];            //  ロングノーツの途中ならばロングノーツの要素番号が入る

    //  notes
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> notes_texture, longnotes_texture;
    vector<DirectX::SimpleMath::Vector2*>            notes_pos;
    DirectX::SimpleMath::Vector2                     notes_origin;
    float                                            imgsize = 50.f;
    float                                            line_pos[4] = { 50.f - imgsize, 150.f - imgsize, 350.f - imgsize, 450.f - imgsize };

    //  effect
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> dramed_texture;
    vector<DirectX::SimpleMath::Vector2*>            eff_dram_pos;

    //  BG
    std::unique_ptr<DirectX::SpriteBatch>            m_spriteBatch;
    RECT* m_bgRect;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_bg;

    RECT* m_titleRect;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> title_texture;

    //  font
    std::unique_ptr<DirectX::SpriteFont>            m_font;
    std::unique_ptr<DirectX::SpriteFont>            judgment_font;
    DirectX::SimpleMath::Vector2                    m_ScorePos;

    vector<note*>                                   _notes;                                 //  ノーツ
    unsigned int                                    draw_min, draw_max;                     //  ノーツの描画範囲
    bool                                            is_long_dramed[LANE_NUM];               //  各レーンのロングノーツが1回以上叩かれたか
    float                                           dramed_lane[LANE_NUM] = { 0.f,0.f,0.f,0.f };   //  叩いたエフェクトの残り表示時間
    //Judgment_lane[i][0]=n.0, nは判定の文字列(judgment[n]), [i][1]=表示時間
    float                                           judgment_lane[4][2] = {};
    vector<DirectX::SimpleMath::Vector2*>           judgment_pos;                           //  判定の文字列を描画する座標
    vector< DirectX::XMVECTORF32>                   judgment_colors;                        //  

    std::unique_ptr<DirectX::Keyboard>              m_keyboard;
    DirectX::Keyboard::KeyboardStateTracker         m_keys;
    KeyConfig                                       keyconfig;                              //  キー割り当て
    unsigned int                                    keyconfig_num;                          //  どのキーの割り当てを変更するか
    vector<string>                                  keyconfig_name;                         //  キー割り当て画面の各キーの名前

    vector<string>                                  audiofiles;                             //  音楽ファイルを格納するための配列
    vector<string>                                  relative_audiofiles;                    //  音楽ファイルまでの相対パス
    vector<bool>                                    chart_exist;                            //  創作譜面の有無を格納する配列
    unsigned int                                    choosed_file = 0;                    //  現在選択しているaudiofilesの要素番号

    vector<int>                                     playable_tracks;                        //  トラック/難易度の一覧を格納する配列, 創作譜面の選択等もここに含まれる
    vector<int>                                     lane_notes_num;                         //  .midを選択した場合、各トラックに存在している音符の数を格納する配列
    unsigned int                                    choosed_track = 0;                    //  現在選択しているplayable_tracksの要素番号


    std::chrono::system_clock::time_point           start, before_time;                     //  楽曲を再生し始めた時間, 前回処理を行った時間
    std::chrono::system_clock::time_point           s_pause_time;                           //  一時停止を開始した時間
    double                                          paused_sumtime = 0;                    //  一時停止をしていた時間の合計
    bool                                            is_grace;                               //  プレイ開始から楽曲を再生する間の待機時間であるならtrue, 再生後はfalse
    double                                          grace_time = GRACE_TIME;           //  プレイ開始から楽曲を再生する間の待機時間, 再生後は0

    MCI_OPEN_PARMS                                  mop;
    double                                          mcilen = 0;                    //  再生中の楽曲の長さ(ms)
    double                                          midilen = 0;                    //  MCIでmidファイルを再生するとファイル内の再生時間よりも
    double                                          play_ratio = 1.0;                  //  高速で再生されることがあるので再生速度に合わせる（要改善）

    AUDIO_EXT                                       audiotype;                              //  選択した音楽ファイルの拡張子のタイプ
    TCHAR                                           choosed_audi_name[1024];                //  選択した音楽ファイルのファイル名（ウインドウタイトル等で使用する）
};
