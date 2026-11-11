#include "StageManager.h"
#include <cassert>
#include <fstream>
#include <sstream>

void StageManager::LoadStageDataCsv() {
	// 🟢 ステージデータファイルのパス（フォルダ位置に合わせて変更）
	const std::string filePath = "Resources/block/stageDatas.csv";

	//ifstreamでステージデータファイルを開く
	std::ifstream file;
	file.open(filePath);

	// assert正常に開けたかチェック
	assert(file.is_open() && "ステージデータファイルが存在しません");

	// ファイルの内容を格納するstringstreamの宣言
	std::stringstream stringStream;

	// ファイルの内容をstringstreamにコピーする
	stringStream << file.rdbuf();

	// ファイルを閉じる
	file.close();

	// 1行分の内容を格納するstringの宣言
	std::string line;

	while (std::getline(stringStream, line)) {
		std::stringstream lineStream(line);

		// ステージデータを格納する構造体
		StageData stageData;

		// 】カンマ区切りの一つ分を格納するstringの宣言
		std::string cell;

		//カンマ区切りで次のデータ（ステージ名）を取得する
		std::getline(lineStream, cell, ',');

		// stageData.nameステージ名を格納する
		stageData.name = cell;

		// カンマ区切りで次のデータ（制限時間）を取得する
		std::getline(lineStream, cell, ',');

		//stageData.timeLimit整数に変換して制限時間を格納する
		stageData.timeLimit = std::stoi(cell);

		// ステージデータテーブルに格納する
		stageDatas_.push_back(stageData);
	}
} // 👈 関数の終わり

void StageManager::SetCurrentStageIndexByName(const std::string& name) {
	// 全ステージデータを検索
	for (size_t i = 0; i < stageDatas_.size(); ++i) {
		// ステージ名が一致したら現在ステージ番号を設定する
		if (stageDatas_[i].name == name) {
			currentStageIndex_ = static_cast<int32_t>(i);
			// 目的を達したので関数を抜ける
			return;
		}
	}

	// ループを抜けてここまで来てしまった ＝ 一致するステージ名が全データの中に無かったということ
	assert(false && "指定されたステージ名は存在しません");
}