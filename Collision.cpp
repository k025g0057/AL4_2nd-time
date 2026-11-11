#include "Collision.h"

// 交差判定関数の実装
bool IsCollision(const AABB& aabb1, const AABB& aabb2) {
	if ((aabb1.min.x <= aabb2.max.x && aabb1.max.x >= aabb2.min.x) && // X軸の重なり
	    (aabb1.min.y <= aabb2.max.y && aabb1.max.y >= aabb2.min.y) && // Y軸の重なり
	    (aabb1.min.z <= aabb2.max.z && aabb1.max.z >= aabb2.min.z))   // Z軸の重なり
	{
		return true; // すべての軸で重なっていれば衝突
	}
	return false;
}