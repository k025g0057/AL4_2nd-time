#include "DeatParticles.h"
#include "Math.h"
#include <algorithm>
#include <cmath>
#include <numbers>

using namespace KamataEngine;

void DeatParticles::Initialize(Model* playerModel, Model* lightParticleModel, Camera* camera, const WorldTransform& playerTransform) {
	playerModel_ = playerModel;
	lightParticleModel_ = lightParticleModel;
	camera_ = camera;
	playerTransform_.Initialize();
	playerTransform_.scale_ = playerTransform.scale_;
	playerTransform_.rotation_ = playerTransform.rotation_;
	playerTransform_.translation_ = playerTransform.translation_;
	originPosition_ = playerTransform.translation_;
	originRotation_ = playerTransform.rotation_;
	initialScale_ = playerTransform.scale_;
	playerColor_.Initialize();
	lightColor_.Initialize();
	elapsed_ = 0.0f;
	isFinished_ = false;
	lightParticlesActive_.fill(false);
	playerTint_ = {1.0f, 1.0f, 1.0f, 1.0f};
	lightTint_ = {1.0f, 0.78f, 0.24f, 1.0f};

	for (std::size_t i = 0; i < kLightParticleCount; ++i) {
		lightTransforms_[i].Initialize();
		lightTransforms_[i].scale_ = {0.0f, 0.0f, 0.0f};
		const float angle = std::numbers::pi_v<float> *
			(static_cast<float>(i) + 0.5f) / static_cast<float>(kLightParticleCount - 1);
		const float speed = 1.0f + 0.15f * static_cast<float>(i % 3);
		lightVelocities_[i] = {std::cos(angle) * speed, 0.7f + std::sin(angle) * 0.5f, 0.0f};
	}
	UpdateWorldTransform(playerTransform_);
}

void DeatParticles::Update() {
	if (isFinished_) {
		return;
	}

	elapsed_ += kDeltaTime;
	const float fallProgress = std::clamp((elapsed_ - 0.22f) / 0.78f, 0.0f, 1.0f);
	const float easedFall = fallProgress * fallProgress * (3.0f - 2.0f * fallProgress);
	const float sinkProgress = std::clamp((elapsed_ - 0.78f) / (kDuration - 0.78f), 0.0f, 1.0f);
	const float easedSink = sinkProgress * sinkProgress * (3.0f - 2.0f * sinkProgress);

	// 最初に短く点滅してから、少し跳ね返り、後ろへ倒れて地面に沈む。
	const float recoilProgress = std::clamp(elapsed_ / 0.22f, 0.0f, 1.0f);
	const float recoilLift = 0.42f * std::sin(recoilProgress * std::numbers::pi_v<float>);
	const float facingSign = originRotation_.y >= 0.0f ? -1.0f : 1.0f;
	playerTransform_.translation_ = {originPosition_.x + facingSign * 0.42f * easedFall,
		originPosition_.y + recoilLift - 0.9f * easedFall - 0.85f * easedSink, originPosition_.z};
	playerTransform_.rotation_ = originRotation_;
	playerTransform_.rotation_.z += facingSign * 1.35f * easedFall;
	const float shrink = std::clamp((elapsed_ - 0.82f) / (kDuration - 0.82f), 0.0f, 1.0f);
	const float scaleFactor = 1.0f - shrink * shrink;
	playerTransform_.scale_ = {initialScale_.x * scaleFactor,
		initialScale_.y * scaleFactor * (1.0f - 0.35f * easedSink),
		initialScale_.z * scaleFactor};

	if (elapsed_ < 0.22f) {
		playerTint_.w = (static_cast<int>(elapsed_ * 60.0f / 3.0f) % 2 == 0) ? 0.28f : 1.0f;
	} else {
		playerTint_.w = std::clamp(1.0f - shrink * 1.3f, 0.0f, 1.0f);
	}
	playerColor_.SetColor(playerTint_);
	UpdateWorldTransform(playerTransform_);

	if (elapsed_ >= 0.42f && lightParticleModel_) {
		const float particleAge = elapsed_ - 0.42f;
		for (std::size_t i = 0; i < kLightParticleCount; ++i) {
			WorldTransform& light = lightTransforms_[i];
			if (!lightParticlesActive_[i]) {
				lightParticlesActive_[i] = true;
				light.translation_ = {playerTransform_.translation_.x, playerTransform_.translation_.y + 0.35f,
					playerTransform_.translation_.z - 0.15f};
				const float size = 0.16f + 0.035f * static_cast<float>(i % 2);
				light.scale_ = {size, size, size};
			}
			light.translation_.x += lightVelocities_[i].x * kDeltaTime;
			light.translation_.y += lightVelocities_[i].y * kDeltaTime;
			lightVelocities_[i].y -= 0.55f * kDeltaTime;
			const float lightScaleDecay = 1.0f - 0.3f * kDeltaTime;
			light.scale_.x *= lightScaleDecay;
			light.scale_.y *= lightScaleDecay;
			light.scale_.z *= lightScaleDecay;
			UpdateWorldTransform(light);
		}
		lightTint_.w = std::clamp(1.0f - particleAge / 0.95f, 0.0f, 1.0f);
		lightColor_.SetColor(lightTint_);
	}

	if (elapsed_ >= kDuration) {
		isFinished_ = true;
	}
}

void DeatParticles::Draw(Camera* camera) {
	if (isFinished_ || !camera) {
		return;
	}
	if (playerModel_) {
		playerModel_->Draw(playerTransform_, *camera, &playerColor_);
	}
	if (lightParticleModel_ && elapsed_ >= 0.42f) {
		for (std::size_t i = 0; i < kLightParticleCount; ++i) {
			if (lightParticlesActive_[i]) {
				lightParticleModel_->Draw(lightTransforms_[i], *camera, &lightColor_);
			}
		}
	}
}