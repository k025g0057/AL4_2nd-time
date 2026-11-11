
#pragma once
#include "KamataEngine.h"

using namespace KamataEngine;

// AABBの定義
struct AABB {
	Vector3 min; //!< 最小値の角座標
	Vector3 max; //!< 最大値の角座標
};

// クラスの前方宣言
class Player;
class Enemy;

// 交差判定関数の宣言
bool IsCollision(const AABB& aabb1, const AABB& aabb2);