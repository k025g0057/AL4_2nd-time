#include "EnemyBullet.h"
#include <cmath>
#include "MapChipField.h"
#include "Math.h"

using namespace KamataEngine;

void EnemyBullet::Initialize(Model* model, Camera* camera, MapChipField* mapChipField,
	const Vector3& position, const Vector3& velocity) {
	model_ = model;
	camera_ = camera;
	mapChipField_ = mapChipField;
	velocity_ = velocity;
	lifetime_ = 0.0f;
	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	// 専用のボス弾モデルを既存と同じ表示サイズで使う。
	worldTransform_.scale_ = {1.5f, 1.5f, 1.5f};
	UpdateWorldTransform(worldTransform_);
}

bool EnemyBullet::Update() {
	constexpr float kDeltaTime = 1.0f / 60.0f;
	constexpr float kMaxLifetime = 6.0f;
	lifetime_ += kDeltaTime;
	worldTransform_.translation_.x += velocity_.x;
	worldTransform_.translation_.y += velocity_.y;
	worldTransform_.translation_.z += velocity_.z;
	worldTransform_.rotation_.z += 0.08f;
	UpdateWorldTransform(worldTransform_);

	if (lifetime_ >= kMaxLifetime || IsCollidingWithBlock()) {
		return true;
	}
	if (camera_ && (worldTransform_.translation_.x < camera_->translation_.x - 60.0f ||
		worldTransform_.translation_.x > camera_->translation_.x + 100.0f)) {
		return true;
	}
	return false;
}

void EnemyBullet::Draw() {
	if (model_ && camera_) {
		model_->Draw(worldTransform_, *camera_);
	}
}

AABB EnemyBullet::GetAABB() const {
	const Vector3& position = worldTransform_.translation_;
	return {{position.x - kHitRadius, position.y - kHitRadius, position.z - kHitRadius},
		{position.x + kHitRadius, position.y + kHitRadius, position.z + kHitRadius}};
}

bool EnemyBullet::IsCollidingWithBlock() const {
	if (!mapChipField_) {
		return false;
	}

	const AABB bulletAabb = GetAABB();
	const Vector3& position = worldTransform_.translation_;
	const int centerColumn = static_cast<int>(std::floor((position.x + MapChipField::kBlockWidth / 2.0f) / MapChipField::kBlockWidth));
	const int rowFromBottom = static_cast<int>(std::floor((position.y + MapChipField::kBlockHeight / 2.0f) / MapChipField::kBlockHeight));
	const int centerRow = static_cast<int>(MapChipField::kNumBlockVertical) - 1 - rowFromBottom;

	// 弾の大きさ分だけ周囲のブロックも調べ、地面・足場に当たったら消す。
	for (int row = centerRow - 1; row <= centerRow + 1; ++row) {
		if (row < 0 || row >= static_cast<int>(MapChipField::kNumBlockVertical)) {
			continue;
		}
		for (int column = centerColumn - 1; column <= centerColumn + 1; ++column) {
			if (column < 0) {
				continue;
			}
			const uint32_t xIndex = static_cast<uint32_t>(column);
			const uint32_t yIndex = static_cast<uint32_t>(row);
			if (mapChipField_->GetMapChipTypeByIndex(xIndex, yIndex) != MapChipType::kBlock) {
				continue;
			}

			const Vector3 blockPosition = mapChipField_->GetMapChipPositionByIndex(xIndex, yIndex);
			const AABB blockAabb{
				{blockPosition.x - MapChipField::kBlockWidth / 2.0f, blockPosition.y - MapChipField::kBlockHeight / 2.0f, blockPosition.z - MapChipField::kBlockWidth / 2.0f},
				{blockPosition.x + MapChipField::kBlockWidth / 2.0f, blockPosition.y + MapChipField::kBlockHeight / 2.0f, blockPosition.z + MapChipField::kBlockWidth / 2.0f},
			};
			if (IsCollision(bulletAabb, blockAabb)) {
				return true;
			}
		}
	}
	return false;
}
