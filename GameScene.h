#pragma once
#include "KamataEngine.h"
#include <vector>
#include <list>
#include <memory>
#include "Player.h"
#include"MapChipField.h"
#include "CameraController.h"
#include "Enemy.h"
#include "DeatParticles.h"
#include "TitleScene.h"
#include"Fade.h"
#include"HitEffect.h"
#include "ShieldEnemy.h"
#include "GuardEffect.h"
#include "EnemyBullet.h"
#include "Skydome.h"



KamataEngine::Matrix4x4 MakeAffineMatrix(const KamataEngine::Vector3& scale, const KamataEngine::Vector3& rotate, const KamataEngine::Vector3& translate);

class StageManager;

class GameScene {
public:
	void Initialize();
	void Update();
	void Draw();
	~GameScene();

	//すべての当たり判定を管理する関数
	void CheckAllCollisions();

	void CreateHitEffect(const KamataEngine::Vector3& position, bool isBossDefeat = false);
	void CreateHitEffectAtContact(const KamataEngine::Vector3& attackCenter, const AABB& targetAabb, bool isBossDefeat = false);
	void CreateGuardEffect(const KamataEngine::Vector3& position);

	KamataEngine::Model* modelMap_ = nullptr;
	KamataEngine::Model* modelGround_ = nullptr;
	KamataEngine::Model* modelDirtBlock_ = nullptr;
	KamataEngine::Model* modelPlayer_ = nullptr;
	KamataEngine::Model* modelPlayerBody_ = nullptr;
	KamataEngine::Model* modelPlayerLeftLeg_ = nullptr;
	KamataEngine::Model* modelPlayerRightLeg_ = nullptr;
	KamataEngine::Model* modelPlayerHammer_ = nullptr;
	KamataEngine::Model* modelHitEffect_ = nullptr;
	KamataEngine::Model* modelHitSpike_ = nullptr;
	KamataEngine::Model* modelHitStar_ = nullptr;
	KamataEngine::Model* modelGuardEffect_ = nullptr;
	KamataEngine::Sprite* bossTackleWarningSprite_ = nullptr;
	KamataEngine::Sprite* bossAttackAlertSprite_ = nullptr;

	Player* player_ = nullptr;
	//modelという空箱を作成↓〇
	Model* Playermodel = nullptr;

	Model* enemyModel_ = nullptr; // 敵のモデル
	Model* bossBodyModel_ = nullptr;
	Model* bossWingNxModel_ = nullptr;
	Model* bossWingPxModel_ = nullptr;
	WorldTransform independentWingNxTransform_;
	WorldTransform independentWingPxTransform_;
	float bossWingAnimationTime_ = 0.0f;
	Model* summonedEnemyModel_ = nullptr; // ボス召喚用スライムモデル
	std::list<Enemy*> enemies_;   // 敵のリスト
	Enemy* boss_ = nullptr;       // 画面右側に表示する仮ボス
	std::list<EnemyBullet*> enemyBullets_;
	KamataEngine::Model* modelBossBullet_ = nullptr;
	static inline constexpr int kBossMaxHp = 15;
	static inline constexpr int kBossPhaseMaxHp = 5;
	static inline constexpr int kBossPhaseTwoHp = 10;
	static inline constexpr int kBossPhaseThreeHp = 5;
	int bossHp_ = kBossMaxHp;
	bool bossHitThisAttack_ = false;
	int hitStopFrames_ = 0;
	float bossShotTimer_ = 0.0f;
	bool bossTelegraphActive_ = false;
	KamataEngine::Vector3 bossAttackDirection_{};
	float bossAttackLineLength_ = 0.0f;

	bool IsFinished() const { return finished_; }

	bool IsReloadRequested() const { return reloadRequested_; }
	
	void Initialize(StageManager* stageDataManager);

private:
	void ChangePhase();
	void InitializeForestBackground();
	void DrawForestBackground();
	void DrawSummonMagicCircles();
	struct SummonMagicCircle {
		KamataEngine::WorldTransform worldTransform;
		KamataEngine::ObjectColor objectColor;
		float age = 0.0f;
	};
	struct ForestBackgroundBlock {
		KamataEngine::WorldTransform worldTransform;
		KamataEngine::ObjectColor objectColor;
		float layoutX = 0.0f;
		float layoutY = 0.0f;
		float layoutWidth = 0.0f;
		float layoutHeight = 0.0f;
		float baseWorldX = 0.0f;
		float tileWorldWidth = 0.0f;
		float parallax = 0.0f;
		float worldZ = 0.0f;
		bool usesTreeModel = false;
		bool usesGrassModel = false;
		bool isBackgroundGround = false;
		bool isGroundTile = false;
	};

	void GenerateFieldObjects();
	void AppendGeneratedFieldColumns();
	void DrawBossIntroConcentrationLines();
	void DrawBossAttackTelegraph();
	void DrawBossTackleTelegraph();
	void UpdateBossAttack();
	void UpdateBossPhase();
	void SummonNormalEnemy(float screenX);
	void UpdateEnemyBullets();

	void GenerateEnemyFromMapChip(uint8_t subID, uint32_t xIndex, uint32_t yIndex);

	StageManager* stageManager_ = nullptr;

	uint32_t textureHandle_ = 0;
	uint32_t textureHandleWhite_ = 0;
	KamataEngine::Model* modelBlock_ = nullptr; // デバッグ用
	KamataEngine::Model* modelForestTree_ = nullptr;
	KamataEngine::Model* modelForestGrass_ = nullptr;
	KamataEngine::Model* modelSummonMagicCircle_ = nullptr;
	KamataEngine::Model* modelSkyDome_ = nullptr;
	Skydome* skydome_ = nullptr;

	std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransformBlocks_;
	std::vector<std::unique_ptr<ForestBackgroundBlock>> forestBackgroundBlocks_;
	std::list<std::unique_ptr<SummonMagicCircle>> summonMagicCircles_;
	float forestBackgroundOriginCameraX_ = 0.0f;
	uint32_t generatedBlockColumnCount_ = 0;

	DeatParticles* deathParticles_ = nullptr;void GenerateEnemy(uint8_t subID, uint32_t xIndex, uint32_t yIndex);
	// --- プレイヤー用のワールド変換データを追加 ---
	KamataEngine::WorldTransform worldTransformPlayer_;

	KamataEngine::Camera camera_;

	// マップチップフィールド
	MapChipField* mapChipField_;

	// シールドエネミー専用モデルとリスト
	KamataEngine::Model* shieldEnemyModel_ = nullptr;
	std::list<ShieldEnemy*> shieldEnemies_;


	std::unique_ptr<CameraController> cameraController_;
	KamataEngine::Vector3 gameplayCameraPosition_{};
	KamataEngine::Vector3 introCameraPosition_{};
	float bossIntroTimer_ = 0.0f;
	enum class BossPhase { kPhaseOne, kTransition, kPhaseTwo, kPhaseThree };
	enum class BossTackleState { kWaiting, kTelegraph, kDashing, kReturning };
	BossPhase bossPhase_ = BossPhase::kPhaseOne;
	bool bossTransitionToPhaseThree_ = false;
	bool bossTransitionRoarStarted_ = false;
	static inline constexpr float kBossPhaseBarRefillDuration = 2.5f;
	BossTackleState bossTackleState_ = BossTackleState::kWaiting;
	float bossPhaseTimer_ = 0.0f;
	float bossSummonTimer_ = 0.0f;
	float bossSummonRoarTimer_ = 0.0f;
	float bossTackleTimer_ = 0.0f;
	float bossTackleStartX_ = 0.78f;
	float bossTackleStartY_ = -0.5f;
	float bossTackleTargetX_ = -0.8f;
	float bossTackleTargetY_ = -0.5f;
	bool bossTackleHitPlayer_ = false;
	AABB bossTacklePreviousAabb_{};
	bool bossTacklePreviousAabbValid_ = false;




	// ゲームのフェーズ（型）
	enum class Phase {
		kFadeIn, // フェードイン
		kBossIntroDrop,
		kBossIntroLand,
		kBossIntroRoar,
		kBossIntroCameraReturn,
		kPlay,  // ゲームプレイ
		kDeath, // デス演出
		kFadeOut, // フェードアウト
		kStageClear // ボス撃破後のクリア表示
	};

	// ゲームの現在フェーズ（変数）
	Phase phase_;

	bool finished_ = false;

	Fade* fade_ = nullptr;

	std::list<HitEffect*> hitEffects_;

	std::list<GuardEffect*> guardEffects_;

	// リロード要求フラグ
	bool reloadRequested_ = false;
	bool isPaused_ = false;
	int pauseMenuSelection_ = 0;
	KamataEngine::Sprite* pauseMenuSprites_[3] = {};
	KamataEngine::Sprite* pauseHintSprite_ = nullptr;
	KamataEngine::Sprite* attackHintSprite_ = nullptr;
	KamataEngine::Sprite* evadeHintSprite_ = nullptr;
	KamataEngine::Sprite* bossNameSprite_ = nullptr;
	KamataEngine::Sprite* bossHpBarFrameSprite_ = nullptr;
	KamataEngine::Sprite* bossHpBarFillSprite_ = nullptr;
	KamataEngine::Sprite* dodgeCooldownFrameSprite_ = nullptr;
	KamataEngine::Sprite* dodgeCooldownBackSprite_ = nullptr;
	KamataEngine::Sprite* dodgeCooldownFillSprite_ = nullptr;
	KamataEngine::Sprite* playerHpSprites_[Player::GetMaxHP()] = {};
	float elapsedGameplayTime_ = 0.0f;
	int clearMenuSelection_ = 0;
	KamataEngine::Sprite* clearMenuSprites_[2] = {};
	KamataEngine::Sprite* clearBackgroundShadeSprite_ = nullptr;
	KamataEngine::Sprite* clearTimeDigitSprites_[5] = {};
	KamataEngine::WorldTransform defeatedBossTransform_;

	KamataEngine::DebugCamera* debugCamera_ = nullptr;
	bool isDebugCameraActive_ = false;
};
