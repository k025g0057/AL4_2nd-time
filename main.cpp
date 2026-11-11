#include <Windows.h>
#include"KamataEngine.h"
#include"GameScene.h"
#include"TitleScene.h"
#include"StageManager.h"
#include <fstream>        
#include <sstream>
#ifdef USE_IMGUI
#include "imgui.h"
#endif


//AL2_4-2ex1の課題
using namespace KamataEngine;

void ChangeScene();

void UpdateScene();

void DrawScene();

enum class Scene {
	kUnknown = 0,
	kTitle,
	kGame,
};

// 現在のシーンを管理する変数。最初はタイトル画面から始めます
Scene scene = Scene::kTitle;

GameScene* gameScene = nullptr;
TitleScene* titleScene = nullptr;

StageManager* stageManager = nullptr;

void LoadDebugSettings() {
	// ファイルを読み込みモードで開く
	std::ifstream file("DebugSettings.ini");

	// ファイルが無事に開けたら処理を行う
	if (file.is_open()) {
		std::string line;
		// 1行ずつ読み込む
		while (std::getline(file, line)) {
			std::stringstream line_stream(line);
			std::string key, value;

			// 1行の中から「キー」と「バリュー」を半角スペース区切りで読み取る
			if (line_stream >> key >> value) {
				// キーが "InitialStage" なら現在ステージに設定する
				if (key == "InitialStage") {
					stageManager->SetCurrentStageIndexByName(value);
				}
			}
		}
		file.close(); // ファイルを閉じる
	}
}


void ChangeScene()
{
	switch (scene) {
	case Scene::kTitle:
		if (titleScene->IsFinished()) {
			// シーン変更
			scene = Scene::kGame;
			// 旧シーンの解放
			delete titleScene;
			titleScene = nullptr;
			// 新シーンの生成と初期化
			gameScene = new GameScene;
			gameScene->Initialize(stageManager);
		}
		break;
	case Scene::kGame:
		// 👑 もしゲーム本編（GameScene）が終了フラグを立てたら、タイトル画面に戻す
		if (gameScene->IsFinished()) {
			// シーン変更
			scene = Scene::kTitle;
			// 旧シーンの解放
			delete gameScene;
			gameScene = nullptr;
			// 新シーンの生成と初期化
			titleScene = new TitleScene();
			titleScene->Initialize();
		}

		else if (gameScene->IsReloadRequested()) {
			// シーンリロード（古いゲームシーンを消して、新しく作り直す）
			delete gameScene;
			gameScene = nullptr;
			gameScene = new GameScene();
			gameScene->Initialize(stageManager);
		}
		break;
	}
}

void UpdateScene()
{
	switch (scene) {
	case Scene::kTitle:
		titleScene->Update();
		break;
	case Scene::kGame:
		gameScene->Update();
		break;
	}
}

void DrawScene()
{
	switch (scene) {
	case Scene::kTitle:
		titleScene->Draw();
		break;
	case Scene::kGame:
		gameScene->Draw();
		break;
	}
}


// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	// エンジンの初期化
	KamataEngine::Initialize(L"LE2C_24_ハヤシ_テルト_ハンマーだけで");
#ifdef USE_IMGUI
	// Use a Japanese Mincho font consistently for ImGui text across the game.
	char windowsDirectory[MAX_PATH] = {};
	const UINT windowsDirectoryLength = GetWindowsDirectoryA(windowsDirectory, MAX_PATH);
	if (windowsDirectoryLength > 0 && windowsDirectoryLength < MAX_PATH) {
		char fontPath[MAX_PATH] = {};
		strcpy_s(fontPath, windowsDirectory);
		strcat_s(fontPath, "\\Fonts\\BIZ-UDMinchoM.ttc");
		ImGuiIO& imguiIO = ImGui::GetIO();
		ImFont* gameFont = imguiIO.Fonts->AddFontFromFileTTF(fontPath, 22.0f, nullptr,
			imguiIO.Fonts->GetGlyphRangesJapanese());
		if (!gameFont) {
			strcpy_s(fontPath, windowsDirectory);
			strcat_s(fontPath, "\\Fonts\\YuGothM.ttc");
			gameFont = imguiIO.Fonts->AddFontFromFileTTF(fontPath, 22.0f, nullptr,
				imguiIO.Fonts->GetGlyphRangesJapanese());
		}
		if (gameFont) {
			imguiIO.FontDefault = gameFont;
		}
	}
#endif

	DirectXCommon* dxCommon = DirectXCommon::GetInstance();


	// 初期化
	stageManager = new StageManager;
	stageManager->LoadStageDataCsv();

	// CSVのロード直後に、設定ファイルを読み込んで初期ステージを上書き設定する
	LoadDebugSettings();


	scene = Scene::kTitle;

	titleScene = new TitleScene();
	titleScene->Initialize();

	// メインループ
	while (true) {
		// エンジンの更新
		if (KamataEngine::Update()) {
			break;
		}
		ChangeScene();
		ImGuiManager::GetInstance()->Begin();
	
		UpdateScene();
		ImGuiManager::GetInstance()->End();
		// 描画開始
		dxCommon->PreDraw();
		// ここに描画を表示する

		
		
		DrawScene();
		ImGuiManager::GetInstance()->Draw();
		dxCommon->PostDraw();
	}

	// ゲームシーンの開放
	delete gameScene;
	delete titleScene;
	delete stageManager;

	// nullparの開放
	gameScene = nullptr;
	titleScene = nullptr; // ←これはなくてもいいけど、念のために書いておくと安全らしい
	stageManager = nullptr;

	// エンジンの終了処理
	KamataEngine::Finalize();

	return 0;

	
}
