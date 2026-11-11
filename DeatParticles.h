#pragma once
#include "KamataEngine.h"
#include <array>
#include <cstddef>

// プレイヤーの死亡時に、倒れて縮みながら光の粒へ変わる演出を管理する。
class DeatParticles {
public:
	void Initialize(KamataEngine::Model* playerModel, KamataEngine::Model* lightParticleModel,
		KamataEngine::Camera* camera, const KamataEngine::WorldTransform& playerTransform);
	void Update();
	void Draw(KamataEngine::Camera* camera);
	bool IsFinished() const { return isFinished_; }

private:
	static inline constexpr std::size_t kLightParticleCount = 6;
	static inline constexpr float kDuration = 1.55f;
	static inline constexpr float kDeltaTime = 1.0f / 60.0f;

	KamataEngine::Model* playerModel_ = nullptr;
	KamataEngine::Model* lightParticleModel_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::WorldTransform playerTransform_;
	KamataEngine::Vector3 originPosition_{};
	KamataEngine::Vector3 originRotation_{};
	KamataEngine::Vector3 initialScale_{};
	std::array<KamataEngine::WorldTransform, kLightParticleCount> lightTransforms_{};
	std::array<KamataEngine::Vector3, kLightParticleCount> lightVelocities_{};
	std::array<bool, kLightParticleCount> lightParticlesActive_{};
	KamataEngine::ObjectColor playerColor_;
	KamataEngine::ObjectColor lightColor_;
	KamataEngine::Vector4 playerTint_{1.0f, 1.0f, 1.0f, 1.0f};
	KamataEngine::Vector4 lightTint_{1.0f, 0.78f, 0.24f, 1.0f};
	float elapsed_ = 0.0f;
	bool isFinished_ = false;
};