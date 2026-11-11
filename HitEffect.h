#pragma once
#include "KamataEngine.h"
#include <array>
#include <cstddef>

/// <summary>ヒット演出を管理するクラス。</summary>
class HitEffect {
public:
	static HitEffect* Create(const KamataEngine::Vector3& position, bool isBossDefeat = false);
	static HitEffect* CreateSummon(const KamataEngine::Vector3& position);
	void Initialize(const KamataEngine::Vector3& position, bool isBossDefeat = false);
	void Update();
	void Draw();

	static void SetModel(KamataEngine::Model* model) { model_ = model; }
	static void SetSpikeModel(KamataEngine::Model* model) { spikeModel_ = model; }
	static void SetStarModel(KamataEngine::Model* model) { starModel_ = model; }
	static void SetCamera(KamataEngine::Camera* camera) { camera_ = camera; }
	bool IsDead() const { return isDead_; }

private:
	struct Particle {
		KamataEngine::WorldTransform transform;
		KamataEngine::Vector3 velocity{};
		KamataEngine::Vector4 color{1.0f, 1.0f, 1.0f, 1.0f};
		float angularVelocity = 0.0f;
	};

	static KamataEngine::Model* model_;
	static KamataEngine::Model* spikeModel_;
	static KamataEngine::Model* starModel_;
	static KamataEngine::Camera* camera_;

	static constexpr float kEffectSizeMultiplier = 1.5f;
	static constexpr size_t kMaxParticles = 64;
	static constexpr size_t kMaxSpikes = 12;
	static constexpr size_t kMaxStars = 12;
	std::array<Particle, kMaxParticles> particles_{};
	std::array<Particle, kMaxSpikes> spikes_{};
	std::array<Particle, kMaxStars> stars_{};
	std::array<KamataEngine::ObjectColor, kMaxSpikes> spikeColors_{};
	size_t particleCount_ = 0;
	size_t spikeCount_ = 0;
	size_t starCount_ = 0;

	KamataEngine::WorldTransform circleWorldTransform_;
	static constexpr size_t kNumEllipses = 2;
	std::array<KamataEngine::WorldTransform, kNumEllipses> ellipseWorldTransforms_;

	float frameCount_ = 0.0f;
	float lifeFrames_ = 36.0f;
	bool isDead_ = false;
	bool isSummonEffect_ = false;
	KamataEngine::ObjectColor objectColor_;
	KamataEngine::ObjectColor starColor_;
	KamataEngine::Vector4 color_{};
};
