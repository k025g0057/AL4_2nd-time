#pragma once
#include "KamataEngine.h"
#include "Collision.h"

class MapChipField;

class EnemyBullet {
public:
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, MapChipField* mapChipField,
		const KamataEngine::Vector3& position, const KamataEngine::Vector3& velocity);
	bool Update();
	void Draw();
	AABB GetAABB() const;

private:
	bool IsCollidingWithBlock() const;

	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;
	MapChipField* mapChipField_ = nullptr;
	
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Vector3 velocity_{};
	float lifetime_ = 0.0f;
	static inline constexpr float kHitRadius = 0.9f;
};
