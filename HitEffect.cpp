#include "HitEffect.h"
#include "Math.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <random>

KamataEngine::Model* HitEffect::model_ = nullptr;
KamataEngine::Model* HitEffect::spikeModel_ = nullptr;
KamataEngine::Model* HitEffect::starModel_ = nullptr;
KamataEngine::Camera* HitEffect::camera_ = nullptr;

HitEffect* HitEffect::Create(const KamataEngine::Vector3& position, bool isBossDefeat) {
	HitEffect* instance = new HitEffect();
	assert(instance);
	instance->Initialize(position, isBossDefeat);
	return instance;
}

HitEffect* HitEffect::CreateSummon(const KamataEngine::Vector3& position) {
	HitEffect* instance = new HitEffect();
	assert(instance);
	instance->Initialize(position, false);
	instance->isSummonEffect_ = true;
	instance->particleCount_ = 40;
	instance->spikeCount_ = 0;
	instance->starCount_ = 8;
	instance->lifeFrames_ = 54.0f;
	instance->color_ = {0.42f, 0.78f, 1.0f, 1.0f};
	instance->objectColor_.SetColor(instance->color_);
	for (size_t i = 0; i < instance->particleCount_; ++i) {
		Particle& particle = instance->particles_[i];
		particle.transform.Initialize();
		const float angle = 2.0f * 3.14159265f * static_cast<float>(i) / static_cast<float>(instance->particleCount_);
		const float radius = 1.8f + 0.55f * static_cast<float>(i % 3) / 2.0f;
		particle.transform.translation_ = {position.x + std::cos(angle) * radius, position.y + 0.15f, position.z - 0.1f};
		const float speed = 0.06f + 0.025f * static_cast<float>(i % 4);
		particle.velocity = {std::cos(angle) * speed, 0.22f + 0.05f * static_cast<float>(i % 5), 0.0f};
		const float size = 0.24f + 0.07f * static_cast<float>(i % 3);
		particle.transform.scale_ = {size, size, 1.0f};
		UpdateWorldTransform(particle.transform);
	}
	instance->starColor_.SetColor({0.7f, 0.92f, 1.0f, 1.0f});
	for (size_t i = 0; i < instance->starCount_; ++i) {
		Particle& star = instance->stars_[i];
		star.transform.Initialize();
		const float angle = 2.0f * 3.14159265f * static_cast<float>(i) / static_cast<float>(instance->starCount_) + 0.2f;
		star.transform.translation_ = {position.x + std::cos(angle) * 2.1f, position.y + 0.2f, position.z - 0.16f};
		star.velocity = {std::cos(angle) * 0.13f, 0.34f + 0.06f * static_cast<float>(i % 3), 0.0f};
		star.angularVelocity = (i % 2 == 0 ? 1.0f : -1.0f) * 0.14f;
		const float size = 0.7f + 0.15f * static_cast<float>(i % 3);
		star.transform.scale_ = {size, size, 1.0f};
		star.transform.rotation_.z = angle;
		UpdateWorldTransform(star.transform);
	}
	return instance;
}

void HitEffect::Initialize(const KamataEngine::Vector3& position, bool isBossDefeat) {
	frameCount_ = 0.0f;
	isDead_ = false;
	particleCount_ = isBossDefeat ? kMaxParticles : 24;
	spikeCount_ = isBossDefeat ? kMaxSpikes : 6;
	starCount_ = isBossDefeat ? kMaxStars : 7;
	lifeFrames_ = isBossDefeat ? 54.0f : 36.0f;
	isSummonEffect_ = false;

	objectColor_.Initialize();
	color_ = isBossDefeat ? KamataEngine::Vector4{1.0f, 0.55f, 0.12f, 1.0f} : KamataEngine::Vector4{1.0f, 0.82f, 0.24f, 1.0f};
	objectColor_.SetColor(color_);
	starColor_.Initialize();
	starColor_.SetColor({1.0f, 0.86f, 0.08f, 1.0f});

	circleWorldTransform_.Initialize();
	circleWorldTransform_.translation_ = position;
	circleWorldTransform_.scale_ = {0.15f * kEffectSizeMultiplier, 0.15f * kEffectSizeMultiplier, 1.0f};


	std::random_device seedGenerator;
	std::mt19937 randomEngine(seedGenerator());
	const float pi = 3.14159265f;
	std::uniform_real_distribution<float> angleDistribution(0.0f, 2.0f * pi);
	std::uniform_real_distribution<float> particleSpeedDistribution(isBossDefeat ? 0.12f : 0.08f, isBossDefeat ? 0.30f : 0.20f);

	for (size_t i = 0; i < particleCount_; ++i) {
		Particle& particle = particles_[i];
		particle.transform.Initialize();
		particle.transform.translation_ = position;
		const float angle = angleDistribution(randomEngine);
		const float speed = particleSpeedDistribution(randomEngine);
		particle.velocity = {std::cos(angle) * speed, std::sin(angle) * speed, 0.0f};
		const float size = isBossDefeat ? 0.38f : 0.33f;
		particle.transform.scale_ = {size * kEffectSizeMultiplier, size * kEffectSizeMultiplier, 1.0f};
	}

	for (size_t i = 0; i < spikeCount_; ++i) {
		Particle& spike = spikes_[i];
		spike.transform.Initialize();
		spike.transform.translation_ = position;
		const float angle = (2.0f * pi * static_cast<float>(i) / static_cast<float>(spikeCount_)) + angleDistribution(randomEngine) * 0.18f;
		spike.transform.rotation_ = {0.0f, 0.0f, angle - pi / 2.0f};
		const float length = (isBossDefeat ? 8.7f : 7.9f) * (i % 3 == 0 ? 1.0f : (i % 3 == 1 ? 0.75f : 0.62f));
		const float width = i % 3 == 0 ? (isBossDefeat ? 0.36f : 0.24f) : (i % 3 == 1 ? (isBossDefeat ? 0.22f : 0.15f) : (isBossDefeat ? 0.13f : 0.09f));
		spike.transform.scale_ = {width * kEffectSizeMultiplier, length * kEffectSizeMultiplier, 1.0f};
		if (i % 3 == 0) {
			spike.color = {1.0f, 1.0f, 1.0f, 1.0f};
		} else if (i % 3 == 1) {
			spike.color = {1.0f, 0.48f, 0.55f, 1.0f};
		} else {
			spike.color = {0.10f, 0.07f, 0.08f, 1.0f};
		}
		spikeColors_[i].Initialize();
		spikeColors_[i].SetColor(spike.color);
	}

	// 命中時に黄色い星を飛ばし、ハンマーが当たったことを見やすくする。
	std::uniform_real_distribution<float> starSpeedDistribution(isBossDefeat ? 0.38f : 0.32f, isBossDefeat ? 0.58f : 0.52f);
	for (size_t i = 0; i < starCount_; ++i) {
		Particle& star = stars_[i];
		star.transform.Initialize();
		star.transform.translation_ = position;
		const float angle = angleDistribution(randomEngine);
		const float speed = starSpeedDistribution(randomEngine);
		star.velocity = {std::cos(angle) * speed, std::sin(angle) * speed + 0.06f, 0.0f};
		star.angularVelocity = (i % 2 == 0 ? 1.0f : -1.0f) * (0.10f + 0.025f * static_cast<float>(i));
		const float size = isBossDefeat ? 0.90f : 0.72f;
		star.transform.scale_ = {size * kEffectSizeMultiplier, size * kEffectSizeMultiplier, 1.0f};
		star.transform.rotation_.z = angle;
		UpdateWorldTransform(star.transform);
	}

	for (KamataEngine::WorldTransform& ellipse : ellipseWorldTransforms_) {
		ellipse.Initialize();
		ellipse.translation_ = position;
		ellipse.scale_ = {0.30f * kEffectSizeMultiplier, (isBossDefeat ? 4.6f : 3.2f) * kEffectSizeMultiplier, 1.0f};
		ellipse.rotation_.z = angleDistribution(randomEngine);
	}

	UpdateWorldTransform(circleWorldTransform_);
	for (KamataEngine::WorldTransform& ellipse : ellipseWorldTransforms_) {
		UpdateWorldTransform(ellipse);
	}
}

void HitEffect::Update() {
	if (isDead_) {
		return;
	}

	frameCount_ += 1.0f;
	const float fade = std::clamp(1.0f - frameCount_ / lifeFrames_, 0.0f, 1.0f);
	color_.w = fade;
	objectColor_.SetColor(color_);

	const float ringScale = (std::min)(frameCount_, 8.0f) * 0.42f;
	circleWorldTransform_.scale_ = {(0.40f + ringScale) * kEffectSizeMultiplier, (0.40f + ringScale) * kEffectSizeMultiplier, 1.0f};
	for (KamataEngine::WorldTransform& ellipse : ellipseWorldTransforms_) {
		ellipse.scale_.x = (0.30f + ringScale * 0.22f) * kEffectSizeMultiplier;
		ellipse.scale_.y = (std::max)(0.1f * kEffectSizeMultiplier, ellipse.scale_.y - 0.015f * kEffectSizeMultiplier);
		UpdateWorldTransform(ellipse);
	}
	UpdateWorldTransform(circleWorldTransform_);

	for (size_t i = 0; i < particleCount_; ++i) {
		Particle& particle = particles_[i];
		particle.transform.translation_.x += particle.velocity.x;
		particle.transform.translation_.y += particle.velocity.y;
		particle.velocity.x *= 0.97f;
		particle.velocity.y = particle.velocity.y * 0.97f - 0.004f;
		UpdateWorldTransform(particle.transform);
	}
	for (size_t i = 0; i < spikeCount_; ++i) {
		Particle& spike = spikes_[i];
		KamataEngine::Vector4 spikeColor = spike.color;
		spikeColor.w *= fade;
		spikeColors_[i].SetColor(spikeColor);
		UpdateWorldTransform(spike.transform);
	}
	starColor_.SetColor(isSummonEffect_ ? KamataEngine::Vector4{0.7f, 0.92f, 1.0f, fade} : KamataEngine::Vector4{1.0f, 0.86f, 0.08f, fade});
	for (size_t i = 0; i < starCount_; ++i) {
		Particle& star = stars_[i];
		star.transform.translation_.x += star.velocity.x;
		star.transform.translation_.y += star.velocity.y;
		star.velocity.x *= 0.97f;
		star.velocity.y = star.velocity.y * 0.97f - 0.004f;
		star.transform.rotation_.z += star.angularVelocity;
		UpdateWorldTransform(star.transform);
	}

	if (frameCount_ >= lifeFrames_) {
		isDead_ = true;
	}
}

void HitEffect::Draw() {
	if (!camera_) {
		return;
	}

	if (model_) {
		if (!isSummonEffect_) {
			model_->Draw(circleWorldTransform_, *camera_, &objectColor_);
			for (KamataEngine::WorldTransform& ellipse : ellipseWorldTransforms_) {
				model_->Draw(ellipse, *camera_, &objectColor_);
			}
		}
		for (size_t i = 0; i < particleCount_; ++i) {
			model_->Draw(particles_[i].transform, *camera_, &objectColor_);
		}
	}
	if (isSummonEffect_) {
		if (starModel_) {
			for (size_t i = 0; i < starCount_; ++i) {
				starModel_->Draw(stars_[i].transform, *camera_, &starColor_);
			}
		}
		return;
	}
	if (spikeModel_) {
		for (size_t i = 0; i < spikeCount_; ++i) {
			spikeModel_->Draw(spikes_[i].transform, *camera_, &spikeColors_[i]);
		}
	}
	if (starModel_) {
		for (size_t i = 0; i < starCount_; ++i) {
			starModel_->Draw(stars_[i].transform, *camera_, &starColor_);
		}
	}
}
