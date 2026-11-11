#include "Enemy.h"
#include <numbers>
#include <algorithm> 
#include "Math.h"
#include "Player.h"
#include "GameScene.h"
#include "MapChipField.h"

void Enemy::Initialize(Model* model, Camera* camera, const Vector3& position) {
	// モデルとカメラを保持
	model_ = model;
	camera_ = camera;

	// ワールド変換の初期化
	worldTransform_.Initialize();
	objectColor_.Initialize();
	objectColor_.SetColor({1.0f, 1.0f, 1.0f, 1.0f});

	// スケールの設定 (プレイヤーに合わせて3.0fにしていますが、敵のサイズに合わせて変更してください)
	worldTransform_.scale_ = {3.0f, 3.0f, 3.0f};

	// 向き（回転）の設定 (最初は左を向かせる場合は -std::numbers::pi_v<float> / 2.0f など)
	worldTransform_.rotation_.y = -std::numbers::pi_v<float> / 2.0f;

	// 引数で受け取った初期位置を設定
	worldTransform_.translation_ = position;

	//アニメーションのタイマーを初期化
	walkTimer_ = 0.0f;

	isDead_ = false;

	// 初期状態は当たり判定を有効（false）にする
	isCollisionDisabled_ = false;

	// 初期状態は歩行
	behavior_ = Behavior::kRoot;
	BehaviorRootInitialize();

	// 初期化時に行列を計算・転送
	UpdateWorldTransform(worldTransform_);

}

//  毎フレームの更新処理（状態によって呼び分ける）
void Enemy::Update() {
	// 仮ボスはカメラから見た画面右側・中央付近に固定する。
	if (isScreenBoss_ && camera_ && behavior_ != Behavior::kDeath && behaviorRequest_ != Behavior::kDeath) {
		// 通常敵と同じ周期・角度で体を揺らす。位置の固定処理とは独立して動かす。
		walkTimer_ += 1.0f / 60.0f;
		const float param = std::sin(std::numbers::pi_v<float> * 2.0f * walkTimer_ / kWalkMotionTime);
		const float degree = kWalkMotionAngleStart +
			(kWalkMotionAngleEnd - kWalkMotionAngleStart) * (param + 1.0f) / 2.0f;
		worldTransform_.rotation_.x = isScreenBossRoaring_ ? -0.4f :
			(screenBossBodyMotionEnabled_ ? degree * (std::numbers::pi_v<float> / 180.0f) : 0.0f);
		worldTransform_.rotation_.y = isScreenBossIntroMode_ ? std::numbers::pi_v<float> : screenBossRotationY_;

		// 被弾時はシールドエネミーと同じように、約0.5秒かけて傾いて元に戻る。
		worldTransform_.rotation_.z = 0.0f;
		if (isHurtAnimating_) {
			constexpr float kHurtDuration = 0.5f;
			hurtTimer_ += 1.0f / 60.0f;
			const float progress = std::clamp(hurtTimer_ / kHurtDuration, 0.0f, 1.0f);
			const float sinWave = std::sin(progress * std::numbers::pi_v<float>);
			constexpr float kStartDip = -0.1f;
			constexpr float kMaxRecline = 0.5f;
			worldTransform_.rotation_.z = kStartDip * (1.0f - sinWave) + kMaxRecline * sinWave;
			if (hurtTimer_ >= kHurtDuration) {
				isHurtAnimating_ = false;
				hurtTimer_ = 0.0f;
				worldTransform_.rotation_.z = 0.0f;
			}
		}

		const float viewDepth = worldTransform_.translation_.z - camera_->translation_.z;
		const float projectionScaleX = camera_->matProjection.m[0][0];
		const float projectionScaleY = camera_->matProjection.m[1][1];
		if (viewDepth > 0.0f && projectionScaleX > 0.0f && projectionScaleY > 0.0f) {
			worldTransform_.translation_.x = camera_->translation_.x + screenBossX_ * viewDepth / projectionScaleX;
			worldTransform_.translation_.y = camera_->translation_.y + screenBossY_ * viewDepth / projectionScaleY;
		}
		UpdateWorldTransform(worldTransform_);
		return;
	}

	// 状態遷移リクエストの処理
	if (behaviorRequest_ != Behavior::kUnknown) {
		// 振るまいを更新
		behavior_ = behaviorRequest_;

		// リクエストをリセット
		behaviorRequest_ = Behavior::kUnknown;

		// 各状態の初期化関数を呼び出す
		switch (behavior_) {
		case Behavior::kRoot:
			BehaviorRootInitialize();
			break;
		case Behavior::kDeath:
			BehaviorDeathInitialize(); // ここでデス演出が始まる
			break;
		}
	}

	// 状態ごとの更新
	switch (behavior_) {
	case Behavior::kRoot:
		BehaviorRootUpdate();
		break;
	case Behavior::kDeath:
		BehaviorDeathUpdate();
		break;
	}
}
#pragma region 歩行ビヘイビア (kRoot)
void Enemy::BehaviorRootInitialize() {
	// 歩行開始時の初期化があればここに書く（今回は特になし）
}

void Enemy::BehaviorRootUpdate() {
	//元々 Update() に書いてあった移動・アニメーション処理
	walkTimer_ += 1.0f / 60.0f;

	float param = std::sin(std::numbers::pi_v<float> * 2.0f * walkTimer_ / 0.5f);
	
	float degree = kWalkMotionAngleStart + (kWalkMotionAngleEnd - kWalkMotionAngleStart) * (param + 1.0f) / 2.0f;

	float toRadian = std::numbers::pi_v<float> / 180.0f;
	worldTransform_.rotation_.x = degree * toRadian;

	worldTransform_.translation_.x += velocity_.x;
	worldTransform_.translation_.y += velocity_.y;
	worldTransform_.translation_.z += velocity_.z;
	if (groundMapChipField_) {
		const MapChipField::IndexSet index = groundMapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_);
		float groundTop = 0.0f;
		bool foundGround = false;
		for (uint32_t yIndex = 0; yIndex < MapChipField::kNumBlockVertical; ++yIndex) {
			if (groundMapChipField_->GetMapChipTypeByIndex(index.xIndex, yIndex) != kBlock) {
				continue;
			}
			const MapChipField::Rect blockRect = groundMapChipField_->GetRectByIndex(index.xIndex, yIndex);
			if (blockRect.top <= worldTransform_.translation_.y + MapChipField::kBlockHeight &&
				(!foundGround || blockRect.top > groundTop)) {
				groundTop = blockRect.top;
				foundGround = true;
			}
		}
		if (foundGround) {
			// モデルの下端が地面に接するよう、設定スケールに合わせて高さを調整する。
			worldTransform_.translation_.y = groundTop + groundModelBottomOffset_ * worldTransform_.scale_.y;
		}
	}

	UpdateWorldTransform(worldTransform_);
}
#pragma endregion

#pragma region デス演出ビヘイビア (kDeath)
void Enemy::BehaviorDeathInitialize() {
	deathTimer_ = 0;
	isHurtAnimating_ = false;
	deathStartScale_ = worldTransform_.scale_;
	deathStartTranslationY_ = worldTransform_.translation_.y;
	worldTransform_.rotation_.z = 0.0f;
	if (!isScreenBoss_) {
		// 雑魚敵は回転させず、正面を保ったまま押しつぶす。
		worldTransform_.rotation_.x = 0.0f;
	}

	// デス演出が始まったら、即座に当たり判定を無効にする。
	isCollisionDisabled_ = true;
}

void Enemy::BehaviorDeathUpdate() {
	++deathTimer_;
	constexpr float kDeathTime = 30.0f;
	const float t = std::clamp(static_cast<float>(deathTimer_) / kDeathTime, 0.0f, 1.0f);

	if (isScreenBoss_) {
		// ボスの死亡演出は従来の回転を維持する。
		const float startRotY = -std::numbers::pi_v<float> / 2.0f;
		const float endRotY = startRotY + (std::numbers::pi_v<float> * 2.0f * 5.0f);
		worldTransform_.rotation_.y = Easing::EaseOut(startRotY, endRotY, t);
		const float startRotX = worldTransform_.rotation_.x;
		const float endRotX = std::numbers::pi_v<float> / 2.0f;
		worldTransform_.rotation_.x = Easing::EaseIn(startRotX, endRotX, t);
		worldTransform_.translation_.y += 0.1f;
	} else {
		// 前半で横に広がりながら低くつぶれ、後半で縮小して消える。
		constexpr float kSquashEnd = 0.55f;
		const float squashLinear = std::clamp(t / kSquashEnd, 0.0f, 1.0f);
		const float squash = squashLinear * squashLinear * (3.0f - 2.0f * squashLinear);
		const float vanishLinear = std::clamp((t - kSquashEnd) / (1.0f - kSquashEnd), 0.0f, 1.0f);
		const float vanish = vanishLinear * vanishLinear * (3.0f - 2.0f * vanishLinear);
		const float remaining = 1.0f - vanish;
		worldTransform_.scale_.x = deathStartScale_.x * (1.0f + 0.25f * squash) * remaining;
		worldTransform_.scale_.y = deathStartScale_.y * (1.0f - 0.88f * squash) * remaining;
		worldTransform_.scale_.z = deathStartScale_.z * (1.0f + 0.25f * squash) * remaining;
		// 地面に接したままつぶれるよう、モデル下端の高さを維持する。
		worldTransform_.translation_.y = deathStartTranslationY_ +
			groundModelBottomOffset_ * (worldTransform_.scale_.y - deathStartScale_.y);
	}

	UpdateWorldTransform(worldTransform_);
	if (deathTimer_ >= static_cast<uint32_t>(kDeathTime)) {
		isDead_ = true;
	}
}
#pragma endregion

void Enemy::Draw(Camera* camera) {
	// 被弾中は点滅し、通常敵の死亡中はつぶれながらフェードアウトする。
	if (model_) {
		float alpha = 1.0f;
		if (isHurtAnimating_) {
			const uint32_t animationFrame = static_cast<uint32_t>(hurtTimer_ * 60.0f);
			alpha = (animationFrame / 3) % 2 == 0 ? 0.25f : 1.0f;
		} else if (behavior_ == Behavior::kDeath) {
			if (isScreenBoss_) {
				alpha = (deathTimer_ / 3) % 2 == 0 ? 0.25f : 1.0f;
			} else {
				const float progress = std::clamp(static_cast<float>(deathTimer_) / 30.0f, 0.0f, 1.0f);
				alpha = 1.0f - std::clamp((progress - 0.55f) / 0.45f, 0.0f, 1.0f);
			}
		}
		objectColor_.SetColor({1.0f, 1.0f, 1.0f, alpha});
		model_->Draw(worldTransform_, *camera, &objectColor_);
	}
}
AABB Enemy::GetAABB() {
	// プレイヤーの現在の中心座標を取得
	Vector3 worldPos = GetWorldPosition(); // たぶんこれが原因


	AABB aabb;

	// 💡 キャラクターの「横幅」を 2.0f、「縦幅」を 2.0f と仮定して計算しています。
	// 当たり判定の箱を大きくしたり小さくしたりしたい場合は、この「2.0f」の数値を調整してください。
	// ボスは通常敵の2倍のモデルスケールなので、当たり判定も幅・高さを2倍にする。
	float width = isScreenBoss_ ? 4.0f : 2.0f;  // 横幅
	float height = isScreenBoss_ ? 4.0f : 2.0f; // 縦幅

	// ボスだけ下端を1ワールド単位下げ、下側からの接触も判定する。
	const float kHitboxDownwardExtension = isScreenBoss_ ? 1.0f : 0.0f;
	// 最小点(min)と最大点(max)を設定。上端は維持して下方向だけ広げる。
	aabb.min = {worldPos.x - width / 2.0f, worldPos.y - height / 2.0f - kHitboxDownwardExtension, worldPos.z - width / 2.0f};
	aabb.max = {worldPos.x + width / 2.0f, worldPos.y + height / 2.0f, worldPos.z + width / 2.0f};

	return aabb;
}

void Enemy::OnCollision(const Player* player) {
	// すでにデス演出中なら、多重ヒットして演出が巻き戻るのを防ぐために何もしない
	if (behavior_ == Behavior::kDeath) {
		return;
	}

	// プレイヤーが攻撃中なら敵が死ぬ（デス演出へ移行リクエスト）
	if (player->IsAttack()) {
		behaviorRequest_ = Behavior::kDeath;

		// 攻撃中心に近い敵の当たり判定表面からヒットエフェクトを出す。
		if (gameScene_) {
			gameScene_->CreateHitEffectAtContact(player->GetWorldPosition(), GetAABB());
		}
	}
}

Vector3 Enemy::GetWorldPosition() {
	// ワールド座標を入れる変数
	Vector3 worldPos;

	// ワールド行列の平行移動成分（m[3][0], m[3][1], m[3][2]）を取得
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];

	return worldPos;
}

void Enemy::SetGameScene(GameScene* gameScene) { gameScene_ = gameScene; }

void Enemy::SetScale(float scale) {
	worldTransform_.scale_ = {scale, scale, scale};
	UpdateWorldTransform(worldTransform_);
}

void Enemy::SetAsScreenBoss(float scale) {
	isScreenBoss_ = true;
	worldTransform_.scale_ = {scale, scale, scale};
	velocity_ = {0.0f, 0.0f, 0.0f};
	UpdateWorldTransform(worldTransform_);
}

void Enemy::SetScreenBossRotationY(float rotationY) {
	screenBossRotationY_ = rotationY;
	if (isScreenBoss_) {
		worldTransform_.rotation_.y = rotationY;
		UpdateWorldTransform(worldTransform_);
	}
}

void Enemy::SetScreenBossBodyMotionEnabled(bool enabled) {
	screenBossBodyMotionEnabled_ = enabled;
}

void Enemy::SetScreenBossPosition(float screenX, float screenY) {
	screenBossX_ = screenX;
	screenBossY_ = screenY;
}

void Enemy::SetScreenBossIntroMode(bool active) {
	isScreenBossIntroMode_ = active;
}

void Enemy::SetScreenBossRoaring(bool roaring) {
	isScreenBossRoaring_ = roaring;
}

void Enemy::StartHurtAnimation() {
	if (behavior_ == Behavior::kDeath || behaviorRequest_ == Behavior::kDeath) {
		return;
	}
	hurtTimer_ = 0.0f;
	isHurtAnimating_ = true;
}

void Enemy::StartDeathAnimation() {
	if (behavior_ == Behavior::kDeath || behaviorRequest_ == Behavior::kDeath) {
		return;
	}
	isHurtAnimating_ = false;
	behaviorRequest_ = Behavior::kDeath;
}
