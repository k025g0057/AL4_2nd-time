#pragma once
#include "KamataEngine.h"
#include "Fade.h"
#include <cstdint>
#include <list>

class TitleScene {
public:
	void Initialize();
	void Update();
	void Draw();
	~TitleScene();

	bool IsFinished() const { return finished_; }

private:
	struct BackgroundProp {
		KamataEngine::WorldTransform transform;
		float screenX = 0.0f;
		float width = 0.0f;
		float height = 0.0f;
		float groundY = 0.0f;
		float parallax = 1.0f;
		float z = 0.0f;
		bool isGrass = false;
	};

	struct GroundTile {
		KamataEngine::WorldTransform grass;
		KamataEngine::WorldTransform dirt;
	};

	struct PreviewEnemy {
		KamataEngine::WorldTransform transform;
		float screenX = 0.0f;
		float deathTimer = -1.0f;
		uint32_t id = 0;
	};

	void SetScreenTransform(KamataEngine::WorldTransform& transform, float screenX, float screenY,
		float width, float height, float z, float depthScale = 1.0f);
	void UpdatePlayerPose();
	void DrawPlayer();
	void DrawBackground();

	KamataEngine::Model* modelSkyDome_ = nullptr;
	KamataEngine::Model* modelTree_ = nullptr;
	KamataEngine::Model* modelGrass_ = nullptr;
	KamataEngine::Model* modelGround_ = nullptr;
	KamataEngine::Model* modelDirtBlock_ = nullptr;
	KamataEngine::Model* modelPlayerBody_ = nullptr;
	KamataEngine::Model* modelPlayerLeftLeg_ = nullptr;
	KamataEngine::Model* modelPlayerRightLeg_ = nullptr;
	KamataEngine::Model* modelPlayerHammer_ = nullptr;
	KamataEngine::Model* modelEnemy_ = nullptr;
	KamataEngine::Model* modelTitleLogo_ = nullptr;
	KamataEngine::Model* modelTitleBossBody_ = nullptr;
	KamataEngine::Model* modelTitleBossWingNx_ = nullptr;
	KamataEngine::Model* modelTitleBossWingPx_ = nullptr;
	KamataEngine::Sprite* menuStartSprite_ = nullptr;
	KamataEngine::Sprite* menuExitSprite_ = nullptr;

	KamataEngine::WorldTransform worldTransformSky_;
	KamataEngine::WorldTransform worldTransformPlayer_;
	KamataEngine::WorldTransform worldTransformLeftLeg_;
	KamataEngine::WorldTransform worldTransformRightLeg_;
	KamataEngine::WorldTransform worldTransformHammer_;
	KamataEngine::WorldTransform titleLogoTransform_;
	KamataEngine::WorldTransform titleBossBodyTransform_;
	KamataEngine::WorldTransform titleBossWingNxTransform_;
	KamataEngine::WorldTransform titleBossWingPxTransform_;

	std::list<BackgroundProp> backgroundProps_;
	std::list<GroundTile> groundTiles_;
	std::list<PreviewEnemy> previewEnemies_;
	KamataEngine::Camera camera_;
	float animationTimer_ = 0.0f;
	float groundScrollPixels_ = 0.0f;
	float enemySpawnTimer_ = 0.0f;
	float attackCooldown_ = 0.0f;
	float attackAnimationTimer_ = -1.0f;
	uint32_t nextEnemyId_ = 1;
	uint32_t attackTargetId_ = 0;
	float playerScreenX_ = 250.0f;
	float groundTopPixels_ = 620.0f;
	int menuSelection_ = 0;

	bool finished_ = false;
	Fade* fade_ = nullptr;

	enum class Phase {
		Main,
		FadeIn,
		FadeOut,
	};

	Phase phase_ = Phase::FadeIn;
};