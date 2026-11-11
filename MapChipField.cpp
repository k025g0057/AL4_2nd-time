#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "MapChipField.h"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <limits>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <utility>



using namespace KamataEngine;

// マップチップ種別テーブル
std::map<char, MapChipType> mapChipTypeTable = {
    {'B', MapChipType::kBlock},
    {'P', MapChipType::kPlayer},
    {'E', MapChipType::kEnemy },

};

void MapChipField::ResetMapChipData() {
	mapChipData_.data.clear();
	mapChipData_.data.resize(kNumBlockVertical);
	for (std::vector<MapChipDataUnit>& mapChipDataLine : mapChipData_.data) {
		mapChipDataLine.resize(kNumBlockHorizontal);

		for (MapChipDataUnit& unit : mapChipDataLine) {//スライドには書いてないけど一応追加しておく
			unit.type = MapChipType::kBlank;
			unit.subID = 0;
		}
	}
	groundHeight_ = 0;
	remainingSegmentColumns_ = 0;
	surfaceRows_.clear();
}

void MapChipField::LoadMapChipCsv(const std::string& filePath) {
	ResetMapChipData();
	std::ifstream file(filePath);
	assert(file.is_open());

	std::stringstream mapChipCsv;
	mapChipCsv << file.rdbuf();
	file.close();

	for (uint32_t i = 0; i < kNumBlockVertical; ++i) {
		std::string line;
		if (!getline(mapChipCsv, line))
			break;
		std::istringstream lineStream(line);

		for (uint32_t j = 0; j < kNumBlockHorizontal; ++j) {
			std::string word;
			if (!std::getline(lineStream, word, ','))
				break;

			// 空白の場合はスキップ
			if (word.empty()) {
				continue;
			}

			// 先頭文字がいずれかのマップチップ種別に該当するか確認
			if (!mapChipTypeTable.contains(word[kChipType])) {
				continue;
			}

			// 先頭文字でマップチップのタイプを判別
			mapChipData_.data[i][j].type = mapChipTypeTable[word[kChipType]];

			// サブIDを含まない場合はスキップ（0番で確定）
			if (word.size() <= kChipSubID) {
				continue;
			}

			// マップチップのサブIDを設定
			mapChipData_.data[i][j].subID = static_cast<uint8_t>(word[kChipSubID] - '0');
		}
	}
}

void MapChipField::GenerateRandomGround() {
	// プレイヤーと敵の配置情報を一時保存し、地形生成後に床の上へ戻す。
	std::vector<std::pair<uint32_t, MapChipDataUnit>> actorMarkers;
	for (uint32_t y = 0; y < kNumBlockVertical; ++y) {
		for (uint32_t x = 0; x < kNumBlockHorizontal; ++x) {
			MapChipDataUnit& unit = mapChipData_.data[y][x];
			if (unit.type == MapChipType::kPlayer || unit.type == MapChipType::kEnemy) {
				actorMarkers.emplace_back(x, unit);
				unit.type = MapChipType::kBlank;
				unit.subID = 0;
			}
		}
	}

	// 右端の壁は無限スクロールの妨げになるので消す（左端の開始用壁は残す）。
	const uint32_t oldColumnCount = GetGeneratedColumnCount();
	if (oldColumnCount >= 3) {
		const uint32_t rightWallColumn = oldColumnCount - 2;
		for (uint32_t row = 1; row + 1 < kNumBlockVertical; ++row) {
			mapChipData_.data[row][rightWallColumn] = {MapChipType::kBlank, 0};
		}
	}

	// 読み込み済みの列を生成し、その後の列を追加するための乱数状態を続けて保持する。
	groundHeight_ = 0;
	remainingSegmentColumns_ = 0;
	surfaceRows_.clear();
	for (uint32_t column = 0; column < oldColumnCount; ++column) {
		GenerateGroundColumn(column);
	}

	// CSVに置かれていたプレイヤーと敵を、その列の地面の1マス上へ配置する。
	for (const auto& [column, marker] : actorMarkers) {
		if (column >= surfaceRows_.size()) {
			continue;
		}
		const uint32_t spawnRow = surfaceRows_[column] > 0 ? surfaceRows_[column] - 1 : 0;
		mapChipData_.data[spawnRow][column] = marker;
	}
}

void MapChipField::GenerateGroundColumn(uint32_t xIndex) {
	// 最初の8列は平地。その先は3～8列ごとに、最大3ブロックの段差を変える。
	if (xIndex >= 8) {
		if (remainingSegmentColumns_ == 0) {
			groundHeight_ = std::clamp(groundHeight_ + heightChange_(randomEngine_), 0, 1);
			remainingSegmentColumns_ = segmentLength_(randomEngine_);
		}
		--remainingSegmentColumns_;
	}

	const uint32_t surfaceRow = kNumBlockVertical - 1 - static_cast<uint32_t>(groundHeight_);
	if (surfaceRows_.size() <= xIndex) {
		surfaceRows_.resize(static_cast<size_t>(xIndex) + 1, kNumBlockVertical - 1);
	}
	surfaceRows_[xIndex] = surfaceRow;

	// 地面より下をブロックで満たして、落下する穴は作らない。
	for (uint32_t row = surfaceRow; row < kNumBlockVertical; ++row) {
		mapChipData_.data[row][xIndex] = {MapChipType::kBlock, 0};
	}
}

void MapChipField::EnsureGeneratedThrough(uint32_t xIndex) {
	while (GetGeneratedColumnCount() <= xIndex) {
		const uint32_t newColumn = GetGeneratedColumnCount();
		for (std::vector<MapChipDataUnit>& row : mapChipData_.data) {
			row.push_back({MapChipType::kBlank, 0});
		}
		GenerateGroundColumn(newColumn);
	}
}

MapChipType MapChipField::GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex) {
	// y方向と、負座標からの符号なし整数アンダーフローを範囲外として扱う。
	if (yIndex >= kNumBlockVertical || xIndex > static_cast<uint32_t>(std::numeric_limits<int32_t>::max())) {
		return MapChipType::kBlank;
	}
	EnsureGeneratedThrough(xIndex);
	return mapChipData_.data[yIndex][xIndex].type;
}

::Vector3 MapChipField::GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex) {
	return ::Vector3(kBlockWidth * static_cast<float>(xIndex), kBlockHeight * (static_cast<float>(kNumBlockVertical) - 1.0f - static_cast<float>(yIndex)), 0.0f);
}

MapChipField::IndexSet MapChipField::GetMapChipIndexSetByPosition(const KamataEngine::Vector3& position) {
	IndexSet indexSet{};
	indexSet.xIndex = static_cast<uint32_t>((position.x + kBlockWidth / 2.0f) / kBlockWidth);
	indexSet.yIndex = static_cast<uint32_t>((position.y + kBlockHeight / 2.0f) / kBlockHeight);
	indexSet.yIndex = kNumBlockVertical - 1 - indexSet.yIndex;
	return indexSet;
}


MapChipField::Rect MapChipField::GetRectByIndex(uint32_t xIndex, uint32_t yIndex) {
	// 指定ブロックの中心座標を取得する（すでに実装されている関数を呼び出す）
	Vector3 center = GetMapChipPositionByIndex(xIndex, yIndex);

	Rect rect;
	rect.left   = center.x - kBlockWidth / 2.0f;  // 左端ライン
	rect.right  = center.x + kBlockWidth / 2.0f;  // 右端ライン
	rect.bottom = center.y - kBlockHeight / 2.0f; // 下端ライン
	rect.top    = center.y + kBlockHeight / 2.0f; // 上端ライン

	return rect;
}

uint8_t MapChipField::GetMapChipSubIDByIndex(uint32_t xIndex, uint32_t yIndex) {
	// 範囲外（配列を飛び出している指定）なら、安全のために「0」を返す
	if (xIndex >= GetGeneratedColumnCount() || yIndex >= kNumBlockVertical) {
		return 0;
	}

	// 指定されたマスの構造体から「.subID」を取り出して返す！
	return mapChipData_.data[yIndex][xIndex].subID;
}
