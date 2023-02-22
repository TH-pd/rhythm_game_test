//
// Game.cpp
//

#include "pch.h"
#include "Game.h"

extern void ExitGame() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

Game::Game() noexcept :
    m_window(nullptr),
    m_outputWidth(1000),
    m_outputHeight(800),
    m_featureLevel(D3D_FEATURE_LEVEL_9_1)
{
    m_bgRect = new RECT;
    m_titleRect = new RECT;
    mop = MCI_OPEN_PARMS();

    //ノーツの判定用の文字列
    swprintf_s(judgment[JUDGMENT_GREAT], 8, L"GREAT");
    swprintf_s(judgment[JUDGMENT_GOOD], 8, L"GOOD");
    swprintf_s(judgment[JUDGMENT_MISS], 8, L"MISS");

    //ノーツの判定を表示する座標
    for (int i = 0; i < LANE_NUM; i++) {
        judgment_lane[i][0] = judgment_lane[i][1] = 0.f;
        DirectX::SimpleMath::Vector2* p = new DirectX::SimpleMath::Vector2;
        p->x = 100.f * i + 50.f;
        p->y = 650.f;
        if (i >= LANE_NUM / 2) {
            p->x += 100;
        }
        judgment_pos.push_back(p);
    }
    judgment_colors = vector<DirectX::XMVECTORF32>(3, Colors::White);
    judgment_colors[JUDGMENT_GREAT] = Colors::HotPink;
    judgment_colors[JUDGMENT_GOOD] = Colors::DeepSkyBlue;
    judgment_colors[JUDGMENT_MISS] = Colors::LightBlue;
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
    m_window = window;
    m_outputWidth = std::max(width, 1);
    m_outputHeight = std::max(height, 1);

    CreateDevice();

    CreateResources();


    // TODO: Change the timer settings if you want something other than the default variable timestep mode.
    // e.g. for 60 FPS fixed timestep update logic, call:

    m_timer.SetFixedTimeStep(true);
    m_timer.SetTargetElapsedSeconds(1.0 / 60);  //60fps

    //キーボード
    m_keyboard = std::make_unique<Keyboard>();

    //ディレクトリが無ければ作成
    if (!PathIsDirectory(L"./MUSIC")) {
        if (_mkdir("./MUSIC")) {
            ofstream ofs("log.txt");
            ofs << "ERROR: Failed to create MUSIC directory" << endl;
            return;
        }
    }
    if (!PathIsDirectory(L"./CHART")) {
        if (_mkdir("./CHART")) {
            ofstream ofs("log.txt");
            ofs << "ERROR: Failed to create CHART directory" << endl;
            return;
        }
    }
    if (!PathIsDirectory(L"./DAT")) {
        if (_mkdir("./DAT")) {
            ofstream ofs("log.txt");
            ofs << "ERROR: Failed to create DAT directory" << endl;
            return;
        }
    }

    Reset();

    return;
}

// Executes the basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]() {
        switch (states) //現在のGAMESTATEに応じて、対応するUpdate関数を呼び出す
        {
        case GAMESTATE::SCR_TITLE:
            TitleUpdate();
            break;
        case GAMESTATE::SCR_TRACK:
            Select_TrackUpdate();
            break;
        case GAMESTATE::SCR_PLAYING:
            Update();
            break;
        case GAMESTATE::SCR_RESULT:
            ResultUpdate();
            break;
        case GAMESTATE::SCR_PAUSE:
            PauseUpdate();
            break;
        case GAMESTATE::SCR_EDIT:
            EditUpdate();
            break;
        case GAMESTATE::SCR_CONFIG:
            ConfigUpdate();
            break;
        case GAMESTATE::WAIT_KEY:
            WaitKeyUpdate();
            break;
        default:
            break;
        }
        });

    switch (states)     //現在のGAMESTATEに応じて、画面の描画を行う
    {
    case GAMESTATE::SCR_TITLE:
        TitleRender();
        break;
    case GAMESTATE::SCR_TRACK:
        TrackRender();
        break;
    case GAMESTATE::SCR_PLAYING:
    case GAMESTATE::SCR_PAUSE:
    case GAMESTATE::SCR_EDIT:
        Render();
        break;
    case GAMESTATE::SCR_RESULT:
        ResultRender();
        break;
    case GAMESTATE::SCR_CHART_SAVE:
        SaveChart();
        break;
    case GAMESTATE::SCR_CONFIG:
        ConfigRender();
        break;
    case GAMESTATE::WAIT_KEY:
        WaitKeyRender();
        break;
    default:
        break;
    }
}

// Updates the world.

//プレイ中のノーツの位置等の変数の更新、キー入力受付
//ノーツ   ：_notes
//スコア   :score, base_score, combo_bonus, highscore
//待機時間 :is_grace, grace_time
//MCI      :mop
//開始時刻 :start
void Game::Update()
{
    std::chrono::system_clock::time_point now_time = std::chrono::system_clock::now();    //現在時刻

    double totalTime = double(std::chrono::duration_cast<std::chrono::microseconds>(now_time - start).count()) - paused_sumtime;  //（現在位置）再生時間 = （スタート時からの経過時間） - （一時停止していた時間）
    double elapsedTime = double(std::chrono::duration_cast<std::chrono::microseconds>(now_time - before_time).count());             //前回の処理からの経過時間
    before_time = now_time;

    // TODO: Add your game logic here.
    unsigned int notes_num = notes_pos.size();
    auto kb = m_keyboard->GetState();
    m_keys.Update(kb);

    int nearest_notes[4] = { -1, -1, -1, -1 };                                                          //各レーンの叩ける位置にあるノーツの要素番号

    for (unsigned int i = draw_min; i < notes_num; i++) {
        notes_pos[i]->y = float(float(BORDER) - (_notes[i]->sec + grace_time - totalTime * play_ratio) * NOTES_SPEED / 1000.0); //現在のノーツの位置を更新、1000はマイクロ秒→ミリ秒への変換

        //ノーツが画面外へ出ている
        float long_y = notes_pos[i]->y - (float)(_notes[i]->length_sec * play_ratio * NOTES_SPEED / 1000); //  ノーツの終端の座標
        if (long_y > 800) {
            bool flg = true;
            for (unsigned int j = 0; j < LANE_NUM; j++) {
                flg &= (is_long[j] == -1);
            }
            if (flg) { draw_min = i; }                                           //次回から描画しない
            if ((unsigned int)is_long[_notes[i]->lane] == i && long_y > BORDER) { //ロングノーツの終端が通り過ぎた場合
                if (!is_long_dramed[_notes[i]->lane]) {
                    combo = 0;
                    judgment_lane[_notes[i]->lane][0] = JUDGMENT_MISS;
                    judgment_lane[_notes[i]->lane][1] = EFF_TIME;
                }
                is_long[_notes[i]->lane] = -1;
//                _notes[i]->is_beaten = true;
                is_long_dramed[_notes[i]->lane] = false;
            }
            else if (!_notes[i]->is_beaten && _notes[i]->length_sec == 0) {                            //叩いていないノーツならMISS判定
                combo = 0;
                judgment_lane[_notes[i]->lane][0] = JUDGMENT_MISS;
                judgment_lane[_notes[i]->lane][1] = EFF_TIME;
                _notes[i]->is_beaten = true;
                judgment_result[JUDGMENT_MISS]++;

            }
        }
        if (notes_pos[i]->y < -400) { draw_max = i + 1; break; }   //描画領域より上部であれば描画せず、ループを抜ける

        if (nearest_notes[_notes[i]->lane] == -1 && !_notes[i]->is_beaten && notes_pos[i]->y - BORDER < GREAT * NOTES_SPEED) {  //叩ける範囲内にノーツがあれば、最初に見つけたものをnearest_notesに格納
            nearest_notes[_notes[i]->lane] = i;
            if (_notes[i]->length_sec && is_long[_notes[i]->lane] == -1) {
                is_long[_notes[i]->lane] = i;
            }
        }
    }

    //keyconfig.Backキーが押されたら中断
    if (m_keys.IsKeyPressed(keyconfig.Get_Back()))
    {
        Reset();
        states = GAMESTATE::SCR_TITLE;
        return;
    }

    //keyconfig.PAUSEが押されたら一時停止
    if (m_keys.IsKeyPressed(keyconfig.Get_Pause()) && !is_grace) {
        s_pause_time = now_time;
        states = GAMESTATE::SCR_PAUSE;
        mciSendCommand(mop.wDeviceID, MCI_PAUSE, MCI_WAIT, 0);
        SetWindowText(m_window, L"Pause");
        return;
    }

    //各レーンについて
    for (int i = 0; i < LANE_NUM; i++) {
        if (dramed_lane[i] > 0) {   //叩いたエフェクトの時間を進める
            dramed_lane[i] -= float(elapsedTime);
        }
        if (judgment_lane[i][1] > 0) {  //判定の文字の表示時間を進める
            judgment_lane[i][1] -= float(elapsedTime);
        }

        //レーンに対応するキーが押されたら以下の処理を行う
        //・エフェクトの発生
        //・近くにあるノーツが叩けるなら叩く
        if (m_keys.IsKeyPressed(keyconfig.Get_Lane(i))) {
            dramed_lane[i] = EFF_TIME;
            if (nearest_notes[i] != -1 && _notes[nearest_notes[i]]->length_sec == 0) {
                float dist = BORDER - notes_pos[nearest_notes[i]]->y;
                if (dist < GREAT * NOTES_SPEED) {
                    combo++;
                    if (max_combo < combo) { max_combo = combo; }
                    score += double(base_score) + (double(combo) / 10) * combo_bonus;
                    _notes[nearest_notes[i]]->is_beaten = true;
                    judgment_lane[_notes[nearest_notes[i]]->lane][0] = JUDGMENT_GREAT;
                    judgment_lane[_notes[nearest_notes[i]]->lane][1] = EFF_TIME;
                    judgment_result[JUDGMENT_GREAT]++;
                }
                else if (dist < GOOD * NOTES_SPEED) {
                    combo++;
                    if (max_combo < combo) { max_combo = combo; }
                    score += (base_score + (combo / 10) * combo_bonus) / 2;
                    _notes[nearest_notes[i]]->is_beaten = true;
                    judgment_lane[_notes[nearest_notes[i]]->lane][0] = JUDGMENT_GOOD;
                    judgment_lane[_notes[nearest_notes[i]]->lane][1] = EFF_TIME;
                    judgment_result[JUDGMENT_GOOD]++;
                }
                else if (dist < MISS * NOTES_SPEED) {
                    combo = 0;
                    _notes[nearest_notes[i]]->is_beaten = true;
                    judgment_lane[_notes[nearest_notes[i]]->lane][0] = JUDGMENT_MISS;
                    judgment_lane[_notes[nearest_notes[i]]->lane][1] = EFF_TIME;
                    judgment_result[JUDGMENT_MISS]++;
                }
            }
            continue;
        }
        //  ロングノーツは押されている時間に応じてスコアを加算する
        if (kb.IsKeyDown(keyconfig.Get_Lane(i)) && is_long[i] != -1 && notes_pos[is_long[i]]->y >= BORDER) {
            //            float dist = BORDER - notes_pos[is_long[i]]->y;
            dramed_lane[i] = EFF_TIME;  //  ロングノーツがある場合はエフェクトを発生させる
            is_long_dramed[i] = true;
            score += (double(base_score) + (double(combo) / 10) * combo_bonus) * elapsedTime / NOTES_OVERLAP;    //  NOTES_OVERLAP(micro sec)あたり1combo分のスコアを加算する　コンボ数にはカウントしない
        }
    }

    //現在のスコアがハイスコアを超えているならハイスコアを更新
    if (highscore < score) {
        highscore = int(score);
    }

    //プレイ前の待機時間が終了するなら音楽ファイルを再生する
    if (is_grace && totalTime >= grace_time) {
        Play_Mci();
        grace_time = 0;
        is_grace = false;
    }

    //音楽ファイルの再生が終わったらリザルト画面へ
    if (mcilen < totalTime) {
        states = GAMESTATE::SCR_RESULT;
        highscores[choosed_track] = highscore;
        SaveScore();
        SetWindowText(m_window, L"Result");
    }

    return;
}

//ノーツ入力モード
void Game::EditUpdate()
{
    //float elapsedTime = static_cast<float>(timer.GetTotalSeconds());    

    std::chrono::system_clock::time_point now_time = std::chrono::system_clock::now();

    double totalTime = double(std::chrono::duration_cast<std::chrono::microseconds>(now_time - start).count());         //（現在位置）再生時間 = （スタート時からの経過時間）
    double elapsedTime = double(std::chrono::duration_cast<std::chrono::microseconds>(now_time - before_time).count()); //前回の処理からの経過時間
    before_time = now_time;


    // TODO: Add your game logic here.
    //elapsedTime;
    auto kb = m_keyboard->GetState();
    m_keys.Update(kb);

    //各レーンについて
    for (int i = 0; i < LANE_NUM; i++) {
        if (dramed_lane[i] > 0 && !is_grace) {  //叩いたエフェクトの表示時間を進める
            dramed_lane[i] -= float(elapsedTime);
        }
        if (kb.IsKeyDown(keyconfig.Get_Lane(i)) && is_long[i] != -1) {
            dramed_lane[i] = EFF_TIME;
            _notes[is_long[i]]->length_sec += elapsedTime;
        }
        if (m_keys.IsKeyPressed(keyconfig.Get_Lane(i))) {                   //キーが押されたらノーツを追加し、エフェクトを発生させる
            dramed_lane[i] = EFF_TIME;
            note* n = new note;
            n->lane = i;
            n->sec = totalTime;
            n->length_sec = 0;
            is_long[i] = _notes.size();
            _notes.push_back(n);
        }
    }

    //keyconfig.Backキーが押されたら中断
    if (m_keys.IsKeyPressed(keyconfig.Get_Back()))
    {
        Reset();
        states = GAMESTATE::SCR_TITLE;
        return;
    }

    //プレイ前の待機時間
    if (is_grace && totalTime >= grace_time) {
        Play_Mci();
        grace_time = 0;
        is_grace = false;
    }

    //再生が終了したらノーツを保存する
    if (mcilen < totalTime) {
        states = GAMESTATE::SCR_CHART_SAVE;
    }

    return;
}

/*
一時停止中
・keyconfig.Backキー　：中断
・keyconfig.PAUSEキー：再開
*/
void Game::PauseUpdate() {
    auto kb = m_keyboard->GetState();
    m_keys.Update(kb);
    std::chrono::system_clock::time_point now_time;

    if (m_keys.IsKeyPressed(keyconfig.Get_Back())) {
        Reset();
        states = GAMESTATE::SCR_TITLE;
        return;
    }
    if (m_keys.IsKeyPressed(keyconfig.Get_Pause())) {
        states = GAMESTATE::SCR_PLAYING;
        now_time = std::chrono::system_clock::now();
        paused_sumtime += double(std::chrono::duration_cast<std::chrono::microseconds>(now_time - s_pause_time).count());
        before_time = now_time;
        SetWindowText(m_window, choosed_audi_name);
        mciSendCommand(mop.wDeviceID, MCI_RESUME, MCI_WAIT, 0);
        return;
    }
}

//タイトル画面（曲選択画面）
void Game::TitleUpdate()
{
    auto kb = m_keyboard->GetState();
    m_keys.Update(kb);

    //  押されたキーに応じて状態遷移
    //  トラック/難易度選択画面へ移行
    if (m_keys.IsKeyPressed(keyconfig.Get_Enter()))
    {
        if (Select_File()) { states = GAMESTATE::SCR_TRACK; }
        return;
    }

    //  キー割り当て変更画面へ移行
    if (m_keys.IsKeyPressed(keyconfig.Get_Config())) { states = GAMESTATE::SCR_CONFIG; }

    //  終了
    if (m_keys.IsKeyPressed(keyconfig.Get_Back())) { ExitGame(); }

    //音楽ファイルを選択
    //上下は繋げてループさせる
    if (m_keys.IsKeyPressed(keyconfig.Get_Up())) {
        if (choosed_file == 0) {
            choosed_file = audiofiles.size() - 1;
        }
        else {
            choosed_file--;
        }
    }
    if (m_keys.IsKeyPressed(keyconfig.Get_Down())) {
        if (choosed_file == audiofiles.size() - 1) {
            choosed_file = 0;
        }
        else {
            choosed_file++;
        }
    }
    return;
}

//トラック/難易度選択画面
void Game::Select_TrackUpdate()
{
    auto kb = m_keyboard->GetState();
    m_keys.Update(kb);

    //Enterキーで決定
    //再生準備を行う
    if (m_keys.IsKeyPressed(keyconfig.Get_Enter()))
    {
        states = GAMESTATE::SCR_PLAYING;
        Load_Audiodata();
        return;
    }

    //keyconfig.Backキーで曲選択画面へ戻る
    if (m_keys.IsKeyPressed(keyconfig.Get_Back()))
    {
        states = GAMESTATE::SCR_TITLE;
        choosed_track = 0;
        return;
    }

    //上下カーソルキーで選択
    if (m_keys.IsKeyPressed(keyconfig.Get_Up())) {
        if (choosed_track == 0) {
            choosed_track = playable_tracks.size() - 1;
        }
        else {
            choosed_track--;
        }
    }
    if (m_keys.IsKeyPressed(keyconfig.Get_Down())) {
        if (choosed_track == playable_tracks.size() - 1) {
            choosed_track = 0;
        }
        else {
            choosed_track++;
        }
    }
    return;
}

// リザルト画面
// Enterキー又はkeyconfig.Backキーで曲選択画面に移行する
void Game::ResultUpdate() {
    auto kb = m_keyboard->GetState();
    m_keys.Update(kb);
    if (m_keys.IsKeyPressed(keyconfig.Get_Back()) || m_keys.IsKeyPressed(keyconfig.Get_Enter())) {
        Reset();
        states = GAMESTATE::SCR_TITLE;
        return;
    }
}

//キー割り当て画面
void Game::ConfigUpdate()
{
    auto kb = m_keyboard->GetState();
    m_keys.Update(kb);

    //  キー入力待ちへ移行
    if (m_keys.IsKeyPressed(keyconfig.Get_Enter())) { states = GAMESTATE::WAIT_KEY; }

    //  曲選択画面に戻る
    if (m_keys.IsKeyPressed(keyconfig.Get_Back())) { states = GAMESTATE::SCR_TITLE; }

    //上下カーソルキーで割り当てを変更するキーを選択
    //上下は繋げてループさせる
    if (m_keys.IsKeyPressed(keyconfig.Get_Up())) {
        if (keyconfig_num == 0) {
            keyconfig_num = keyconfig_name.size() - 1;
        }
        else {
            keyconfig_num--;
        }
    }
    if (m_keys.IsKeyPressed(keyconfig.Get_Down())) {
        if (keyconfig_num == keyconfig_name.size() - 1) {
            keyconfig_num = 0;
        }
        else {
            keyconfig_num++;
        }
    }
    return;
}

//キー割り当てを変更するキーの入力待ち画面
void Game::WaitKeyUpdate()
{
    auto kb = m_keyboard->GetState();
    m_keys.Update(kb);

    for (unsigned char i = 0; i < 0xff; i++) {
        DirectX::Keyboard::Keys key = (DirectX::Keyboard::Keys)i;
        if (m_keys.IsKeyPressed(key)) {
            if (keyconfig.Set_Key(keyconfig_num, key)) { Save_Config(); states = GAMESTATE::SCR_CONFIG; return; }
        }
    }
    return;
}

// Draws the scene.
// プレイ画面の描画
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    Clear();

    // TODO: Add your rendering code here.
    m_spriteBatch->Begin();
    m_spriteBatch->Draw(m_bg.Get(), *m_bgRect); //背景

    //  ノーツ入力モード・一時停止中でなければノーツを描画する
    if (states == GAMESTATE::SCR_PLAYING) {
        for (unsigned int i = draw_min; i < draw_max; i++) {
            if (!_notes[i]->is_beaten) {
                if (_notes[i]->length_sec == 0) {
                    m_spriteBatch->Draw(notes_texture.Get(), *(notes_pos)[i]);
                }
                else {
                    RECT longnoteRect;
                    longnoteRect.left = (LONG)notes_pos[i]->x;
                    longnoteRect.bottom = (LONG)notes_pos[i]->y;
                    longnoteRect.right = longnoteRect.left + 100;
                    longnoteRect.top = longnoteRect.bottom - (LONG)(_notes[i]->length_sec * play_ratio * NOTES_SPEED / 1000);
                    m_spriteBatch->Draw(longnotes_texture.Get(), longnoteRect, nullptr, Colors::White);
                }
            }
        }
    }

    // レーンが叩かれていればエフェクトを描画する
    for (int i = 0; i < LANE_NUM; i++)
    {
        if (dramed_lane[i] > 0) {
            m_spriteBatch->Draw(dramed_texture.Get(), *(eff_dram_pos)[i]);
        }
    }


    //ハイスコア等の文字の描画
    wchar_t str_score[1024];
    swprintf_s(str_score, 1024, L"High Score:%07d\nScore     :%07d\nCombo     :%d\n\n\nGREAT :%d\nGOOD  :%d\nMISS  :%d", highscore, int(score), combo, judgment_result[0], judgment_result[1], judgment_result[2]);

    DirectX::SimpleMath::Vector2 origin = m_font->MeasureString(str_score) / 2.f;

    m_font->DrawString(m_spriteBatch.get(), str_score, m_ScorePos, Colors::White, 0.f, origin, 1.f);

    // ノーツの判定の描画
    for (int i = 0; i < LANE_NUM; i++) {
        if (judgment_lane[i][1] > 0) {
            origin = judgment_font->MeasureString(judgment[int(judgment_lane[i][0])]) / 2.f;
            judgment_font->DrawString(m_spriteBatch.get(), judgment[int(judgment_lane[i][0])],
                *judgment_pos[i], judgment_colors[int(judgment_lane[i][0])], 0.f, origin, 0.6f);
        }
    }

    m_spriteBatch->End();

    Present();
}

// 曲選択画面の描画
void Game::TitleRender()
{
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }
    Clear();
    m_spriteBatch->Begin();
    m_spriteBatch->Draw(title_texture.Get(), *m_titleRect);
    DirectX::SimpleMath::Vector2 origin;
    DirectX::SimpleMath::Vector2 af_pos;    //文字列の座標
    int width, height;
    GetDefaultSize(width, height);
    float center_x = float(width) / 2, center_y = float(height) / 2;


    unsigned int af_size = audiofiles.size();
    // 現在選択中のファイルが画面の中心になり、画面上端/下端からはみ出た部分は繋げてループしているように描画する
    for (unsigned int i = 0; i < af_size; i++)
    {
        origin = m_font->MeasureString(audiofiles[i].c_str()) / 2.f;
        af_pos.x = center_x;
        af_pos.y = center_y + (float(i) - choosed_file) * 50;

        //  リストの先頭と末尾を繋げる
        if (af_pos.y < 50) {
            af_pos.y = center_y + (float(i + af_size) - choosed_file) * 50;
        }
        else if (af_pos.y > 750) {
            af_pos.y = center_y + (float(i) - choosed_file - af_size) * 50;
        }

        // 選択中の項目は色を変える
        DirectX::XMVECTORF32 color;
        if (i == choosed_file) {
            color = Colors::Orange;
        }
        else {
            color = Colors::White;
        }

        m_font->DrawString(m_spriteBatch.get(), audiofiles[i].c_str(),
            af_pos, color, 0.f, origin, 1.f);

    }
    m_spriteBatch->End();


    Present();
}

//  キー割り当て画面の描画
void Game::ConfigRender()
{
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }
    Clear();

    m_spriteBatch->Begin();
    m_spriteBatch->Draw(title_texture.Get(), *m_titleRect);
    DirectX::SimpleMath::Vector2 origin;
    DirectX::SimpleMath::Vector2 af_pos;    //文字列の座標
    int width, height;
    GetDefaultSize(width, height);
    float center_x = float(width) / 2, center_y = float(height) / 2;


    unsigned int af_size = keyconfig_name.size();
    // 現在選択中のファイルが画面の中心になり、画面上端/下端からはみ出た部分は繋げてループしているように描画する
    for (unsigned int i = 0; i < af_size; i++)
    {
        string str = keyconfig_name[i] + " : ";
        if ((char)keyconfig.keys[i] >= 'A' && (char)keyconfig.keys[i] <= 'Z') {
            str += (char)keyconfig.keys[i];
        }
        else {
            switch (keyconfig.keys[i])
            {
            case DirectX::Keyboard::Keys::Up:
                str += "UP";
                break;
            case DirectX::Keyboard::Keys::Down:
                str += "DOWN";
                break;
            case DirectX::Keyboard::Keys::Right:
                str += "RIGHT";
                break;
            case DirectX::Keyboard::Keys::Left:
                str += "LEFT";
                break;
            case DirectX::Keyboard::Keys::Enter:
                str += "ENTER";
                break;
            case DirectX::Keyboard::Keys::Space:
                str += "SPACE";
                break;
            case DirectX::Keyboard::Keys::Escape:
                str += "Esc";
                break;
            default:
                str += "Not Alphabet Key";
                break;
            }
        }
        origin = m_font->MeasureString(str.c_str()) / 2.f;
        af_pos.x = center_x;
        af_pos.y = center_y + (float(i) - keyconfig_num) * 50;

        //  リストの先頭と末尾を繋げる
        if (af_pos.y < 50) {
            af_pos.y = center_y + (float(i + af_size) - keyconfig_num) * 50;
        }
        else if (af_pos.y > 750) {
            af_pos.y = center_y + (float(i) - keyconfig_num - af_size) * 50;
        }

        // 選択中の項目は色を変える
        DirectX::XMVECTORF32 color;
        if (i == keyconfig_num) {
            color = Colors::Orange;
        }
        else {
            color = Colors::White;
        }

        m_font->DrawString(m_spriteBatch.get(), str.c_str(), af_pos, color, 0.f, origin, 1.f);

    }
    m_spriteBatch->End();


    Present();
}

// 難易度/トラック選択画面の描画
void Game::TrackRender()
{
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    Clear();

    m_spriteBatch->Begin();
    m_spriteBatch->Draw(title_texture.Get(), *m_titleRect);
    DirectX::SimpleMath::Vector2 origin;
    DirectX::SimpleMath::Vector2 af_pos;    //文字列の座標
    int width, height;
    GetDefaultSize(width, height);
    float center_x = float(width) / 2, center_y = float(height) / 2;


    unsigned int tr_size = playable_tracks.size(), cnt = 0;
    // 現在選択中のトラック/難易度が画面の中心になり、画面上端/下端からはみ出た部分は繋げてループしているように描画する
    for (unsigned int i = 0; i < tr_size; i++)
    {
        wchar_t str_track[1024];
        if (playable_tracks[i] == -1) {
            swprintf_s(str_track, 1024, L"Edit Chart");
        }
        else if (playable_tracks[i] == -2) {
            swprintf_s(str_track, 1024, L"Play original Chart (%u)", highscores[cnt++]);
        }
        else {
            switch (audiotype)
            {
            case AUDIO_EXT::MID:
                swprintf_s(str_track, 1024, L"Track%d : %d(notes) (%u)", playable_tracks[i], lane_notes_num[i], highscores[cnt++]);
                break;
            case AUDIO_EXT::WAV:
                swprintf_s(str_track, 1024, L"%s (%u)", difficulty_mode[i], highscores[cnt++]);
                break;
            default:
                swprintf_s(str_track, 1024, L"NO DATA");
                break;
            }
        }

        origin = m_font->MeasureString(str_track) / 2.f;

        af_pos.x = center_x;
        af_pos.y = center_y + (float(i) - choosed_track) * 50;

        //  リストの先頭と末尾を繋げる
        if (af_pos.y < 50) {
            af_pos.y = center_y + (float(i + tr_size) - choosed_track) * 50;
        }
        else if (af_pos.y > 750) {
            af_pos.y = center_y + (float(i) - choosed_track - tr_size) * 50;
        }

        // 選択中の項目は色を変える
        DirectX::XMVECTORF32 color;
        if (i == choosed_track) {
            color = Colors::Orange;
        }
        else {
            color = Colors::White;
        }

        m_font->DrawString(m_spriteBatch.get(), str_track,
            af_pos, color, 0.f, origin, 1.f);

    }
    m_spriteBatch->End();

    Present();
}

//  リザルト画面の描画
void Game::ResultRender()
{
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }
    Clear();
    m_spriteBatch->Begin();
    m_spriteBatch->Draw(title_texture.Get(), *m_titleRect);

    wchar_t str_result[1024];
    //  ハイスコアなら"HIGH SCORE!!!"と表示する
    if (highscore == score) {
        swprintf_s(str_result, 1024, L"%hs\n\n\nHIGH SCORE!!!\n\nHighScore:%u\nScore:%u\n\nMAX Combo:%d\n\nGREAT :%d\nGOOD :%d\nMISS :%d", relative_audiofiles[choosed_file].substr(8, relative_audiofiles[choosed_file].length() - 8).c_str(), highscore, int(score), max_combo, judgment_result[0], judgment_result[1], judgment_result[2]);
    }
    else {
        swprintf_s(str_result, 1024, L"%hs\n\n\nHighScore:%d\nScore:%u\n\nMAX Combo:%d\n\nGREAT :%d\nGOOD :%d\nMISS :%d", relative_audiofiles[choosed_file].substr(8, relative_audiofiles[choosed_file].length() - 8).c_str(), highscore, int(score), max_combo, judgment_result[0], judgment_result[1], judgment_result[2]);

    }

    DirectX::SimpleMath::Vector2 origin = m_font->MeasureString(str_result) / 2.f;
    DirectX::SimpleMath::Vector2 m_resultpos;

    m_resultpos.x = static_cast<UINT>(m_outputWidth) / 2.f;
    m_resultpos.y = static_cast<UINT>(m_outputHeight) / 2.f;

    m_font->DrawString(m_spriteBatch.get(), str_result, m_resultpos, Colors::White, 0.f, origin, 1.0f);

    m_spriteBatch->End();

    Present();
}

void Game::WaitKeyRender()
{
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }
    Clear();
    m_spriteBatch->Begin();
    m_spriteBatch->Draw(title_texture.Get(), *m_titleRect);
    DirectX::SimpleMath::Vector2 origin;
    DirectX::SimpleMath::Vector2 af_pos;    //文字列の座標
    int width, height;
    GetDefaultSize(width, height);
    float center_x = float(width) / 2, center_y = float(height) / 2;

    af_pos.x = center_x;
    af_pos.y = center_y;

    string str = "Please input " + keyconfig_name[keyconfig_num] + "'s key";

    origin = m_font->MeasureString(str.c_str()) / 2.f;
    m_font->DrawString(m_spriteBatch.get(), str.c_str(), af_pos, Colors::White, 0.f, origin, 1.f);

    m_spriteBatch->End();

    Present();
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    // Clear the views.
    m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), Colors::CornflowerBlue);
    m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    m_d3dContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

    // Set the viewport.
    CD3D11_VIEWPORT viewport(0.0f, 0.0f, static_cast<float>(m_outputWidth), static_cast<float>(m_outputHeight));
    m_d3dContext->RSSetViewports(1, &viewport);
}

// Presents the back buffer contents to the screen.
void Game::Present()
{
    // The first argument instructs DXGI to block until VSync, putting the application
    // to sleep until the next VSync. This ensures we don't waste any cycles rendering
    // frames that will never be displayed to the screen.
    HRESULT hr = m_swapChain->Present(1, 0);

    // If the device was reset we must completely reinitialize the renderer.
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        OnDeviceLost();
    }
    else
    {
        DX::ThrowIfFailed(hr);
    }
}

// Message handlers
void Game::OnActivated()
{
    // TODO: Game is becoming active window.
    m_keys.Reset();
}

void Game::OnDeactivated()
{
    // TODO: Game is becoming background window.
}

void Game::OnSuspending()
{
    // TODO: Game is being power-suspended (or minimized).
}

void Game::OnResuming()
{
    // TODO: Game is being power-resumed (or returning from minimize).
    m_timer.ResetElapsedTime();
    m_keys.Reset();
}

void Game::OnWindowSizeChanged(int width, int height)
{
    width;
    height;
    return;
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const noexcept
{
    // TODO: Change to desired default window size (note minimum size is 320x200).
    width = 1000;
    height = 800;
}

// These are the resources that depend on the device.
void Game::CreateDevice()
{
    UINT creationFlags = 0;

#ifdef _DEBUG
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    static const D3D_FEATURE_LEVEL featureLevels[] =
    {
        // TODO: Modify for supported Direct3D feature levels
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1,
    };

    // Create the DX11 API device object, and get a corresponding context.
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    DX::ThrowIfFailed(D3D11CreateDevice(
        nullptr,                            // specify nullptr to use the default adapter
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        creationFlags,
        featureLevels,
        static_cast<UINT>(std::size(featureLevels)),
        D3D11_SDK_VERSION,
        device.ReleaseAndGetAddressOf(),    // returns the Direct3D device created
        &m_featureLevel,                    // returns feature level of device created
        context.ReleaseAndGetAddressOf()    // returns the device immediate context
    ));

#ifndef NDEBUG
    ComPtr<ID3D11Debug> d3dDebug;
    if (SUCCEEDED(device.As(&d3dDebug)))
    {
        ComPtr<ID3D11InfoQueue> d3dInfoQueue;
        if (SUCCEEDED(d3dDebug.As(&d3dInfoQueue)))
        {
#ifdef _DEBUG
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
#endif
            D3D11_MESSAGE_ID hide[] =
            {
                D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
                // TODO: Add more message IDs here as needed.
            };
            D3D11_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = static_cast<UINT>(std::size(hide));
            filter.DenyList.pIDList = hide;
            d3dInfoQueue->AddStorageFilterEntries(&filter);
        }
    }
#endif

    DX::ThrowIfFailed(device.As(&m_d3dDevice));
    DX::ThrowIfFailed(context.As(&m_d3dContext));

    m_spriteBatch = std::make_unique<SpriteBatch>(m_d3dContext.Get());
    ComPtr<ID3D11Resource> notes_resource, longnotes_resource;
    ComPtr<ID3D11Resource> eff_dram;
    ComPtr<ID3D11Resource> title_resource;

    // TODO: Initialize device dependent objects here (independent of window size).
    //各種画像読み込み
    DX::ThrowIfFailed(
        CreateWICTextureFromFile(m_d3dDevice.Get(), L"./IMG/BG.png", nullptr,
            m_bg.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(
        CreateWICTextureFromFile(m_d3dDevice.Get(), L"./IMG/DRAM.png", eff_dram.GetAddressOf(), dramed_texture.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(
        CreateWICTextureFromFile(m_d3dDevice.Get(), L"./IMG/NOTE.png", notes_resource.GetAddressOf(), notes_texture.ReleaseAndGetAddressOf())
    );
    DX::ThrowIfFailed(
        CreateWICTextureFromFile(m_d3dDevice.Get(), L"./IMG/LONGNOTE.png", longnotes_resource.GetAddressOf(), longnotes_texture.ReleaseAndGetAddressOf())
    );
    DX::ThrowIfFailed(
        CreateWICTextureFromFile(m_d3dDevice.Get(), L"./IMG/TITLE.png", title_resource.GetAddressOf(), title_texture.ReleaseAndGetAddressOf())
    );
    ComPtr<ID3D11Texture2D> texture;
    DX::ThrowIfFailed(eff_dram.As(&texture));
    CD3D11_TEXTURE2D_DESC textureDesc;
    texture->GetDesc(&textureDesc);

    DX::ThrowIfFailed(notes_resource.As(&texture));
    texture->GetDesc(&textureDesc);

    DX::ThrowIfFailed(longnotes_resource.As(&texture));
    texture->GetDesc(&textureDesc);

    //  エフェクトの位置の設定
    for (int i = 0; i < LANE_NUM; i++) {
        DirectX::SimpleMath::Vector2* v = new DirectX::SimpleMath::Vector2;
        v->x = 100.f * i;
        v->y = 0.f;
        if (i >= LANE_NUM / 2) {
            v->x += 100;
        }
        eff_dram_pos.push_back(v);
    }

    //Load font
    m_font = std::make_unique<SpriteFont>(m_d3dDevice.Get(), L"./font/main.spritefont");
    judgment_font = std::make_unique<SpriteFont>(m_d3dDevice.Get(), L"./font/bold.spritefont");
    return;
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateResources()
{
    // Clear the previous window size specific context.
    ID3D11RenderTargetView* nullViews[] = { nullptr };
    m_d3dContext->OMSetRenderTargets(static_cast<UINT>(std::size(nullViews)), nullViews, nullptr);
    m_renderTargetView.Reset();
    m_depthStencilView.Reset();
    m_d3dContext->Flush();

    const UINT backBufferWidth = static_cast<UINT>(m_outputWidth);
    const UINT backBufferHeight = static_cast<UINT>(m_outputHeight);
    const DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    const DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    constexpr UINT backBufferCount = 2;

    // If the swap chain already exists, resize it, otherwise create one.
    if (m_swapChain)
    {
        HRESULT hr = m_swapChain->ResizeBuffers(backBufferCount, backBufferWidth, backBufferHeight, backBufferFormat, 0);

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            // If the device was removed for any reason, a new device and swap chain will need to be created.
            OnDeviceLost();

            // Everything is set up now. Do not continue execution of this method. OnDeviceLost will reenter this method 
            // and correctly set up the new device.
            return;
        }
        else
        {
            DX::ThrowIfFailed(hr);
        }
    }
    else
    {
        // First, retrieve the underlying DXGI Device from the D3D Device.
        ComPtr<IDXGIDevice1> dxgiDevice;
        DX::ThrowIfFailed(m_d3dDevice.As(&dxgiDevice));

        // Identify the physical adapter (GPU or card) this device is running on.
        ComPtr<IDXGIAdapter> dxgiAdapter;
        DX::ThrowIfFailed(dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf()));

        // And obtain the factory object that created it.
        ComPtr<IDXGIFactory2> dxgiFactory;
        DX::ThrowIfFailed(dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.GetAddressOf())));

        // Create a descriptor for the swap chain.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = backBufferWidth;
        swapChainDesc.Height = backBufferHeight;
        swapChainDesc.Format = backBufferFormat;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = backBufferCount;

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
        fsSwapChainDesc.Windowed = TRUE;

        // Create a SwapChain from a Win32 window.
        DX::ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(
            m_d3dDevice.Get(),
            m_window,
            &swapChainDesc,
            &fsSwapChainDesc,
            nullptr,
            m_swapChain.ReleaseAndGetAddressOf()
        ));

        // This template does not support exclusive fullscreen mode and prevents DXGI from responding to the ALT+ENTER shortcut.
        DX::ThrowIfFailed(dxgiFactory->MakeWindowAssociation(m_window, DXGI_MWA_NO_ALT_ENTER));
    }

    // Obtain the backbuffer for this window which will be the final 3D rendertarget.
    ComPtr<ID3D11Texture2D> backBuffer;
    DX::ThrowIfFailed(m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf())));

    // Create a view interface on the rendertarget to use on bind.
    DX::ThrowIfFailed(m_d3dDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, m_renderTargetView.ReleaseAndGetAddressOf()));

    // Allocate a 2-D surface as the depth/stencil buffer and
    // create a DepthStencil view on this surface to use on bind.
    CD3D11_TEXTURE2D_DESC depthStencilDesc(depthBufferFormat, backBufferWidth, backBufferHeight, 1, 1, D3D11_BIND_DEPTH_STENCIL);

    ComPtr<ID3D11Texture2D> depthStencil;
    DX::ThrowIfFailed(m_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, depthStencil.GetAddressOf()));

    CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
    DX::ThrowIfFailed(m_d3dDevice->CreateDepthStencilView(depthStencil.Get(), &depthStencilViewDesc, m_depthStencilView.ReleaseAndGetAddressOf()));

    // TODO: Initialize windows-size dependent objects here.
    //  背景の座標
    m_bgRect->left = 0;
    m_bgRect->top = 0;
    m_bgRect->right = backBufferWidth;
    m_bgRect->bottom = backBufferHeight;

    m_titleRect->left = 0;
    m_titleRect->top = 0;
    m_titleRect->right = backBufferWidth;
    m_titleRect->bottom = backBufferHeight;

    //  ゲーム中のスコアの座標
    m_ScorePos.x = 3.f * backBufferWidth / 4.f;
    m_ScorePos.y = backBufferHeight / 2.f;
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.

    m_depthStencilView.Reset();
    m_renderTargetView.Reset();
    m_swapChain.Reset();
    m_d3dContext.Reset();
    m_d3dDevice.Reset();

    CreateDevice();

    CreateResources();

    notes_pos.clear();
    eff_dram_pos.clear();
    dramed_texture.Reset();
    notes_texture.Reset();
    title_texture.Reset();

    judgment_pos.clear();

    //    m_texture.Reset();
    m_spriteBatch.reset();

    m_bg.Reset();

    m_font.reset();
}

//  音楽ファイル一覧の取得
bool Game::SetTilte() {
    states = GAMESTATE::SCR_TITLE;

    vector<string> abs_chartfiles;

    if (!getFileNames("./Music", relative_audiofiles) || relative_audiofiles.size() == 0) { return false; };  //音楽ファイル一覧
    if (!getFileNames("./CHART", abs_chartfiles)) { return false; };                                //創作譜面一覧

    //  音楽ファイル一覧から、ディレクトリと拡張子を消したファイル名一覧を得る（曲選択画面で使用）
    for (unsigned int i = 0; i < relative_audiofiles.size();) {
        string ext = relative_audiofiles[i].substr(relative_audiofiles[i].length() - 4);
        if (ext != ".mid" && ext != ".wav") { relative_audiofiles.erase(relative_audiofiles.begin() + i); continue; }
        audiofiles.push_back(relative_audiofiles[i].substr(8, relative_audiofiles[i].length() - 12));
        i++;
    }

    chart_exist = vector<bool>(audiofiles.size(), false);   //創作譜面の有無を初期化
    //創作譜面のファイル名（拡張子抜き）と一致する音楽ファイルがあれば、創作譜面有りとする
    for (unsigned int i = 0; i < abs_chartfiles.size(); i++) {
        string ext = abs_chartfiles[i].substr(abs_chartfiles[i].length() - 4);
        if (ext == ".chr") {
            string chrfile = abs_chartfiles[i].substr(8, abs_chartfiles[i].length() - 12);
            for (unsigned int j = 0; j < relative_audiofiles.size(); j++) {
                string audiofile = relative_audiofiles[j].substr(8, relative_audiofiles[j].length() - 8);
                if (chrfile == audiofile) {
                    chart_exist[j] = true;
                }
            }
        }
    }
    return true;
}

//  choosed_file:選択された曲の要素番号
//  midiならノーツが存在するトラックの一覧を、
//  wavなら4段階の難易度をplayable_tracksに格納する
//  また、譜面作成モードと、譜面があれば創作譜面もplayable_tracksに追加する
//  ハイスコアのロードもここで行う
bool Game::Select_File() {
    string ext = relative_audiofiles[choosed_file].substr(relative_audiofiles[choosed_file].length() - 4);    //拡張子
    bool flg = false;

    //  ファイル名の型変換
    const int fn_len = relative_audiofiles[choosed_file].size() + 1;
    char* c_filename = new char[fn_len];

    strcpy_s(c_filename, fn_len, relative_audiofiles[choosed_file].substr(8, relative_audiofiles[choosed_file].length() - 8).c_str());
    ZeroMemory(&choosed_audi_name[0], strlen(c_filename) + 1);
    MultiByteToWideChar(CP_ACP, 0, c_filename, fn_len, &choosed_audi_name[0], fn_len);

    SetWindowText(m_window, choosed_audi_name); //  ウインドウタイトルを音楽ファイル名に変更

    //  各リストをリセット
    lane_notes_num.resize(0);
    playable_tracks.resize(0);
    lane_notes_num.shrink_to_fit();
    playable_tracks.shrink_to_fit();

    if (ext == ".mid") { audiotype = AUDIO_EXT::MID; flg = Get_MidiTracks(); }
    if (ext == ".wav") { audiotype = AUDIO_EXT::WAV; flg = Get_WavData(); }

    if (chart_exist[choosed_file]) {    //  創作譜面があればリストに追加
        playable_tracks.push_back(-2);
    }
    playable_tracks.push_back(-1);      //  譜面作成モード
    LoadScore();                        //  ハイスコアのロード
    return flg;
}

//  ノーツの読み込み、スコアの算出、タイマーのセットを行う
bool Game::Load_Audiodata() {
    score = 0.0;
    combo = max_combo = 0;  //  初期化
    judgment_result[0] = judgment_result[1] = judgment_result[2] = 0;
    is_grace = true;
    grace_time = GRACE_TIME;
    for (unsigned int i = 0; i < LANE_NUM; i++) {   //  ロングノーツのid
        is_long[i] = -1;
    }

    if (playable_tracks[choosed_track] == -1) { //  譜面作成モードならノーツを生成しない
        Ready_Mci();
        states = GAMESTATE::SCR_EDIT;
        start = std::chrono::system_clock::now();
        before_time = start;
        return true;
    }

    bool flg = false;
    if (playable_tracks[choosed_track] == -2) { //  創作譜面なら読み込み
        flg = LoadChart();
    }
    else {                                      //  ファイル形式に応じて読み込み
        switch (audiotype)
        {
        case AUDIO_EXT::MID:
            flg = Load_Midi();
            break;
        case AUDIO_EXT::WAV:
            flg = Load_Wav();
            break;
        default:
            break;
        }
    }

    Ready_Mci();                               //  mciの準備

    grace_time = GRACE_TIME;

    draw_max = _notes.size();
    draw_min = 0;
    unsigned int notes_num = 0;

    for (unsigned int i = 0; i < draw_max; i++) {   //  ノーツの初期座標
        DirectX::SimpleMath::Vector2* p = new DirectX::SimpleMath::Vector2;
        p->x = line_pos[_notes[i]->lane];
        p->y = float(BORDER - NOTES_SPEED * (_notes[i]->sec + grace_time));
        notes_pos.push_back(p);
        _notes[i]->is_beaten = false;

        if (_notes[i]->length_sec == 0) {
            notes_num++;
        }
        else {
            notes_num += (unsigned int)(_notes[i]->length_sec / NOTES_OVERLAP / 2);     //  ロングノーツの場合は長さに応じた数のノーツとしてカウント
        }
    }

    //  スコアは(base_score + int(combo_bonus/10))とし、MAX_SCORE-MAX_SCORE*2程度とする
    //  ただし、base_score = 4*combo_bonusとする
    int step = notes_num / 10;   //  公差 int(combo_bonus/10)
    combo_bonus = int(MAX_SCORE / (40 + 5 * (step * (step + 1))));
    base_score = 4 * combo_bonus;
    for (unsigned int i = 0; i < LANE_NUM; i++) { is_long_dramed[i] = false; }

    highscore = highscores[choosed_track];

    start = std::chrono::system_clock::now();   //タイマーをセット
    before_time = start;

    return flg;
}

//  wavファイルからノーツを生成する
//  choosed_file :選択したファイル
//  choosed_track:難易度
bool Game::Load_Wav() {
    const int fn_len = relative_audiofiles[choosed_file].size() + 1;
    char* c_filename = new char[fn_len];

    strcpy_s(c_filename, fn_len, relative_audiofiles[choosed_file].c_str());
    _notes = wav2notes(c_filename, NOTES_SPEED, playable_tracks[choosed_track]);
    if (_notes.size() == 0) { return false; }

    delete[] c_filename;

    return true;
}

//  midファイルからノーツを生成する
//  choosed_file :選択したファイル
//  playable_tracks[choosed_track]:トラック番号
bool Game::Load_Midi() {

    const int fn_len = relative_audiofiles[choosed_file].size() + 1;
    char* c_filename = new char[fn_len];
    AUDIODATA audiodata;

    strcpy_s(c_filename, fn_len, relative_audiofiles[choosed_file].c_str());

    if (audiodata.Read_Mid(c_filename, playable_tracks[choosed_track]) == -1) { return false; }
    delete[] c_filename;
    audiodata.Decision_Lane();
    audiodata.Get_sec(float(NOTES_SPEED));

    _notes = audiodata.Get_Notes();
    midilen = audiodata.TotalTime();

    return true;
}

//  MCIでファイルをOPENし、
//  mcilenに再生時間を格納する
bool Game::Ready_Mci() {
    const int fn_len = relative_audiofiles[choosed_file].size() + 1;
    char* c_filename = new char[fn_len];
    TCHAR* t_filename = new TCHAR[fn_len];

    strcpy_s(c_filename, fn_len, relative_audiofiles[choosed_file].c_str());
    ZeroMemory(&t_filename[0], strlen(c_filename) + 1);
    MultiByteToWideChar(CP_ACP, 0, c_filename, fn_len, &t_filename[0], fn_len);

    delete[] c_filename;

    switch (audiotype)
    {
    case AUDIO_EXT::MID:
        mop.lpstrDeviceType = (LPWSTR)MCI_DEVTYPE_SEQUENCER;
        break;
    case AUDIO_EXT::WAV:
        mop.lpstrDeviceType = (LPWSTR)MCI_DEVTYPE_WAVEFORM_AUDIO;
        break;
    default:
        break;
    }

    //  OPEN
    mop.lpstrElementName = (LPCWSTR)(t_filename);
    int result = mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID | MCI_OPEN_ELEMENT | MCI_WAIT, (DWORD)&mop);
    if (result) { return false; }

    MCI_SET_PARMS  mciset;
    MCI_STATUS_PARMS msp;

    mciset.dwTimeFormat = MCI_FORMAT_MILLISECONDS;
    msp.dwItem = MCI_STATUS_LENGTH;
    mciSendCommand(mop.wDeviceID, MCI_SET, MCI_WAIT | MCI_SET_TIME_FORMAT, (DWORD)&mciset); //  時間の単位をmsに設定
    mciSendCommand(mop.wDeviceID, MCI_STATUS, MCI_WAIT | MCI_STATUS_ITEM, (DWORD)&msp);     //  再生時間を取得

    mcilen = (double)msp.dwReturn * 1000.0;

    if (audiotype == AUDIO_EXT::MID) { play_ratio = max(midilen / mcilen, 1.0); } //  midファイルなら内部タイマーの進行速度をmciに合わせる
    else { play_ratio = 1.0; }

    return true;
}

//  MCIで現在開いているファイルを再生する
bool Game::Play_Mci() {
    mciSendCommand(mop.wDeviceID, MCI_PLAY | MCI_WAIT, 0, 0);
    paused_sumtime = 0;
    start = std::chrono::system_clock::now();
    before_time = start;
    return true;
}

//  choosed_fileで指定されたmidファイルの各トラックの音符の数を調べる
//  トラックの音符の数が0でなければplayable_tracksにトラック番号を追加し、
//  更にlane_notes_numにその数を追加する
bool Game::Get_MidiTracks() {
    const int fn_len = relative_audiofiles[choosed_file].size() + 1;
    char* c_filename = new char[fn_len];
    bool flg = false;
    AUDIODATA audiodata;

    strcpy_s(c_filename, fn_len, relative_audiofiles[choosed_file].c_str());

    vector<int> v_notes = audiodata.Get_Tracknum(c_filename);

    for (unsigned int i = 0; i < v_notes.size(); i++) {
        if (v_notes[i] > 0) {                           //  トラックの音符の数　>　0
            lane_notes_num.push_back(v_notes[i]);
            playable_tracks.push_back(i);
            flg = true;
        }
    }
    delete[] c_filename;
    choosed_track = 0;
    return flg;
}

//  wavの難易度を0-3の4段階とし、playable_tracksに格納する
bool Game::Get_WavData() {
    for (unsigned int i = 0; i < 4; i++) {
        playable_tracks.push_back(i);
        lane_notes_num.push_back(0);
    }
    return true;
}

//  作成した譜面(_notes)を保存する
bool Game::SaveChart() {
    const int fn_len = relative_audiofiles[choosed_file].size() + 1;
    char* fn = new char[fn_len];
    char* filename = new char[fn_len + 20];
    FILE* fp;

    strcpy_s(fn, fn_len, relative_audiofiles[choosed_file].substr(8, relative_audiofiles[choosed_file].length() - 8).c_str());

    sprintf_s(filename, fn_len + 20, "./CHART/%s.chr", fn); //  ファイル名は元の音楽ファイルの名前.chr

    if (fopen_s(&fp, filename, "wb")) { return false; }

    size_t size = _notes.size();
    fwrite(&size, sizeof(size_t), 1, fp);

    for (unsigned int i = 0; i < size; i++) {
        if (_notes[i]->length_sec < 5.0 * NOTES_OVERLAP) { _notes[i]->length_sec = 0; }
        fwrite((&_notes[i]->lane), sizeof(int), 1, fp);
        fwrite((&_notes[i]->sec), sizeof(double), 1, fp);
        fwrite((&_notes[i]->length_sec), sizeof(double), 1, fp);
    }
    fclose(fp);

    if (!chart_exist[choosed_file]) {   //  作成した譜面のハイスコアを0で初期化
        highscores.push_back(0);
    }
    else {
        highscores[highscores.size() - 1] = 0;
    }
    SaveScore();
    chart_exist[choosed_file] = true;   //  次回以降、譜面が使用できるようにchart_existを更新
    Reset();
    states = GAMESTATE::SCR_TITLE;      //  曲選択画面へ
    return true;
}

//  創作譜面のロード
bool Game::LoadChart() {
    const int fn_len = relative_audiofiles[choosed_file].size() + 1;
    char* fn = new char[fn_len];
    char* filename = new char[fn_len + 20];
    FILE* fp;

    strcpy_s(fn, fn_len, relative_audiofiles[choosed_file].substr(8, relative_audiofiles[choosed_file].length() - 8).c_str());

    sprintf_s(filename, fn_len + 20, "./CHART/%s.chr", fn); //  ファイル名は元の音楽ファイルの名前.chr

    if (fopen_s(&fp, filename, "rb")) { return false; }

    size_t size;
    fread_s(&size, sizeof(size_t), sizeof(size_t), 1, fp);

    for (unsigned int i = 0; i < size; i++) {
        note* n = new note;
        fread_s(&(n->lane), sizeof(int), sizeof(int), 1, fp);
        fread_s(&(n->sec), sizeof(double), sizeof(double), 1, fp);
        fread_s(&(n->length_sec), sizeof(double), sizeof(double), 1, fp);
        n->is_beaten = false;
        _notes.push_back(n);
    }
    fclose(fp);
    return true;
}

//  ハイスコア(highscores)の保存
bool Game::SaveScore() {
    const int fn_len = relative_audiofiles[choosed_file].size() + 1;
    char* fn = new char[fn_len];
    char* filename = new char[fn_len + 20];
    FILE* fp;

    strcpy_s(fn, fn_len, relative_audiofiles[choosed_file].substr(8, relative_audiofiles[choosed_file].length() - 8).c_str());

    sprintf_s(filename, fn_len + 20, "./DAT/%s.score", fn); //  ファイル名は元の音楽ファイルの名前.score

    if (fopen_s(&fp, filename, "wb")) { return false; }
    size_t size = highscores.size();
    fwrite(&size, sizeof(size_t), 1, fp);

    for (unsigned int i = 0; i < size; i++) {
        fwrite((&highscores[i]), sizeof(int), 1, fp);
    }
    fclose(fp);
    return true;
}

//  ハイスコアのロード
bool Game::LoadScore() {
    const int fn_len = relative_audiofiles[choosed_file].size() + 1;
    char* fn = new char[fn_len];
    char* filename = new char[fn_len + 20];
    FILE* fp;

    strcpy_s(fn, fn_len, relative_audiofiles[choosed_file].substr(8, relative_audiofiles[choosed_file].length() - 8).c_str());

    sprintf_s(filename, fn_len + 20, "./DAT/%s.score", fn); //  ファイル名は元の音楽ファイルの名前.score

    if (fopen_s(&fp, filename, "rb")) {                     //  scoreファイルが無ければ、全てのトラック/難易度のハイスコアを0とする
        if (fopen_s(&fp, filename, "wb")) { return false; }

        size_t size = playable_tracks.size() - 1;
        highscores = vector<unsigned int>(size, 0);

        fwrite(&size, sizeof(size_t), 1, fp);
        for (unsigned int i = 0; i < size; i++) {
            fwrite(&(highscores[i]), sizeof(unsigned int), 1, fp);
        }
        fclose(fp);
        return true;
    }

    size_t size;
    fread_s(&size, sizeof(size_t), sizeof(size_t), 1, fp);
    if (size != playable_tracks.size() - 1) {                               //  ハイスコアの数が一致しない場合（音楽ファイルが差し替えられている場合など）
        highscores = vector<unsigned int>(playable_tracks.size() - 1, 0);   //  ハイスコアをリセット
        fclose(fp);
        return true;
    }
    highscores = vector<unsigned int>(size, 0);
    for (unsigned int i = 0; i < size; i++) {
        fread_s(&(highscores[i]), sizeof(unsigned int), sizeof(unsigned int), 1, fp);
    }
    fclose(fp);
    return true;
}

void Game::Save_Config() {
    FILE* fp;

    if (fopen_s(&fp, "./keyconfig.dat", "wb")) { return; }

    size_t size = keyconfig.keys.size();
    fwrite(&size, sizeof(size_t), 1, fp);

    for (unsigned int i = 0; i < size; i++) {
        fwrite((&keyconfig.keys[i]), sizeof(DirectX::Keyboard::Keys), 1, fp);
    }
    fclose(fp);

    return;
}

//  キー割り当てのロード
void Game::Load_Config() {
    FILE* fp;
    bool can_read = true;

    if (fopen_s(&fp, "./keyconfig.dat", "rb")) {                     //  キーコンフィグファイルが無ければデフォルト
        can_read = false;
    }
    else {
        size_t size;
        fread_s(&size, sizeof(size_t), sizeof(size_t), 1, fp);
        if (size != keyconfig.keys.size()) {                               //  ハイスコアの数が一致しない場合（音楽ファイルが差し替えられている場合など）
            can_read = false;
        }
        else {
            for (unsigned int i = 0; i < size; i++) {
                fread_s(&(keyconfig.keys[i]), sizeof(DirectX::Keyboard::Keys), sizeof(DirectX::Keyboard::Keys), 1, fp);
            }
        }
        fclose(fp);
    }

    if (!can_read) {
        keyconfig.Set_Up(DirectX::Keyboard::Keys::Up);
        keyconfig.Set_Down(DirectX::Keyboard::Keys::Down);

        keyconfig.Set_Lane(0, DirectX::Keyboard::Keys::A);
        keyconfig.Set_Lane(1, DirectX::Keyboard::Keys::S);
        keyconfig.Set_Lane(2, DirectX::Keyboard::Keys::K);
        keyconfig.Set_Lane(3, DirectX::Keyboard::Keys::L);

        keyconfig.Set_Pause(DirectX::Keyboard::Keys::Space);
        keyconfig.Set_Enter(DirectX::Keyboard::Keys::Enter);
        keyconfig.Set_Back(DirectX::Keyboard::Keys::Escape);
        keyconfig.Set_Config(DirectX::Keyboard::Keys::C);
    }
    keyconfig_name.clear();

    keyconfig_name.push_back("Up");
    keyconfig_name.push_back("Down");

    keyconfig_name.push_back("Pause");
    keyconfig_name.push_back("Enter");
    keyconfig_name.push_back("Back/ShutDown");
    keyconfig_name.push_back("Config");

    keyconfig_name.push_back("LANE1");
    keyconfig_name.push_back("LANE2");
    keyconfig_name.push_back("LANE3");
    keyconfig_name.push_back("LANE4");

    return;
}

//  原則、曲選択画面に移行する際に呼び出す
//  ゲームをプレイする際に使用するvector _notes, notes_pos及びmopを初期化する
void Game::Reset() {
    mciSendCommand(mop.wDeviceID, MCI_CLOSE, 0, 0);
    for (unsigned int i = 0; i < _notes.size(); i++) {
        delete[] _notes[i];
    }
    for (unsigned int i = 0; i < notes_pos.size(); i++) {
        delete[] notes_pos[i];
    }
    _notes.clear();
    _notes.shrink_to_fit();
    notes_pos.clear();
    notes_pos.shrink_to_fit();
    SetWindowText(m_window, L"TITLE");
}

/// <summary>
/// キー割り当てを変更する
/// 同じキーが同時に使用する可能性がある場所に登録されていた場合はfalseが返る
/// </summary>
/// <param name="index">keysの要素番号</param>
/// <param name="k">キー</param>
/// <returns>正常に割り当てを変更できたらtrueが返る</returns>
bool KeyConfig::Set_Key(unsigned int index, DirectX::Keyboard::Keys k) {
    if (index >= keys.size()) { return false; } //  不正な要素番号
    bool same_key = false;
    ofstream ofs("log_.txt");
    switch (index)
    {
    case 0:     //  UPはDOWN, Enter, Back, Configと同じキーになってはいけない
        same_key |= (k == Get_Down());
        same_key |= (k == Get_Enter());
        same_key |= (k == Get_Back());
        same_key |= (k == Get_Config());
        break;
    case 1:     //  Down
        same_key |= (k == Get_Up());
        same_key |= (k == Get_Enter());
        same_key |= (k == Get_Back());
        same_key |= (k == Get_Config());
        break;
    case 2:     //  Pauseは各レーンに対応するキー、Backと同じキーになってはいけない
        same_key |= (k == Get_Back());
        for (unsigned int i = 0; i < LANE_NUM; i++) {
            same_key |= (Get_Lane(i) == k);   //  各レーンのキーをチェック
        }
        break;
    case 3:     //  Enter
        same_key |= (k == Get_Up());
        same_key |= (k == Get_Down());
        same_key |= (k == Get_Back());
        same_key |= (k == Get_Config());
        break;
    case 4:     //  Backは他の全てのキーと同じキーになってはいけない
        for (unsigned int i = 0; i < keys.size(); i++) {
            same_key |= (i != index && keys[i] == k);       //  登録済みのキーでないかチェック
        }
        break;
    case 5:     //  Config
        same_key |= (k == Get_Up());
        same_key |= (k == Get_Down());
        same_key |= (k == Get_Enter());
        same_key |= (k == Get_Back());
        break;
    default:    //  各レーンに対応するキーは、他のレーン及びPause, Backと同じキーになってはいけない
        same_key |= (k == Get_Pause());
        same_key |= (k == Get_Back());
        for (unsigned int i = 0; i < LANE_NUM; i++) {
            same_key |= (index != i + keys.size() - LANE_NUM && Get_Lane(i) == k);   //  各レーンのキーをチェック
        }
        break;
    }
    if (same_key) { return false; }
    keys[index] = k;
    return true;
}
