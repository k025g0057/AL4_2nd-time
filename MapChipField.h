#pragma once
#include <cstdint>
#include <vector>
#include <fstream>
#include <sstream>
#include <cassert>
#include <random>
#include "KamataEngine.h"



enum MapChipType {
	kBlank, // 空白
	kBlock, // ブロック
	kPlayer,//プレイヤー
	kEnemy,  // 敵
};

struct MapChipDataUnit {
	MapChipType type;
	uint8_t subID;
};

struct MapChipData { 
	std::vector<std::vector<MapChipDataUnit>> data;
};

// マップチップCSVの文字番号
enum MapChipCharIndex {
	kChipType = 0, // マップチップタイプ
	kChipSubID = 1 // タイプごとのサブID
};


class MapChipField {
public: 
	struct IndexSet {
		uint32_t xIndex;
		uint32_t yIndex;
	};

	struct Rect {
		float left;   // 左端
		float right;  // 右端
		float bottom; // 下端
		float top;    // 上端
	};

	Rect GetRectByIndex(uint32_t xIndex, uint32_t yIndex);

	void ResetMapChipData();

	// 1ブロックのサイズ
	static inline const float kBlockWidth = 4.0f;
	static inline const float kBlockHeight = 4.0f;

	// ブロックの個数
	static inline const uint32_t kNumBlockVertical = 20;
	static inline const uint32_t kNumBlockHorizontal = 100; // CSVから読み込む初期列数。以降は動的に追加する。

	MapChipData mapChipData_; 

	void LoadMapChipCsv(const std::string& filePath);

	// CSVの床を、穴のないランダムな段差の地形に置き換える。
	void GenerateRandomGround();
	void EnsureGeneratedThrough(uint32_t xIndex);
	uint32_t GetGeneratedColumnCount() const {
		return mapChipData_.data.empty() ? 0 : static_cast<uint32_t>(mapChipData_.data[0].size());
	}

	MapChipType GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex);

 KamataEngine::Vector3 GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex);
	
 IndexSet GetMapChipIndexSetByPosition(const KamataEngine::Vector3& position);

 uint8_t GetMapChipSubIDByIndex(uint32_t xIndex, uint32_t yIndex);

private:
	void GenerateGroundColumn(uint32_t xIndex);

	std::mt19937 randomEngine_{std::random_device{}()};
	std::uniform_int_distribution<int> heightChange_{-1, 1};
	std::uniform_int_distribution<uint32_t> segmentLength_{3, 8};
	int groundHeight_ = 0;
	uint32_t remainingSegmentColumns_ = 0;
	std::vector<uint32_t> surfaceRows_;

};
