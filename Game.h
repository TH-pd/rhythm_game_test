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

#define NOTES_SPEED 1.f         //  �m�[�c�̑����i1.0�̂Ƃ�y���W��1ms������1�i�ށj
#define BORDER 700.f            //  ����̃��C��
#define LANE_NUM 4              //  ���[����
#define MAX_SCORE 1000000       //  �ő�X�R�A�i�̉����j
#define EFF_TIME 200000.f       //  �G�t�F�N�g�̕\�����ԁimicro sec�j
#define GRACE_TIME 3000000.f;   //  �v���C�J�n����Ȃ�����n�߂�܂ł̑ҋ@����(micro sec)

#define GREAT 100.f             //  ���̎���(ms)�ȓ��̃Y���ł����GREAT�̔���ƂȂ�
#define GOOD  200.f             //  ���̎���(ms)�ȓ��̃Y���ł����GOODT�̔���ƂȂ�
#define MISS  250.f             //  ���̎���(ms)�ȓ��̃Y���ł����MISS�̔���ƂȂ�i����ȏ�̓m�[�c��@�������ƂɂȂ�Ȃ��j

#define JUDGMENT_GREAT 0        //  �z��judgement�̒���GREAT�̈ʒu
#define JUDGMENT_GOOD  1        //  �z��judgement�̒���GOOD�̈ʒu
#define JUDGMENT_MISS  2        //  �z��judgement�̒���MISS�̈ʒu

enum class GAMESTATE { SCR_TITLE, SCR_TRACK, SCR_PLAYING, SCR_RESULT, SCR_PAUSE, SCR_EDIT, SCR_CHART_SAVE, SCR_CONFIG, WAIT_KEY }; //���݂̉��
enum class AUDIO_EXT { MID, WAV };                                                                          //���y�t�@�C���̊g���q

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

    unsigned int                                     base_score;            //  ��b�X�R�A
    unsigned int                                     combo_bonus;           //  �R���{�{�[�i�X�i10�R���{���Ɋ�b�X�R�A�ɉ��Z�����j
    unsigned int                                     combo, max_combo;      //  ���݂̃R���{��, �ő�R���{��
    double                                           score;                 //  �X�R�A�i�\�����鎞�ɂ�int�^�ɃL���X�g����j
    unsigned int                                     highscore;             //  �n�C�X�R�A
    vector<unsigned int>                             highscores;            //  �e�g���b�N/��Փx�̃n�C�X�R�A
    wchar_t                                          judgment[3][8];        //  ����̕�����iGREAT, GOOD, MISS�j���i�[���邽�߂̕�����
    unsigned int                                     judgment_result[3] = { 0,0,0 };         //  �e����̐�
    int                                              is_long[4];            //  �����O�m�[�c�̓r���Ȃ�΃����O�m�[�c�̗v�f�ԍ�������

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

    vector<note*>                                   _notes;                                 //  �m�[�c
    unsigned int                                    draw_min, draw_max;                     //  �m�[�c�̕`��͈�
    bool                                            is_long_dramed[LANE_NUM];               //  �e���[���̃����O�m�[�c��1��ȏ�@���ꂽ��
    float                                           dramed_lane[LANE_NUM] = { 0.f,0.f,0.f,0.f };   //  �@�����G�t�F�N�g�̎c��\������
    //Judgment_lane[i][0]=n.0, n�͔���̕�����(judgment[n]), [i][1]=�\������
    float                                           judgment_lane[4][2] = {};
    vector<DirectX::SimpleMath::Vector2*>           judgment_pos;                           //  ����̕������`�悷����W
    vector< DirectX::XMVECTORF32>                   judgment_colors;                        //  

    std::unique_ptr<DirectX::Keyboard>              m_keyboard;
    DirectX::Keyboard::KeyboardStateTracker         m_keys;
    KeyConfig                                       keyconfig;                              //  �L�[���蓖��
    unsigned int                                    keyconfig_num;                          //  �ǂ̃L�[�̊��蓖�Ă�ύX���邩
    vector<string>                                  keyconfig_name;                         //  �L�[���蓖�ĉ�ʂ̊e�L�[�̖��O

    vector<string>                                  audiofiles;                             //  ���y�t�@�C�����i�[���邽�߂̔z��
    vector<string>                                  relative_audiofiles;                    //  ���y�t�@�C���܂ł̑��΃p�X
    vector<bool>                                    chart_exist;                            //  �n�안�ʂ̗L�����i�[����z��
    unsigned int                                    choosed_file = 0;                    //  ���ݑI�����Ă���audiofiles�̗v�f�ԍ�

    vector<int>                                     playable_tracks;                        //  �g���b�N/��Փx�̈ꗗ���i�[����z��, �n�안�ʂ̑I�𓙂������Ɋ܂܂��
    vector<int>                                     lane_notes_num;                         //  .mid��I�������ꍇ�A�e�g���b�N�ɑ��݂��Ă��鉹���̐����i�[����z��
    unsigned int                                    choosed_track = 0;                    //  ���ݑI�����Ă���playable_tracks�̗v�f�ԍ�


    std::chrono::system_clock::time_point           start, before_time;                     //  �y�Ȃ��Đ����n�߂�����, �O�񏈗����s��������
    std::chrono::system_clock::time_point           s_pause_time;                           //  �ꎞ��~���J�n��������
    double                                          paused_sumtime = 0;                    //  �ꎞ��~�����Ă������Ԃ̍��v
    bool                                            is_grace;                               //  �v���C�J�n����y�Ȃ��Đ�����Ԃ̑ҋ@���Ԃł���Ȃ�true, �Đ����false
    double                                          grace_time = GRACE_TIME;           //  �v���C�J�n����y�Ȃ��Đ�����Ԃ̑ҋ@����, �Đ����0

    MCI_OPEN_PARMS                                  mop;
    double                                          mcilen = 0;                    //  �Đ����̊y�Ȃ̒���(ms)
    double                                          midilen = 0;                    //  MCI��mid�t�@�C�����Đ�����ƃt�@�C�����̍Đ����Ԃ���
    double                                          play_ratio = 1.0;                  //  �����ōĐ�����邱�Ƃ�����̂ōĐ����x�ɍ��킹��i�v���P�j

    AUDIO_EXT                                       audiotype;                              //  �I���������y�t�@�C���̊g���q�̃^�C�v
    TCHAR                                           choosed_audi_name[1024];                //  �I���������y�t�@�C���̃t�@�C�����i�E�C���h�E�^�C�g�����Ŏg�p����j
};
