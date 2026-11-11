#define NOMINMAX
#include "CameraController.h"
#include "Player.h"
#include <algorithm>



void CameraController::Initialize(KamataEngine::Camera* camera) {
	camera_ = camera;
}

void CameraController::PauseScroll() {
	isScrollPaused_ = true;
	scrollPauseTimer_ = 0.0f;
	if (target_) {
		// 停止中にプレイヤーがスクロール速度で押し戻されないようにする。
		target_->SetAutoRunSpeed(0.0f);
	}
}

void CameraController::SetTransitionScrollLock(bool locked) {
	isTransitionScrollLocked_ = locked;
	if (target_) {
		target_->SetAutoRunSpeed((locked || isScrollPaused_) ? 0.0f : scrollSpeed_);
	}
}

void CameraController::StartShake(int frames, float strength) {
	shakeFrames_ = frames;
	shakeStrength_ = strength;
}

void CameraController::UpdateShake() {
	if (!camera_) {
		return;
	}

	// 前フレームの揺れを先に戻し、カメラ位置がずれ続けないようにする。
	camera_->translation_.x -= shakeOffset_.x;
	camera_->translation_.y -= shakeOffset_.y;
	shakeOffset_ = {};

	if (shakeFrames_ > 0) {
		std::uniform_real_distribution<float> offsetDistribution(-shakeStrength_, shakeStrength_);
		shakeOffset_.x = offsetDistribution(randomEngine_);
		shakeOffset_.y = offsetDistribution(randomEngine_);
		camera_->translation_.x += shakeOffset_.x;
		camera_->translation_.y += shakeOffset_.y;
		--shakeFrames_;
	}

	camera_->UpdateMatrix();
}

void CameraController::Reset() {
	shakeFrames_ = 0;
	shakeStrength_ = 0.0f;
	shakeOffset_ = {};
	// 追従対象のワールドトランスフォームを参照
	const KamataEngine::WorldTransform& targetWorldTransform = target_->GetWorldTransform();

	// 開始時はプレイヤーが画面左端から10%の位置に見えるようにカメラを置く。
	camera_->translation_.y = targetWorldTransform.translation_.y + targetOffset_.y;
	camera_->translation_.z = targetWorldTransform.translation_.z + targetOffset_.z;
	constexpr float kStartScreenX = -0.8f; // NDC: -1が左端、+1が右端
	const float viewDepth = targetWorldTransform.translation_.z - camera_->translation_.z;
	const float projectionScaleX = camera_->matProjection.m[0][0];
	float cameraOffsetX = targetOffset_.x;
	if (viewDepth > 0.0f && projectionScaleX > 0.0f) {
		cameraOffsetX += -kStartScreenX * viewDepth / projectionScaleX;
	}
	camera_->translation_.x = targetWorldTransform.translation_.x + cameraOffsetX;

	// X方向に終端を設けず、生成した地形に合わせて先へ進める。
	camera_->translation_.y = std::clamp(camera_->translation_.y, movableArea_.bottom, movableArea_.top);

	camera_->UpdateMatrix();
}

void CameraController::Update() {
	// 追従対象がいなければ何もしない
	if (!target_)
		return;

	// 前回の画面揺れを取り除いてから、通常のカメラ位置を計算する。
	camera_->translation_.x -= shakeOffset_.x;
	camera_->translation_.y -= shakeOffset_.y;
	shakeOffset_ = {};

	// --- 追従対象のワールドトランスフォームを参照 ---
	const KamataEngine::WorldTransform& targetWorldTransform = target_->GetWorldTransform();

	Vector3 velocity = target_->GetVelocity();

	// ノックバックから1秒以上経ち、プレイヤーが左端で着地したらスクロールを再開する。
	if (isScrollPaused_) {
		scrollPauseTimer_ += 1.0f / 60.0f;
		const float viewDepth = targetWorldTransform.translation_.z - camera_->translation_.z;
		const float projectionScaleX = camera_->matProjection.m[0][0];
		if (viewDepth > 0.0f && projectionScaleX > 0.0f) {
			if (scrollPauseTimer_ >= 1.0f && target_->IsOnGround()) {
				isScrollPaused_ = false;
				target_->SetAutoRunSpeed(isTransitionScrollLocked_ ? 0.0f : scrollSpeed_);
				target_->CompleteBossHitKnockback();
			}
		}
	}

	// X方向はプレイヤーを追従せず、一定速度で右へ進める（強制スクロール）。
	if (!isScrollPaused_ && !isTransitionScrollLocked_) {
		camera_->translation_.x += scrollSpeed_;
	}

	// 奥行き方向だけプレイヤーを追従する。Y方向は開始時の高さに固定する。
	targetPos_.z = targetWorldTransform.translation_.z + targetOffset_.z + velocity.z * kVelocityBias;

	// --- Z方向だけ追従（線形補間） ---
	camera_->translation_.z = Lerp(camera_->translation_.z, targetPos_.z, kInterpolationRate);

	// 通常のカメラ移動のあとに揺れを重ねる。
	UpdateShake();
}
