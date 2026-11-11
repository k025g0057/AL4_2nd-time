#pragma once
#include "KamataEngine.h"
#include "Collision.h"

// KamataEngine:: を省略できるようにする
using namespace KamataEngine;

class Player;
class GameScene;
class ShieldEnemy {
public:

	enum class Behavior {
		kRoot,  // 歩行（通常状態）
		kDeath, // デス演出
		kGuard,   // ガード状態
		kUnknown, // 👑【追加】遷移リクエストがない状態を表す
	};

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(Model* model, Camera* camera, const Vector3& position);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	/// <param name="camera">カメラ</param>
	void Draw(Camera* camera);

	// ワールド変換を取得するゲッター（後でプレイヤーとの当たり判定で使います）
	const WorldTransform& GetWorldTransform() const { return worldTransform_; }

	Vector3 GetWorldPosition();

		// AABBを取得するゲッター関数
	AABB GetAABB();

	void OnCollision(Player* player);

	bool IsDead() const { return isDead_; }

	// 👑【追加】コリジョン無効フラグを取得するゲッター関数
	bool IsCollisionDisabled() const { return isCollisionDisabled_; }

	void SetGameScene(GameScene* gameScene);

	private:
	// 各ビヘイビア（状態）ごとの初期化と更新関数
	void BehaviorRootInitialize();
	void BehaviorRootUpdate();

	void BehaviorDeathInitialize();
	void BehaviorDeathUpdate();

	void BehaviorGuardInitialize();
	void BehaviorGuardUpdate();

private:
	// ワールド変換データ
	WorldTransform worldTransform_;
	ObjectColor objectColor_;
	// モデルのポインタ
	Model* model_ = nullptr;
	// カメラのポインタ
	Camera* camera_ = nullptr;

	// 歩行の速さ（数値はステージに合わせて 0.05f など自由に調整してください）
	static inline const float kWalkSpeed = -0.05f;

		// 敵の移動速度や向き（必要に応じて調整してください）
	Vector3 velocity_ = {kWalkSpeed, 0.0f, 0.0f};

	// 最初の角度[度] (例: -20度傾ける)
	static inline const float kWalkMotionAngleStart = -10.0f;
	// 最後の角度[度] (例: 20度傾ける)
	static inline const float kWalkMotionAngleEnd = 30.0f;
	// アニメーションの周期となる時間[秒] (例: 0.5秒で1往復)
	static inline const float kWalkMotionTime = 0.5f;

	//アニメーションの経過時間タイマー
	float walkTimer_ = 0.0f;

	// 👑デス演出のアニメーション用タイマー
	uint32_t deathTimer_ = 0;

	bool isDead_ = false;

	bool isCollisionDisabled_ = false;

	Behavior behavior_ = Behavior::kRoot;

	// 状態遷移のリクエストを保持する変数
	Behavior behaviorRequest_ = Behavior::kUnknown;
	
	GameScene* gameScene_ = nullptr;

	float guardTimer_ = 0.0f;
};
