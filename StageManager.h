#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include<KamataEngine.h>
#include <cassert>

// 🟢【受け側の構造体】
struct StageData {
	std::string name;  // ステージ名（フィールドCSVファイル名）
	int32_t timeLimit; // 制限時間（秒）※先ほどの100や130といった数字のことです
};


class StageManager {
public:
	void Initialize();
	void Update();

	void LoadStageDataCsv();
	int32_t GetStageCount() const { return static_cast<int32_t>(stageDatas_.size()); }

	const StageData& GetStageData(int32_t index) const {
		// index が 0以上 かつ 配列の要素数未満かチェック！
		assert(index >= 0 && index < stageDatas_.size() && "ステージインデックスが範囲外です");
		return stageDatas_[index];
	}

	void SetCurrentStageIndex(int32_t index) {
		// index が 0以上 かつ 配列の要素数未満かチェック！
		assert(index >= 0 && index < stageDatas_.size() && "ステージインデックスが範囲外です");
		currentStageIndex_ = index;
	}

	int32_t GetCurrentStageIndex() const { return currentStageIndex_; }

	const StageData& GetCurrentStageData() const {
		return GetStageData(currentStageIndex_);
	}

	void SetCurrentStageIndexByName(const std::string& name);

private:
	std::vector<StageData> stageDatas_;

	// 【メンバ変数】現在のステージ番号（0がステージ1、1がステージ2...）
	int32_t currentStageIndex_ = 0;
};
