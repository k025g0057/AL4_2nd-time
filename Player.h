#pragma once
#include "KamataEngine.h"
#include"Collision.h"
#include"Easing.h"

// KamataEngine:: を省略できるようにする
using namespace KamataEngine;

class MapChipField;

enum class LR {
	kRight,
	kLeft,
};

enum class Behavior {
	kRoot,   // 通常状態
	kDodge, // 回避中（旧突進アニメーションを使用）
	kSpinAttack, // ハンマー攻撃中（既存の状態名を維持）
	kKnockback, // 👈【追加】ノックバック状態
	kUnknown,
};

enum class KnockbackPhase {
	kFling,   // 👈【追加】「強い初速で弾き飛ばされる」フェーズ
	kRecover, // 👈【追加】「移動が停止し、体勢を立て直す」フェーズ
};

class Enemy;

class Player {
public:

	struct CollisionMapInfo {
		bool isCeilingHit = false; // 天井衝突フラグ
		bool isLanding = false;    // 着地フラグ
		bool isWallHit = false;    // 壁接触フラグ
		Vector3 velocity;          // 移動量
	};

	


	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="model">モデルデータ</param>
	void Initialize(Model* model, Model* modelLeftLeg, Model* modelRightLeg, Model* modelHammer, Camera* camera, const Vector3& position);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();
	void SetControlsLocked(bool locked) { controlsLocked_ = locked; }

	/// <summary>
	/// 移動入力処理
	/// </summary>
	void Move();
	void SetAutoRunSpeed(float speed) { autoRunSpeed_ = speed; }

	/// <summary>
	/// 描画
	/// </summary>
	/// <param name="camera">カメラ</param>
	void Draw(KamataEngine::Camera* camera);

	//MapChipFieldを登録するためのSetter
	void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }

	const KamataEngine::WorldTransform& GetWorldTransform() const { return worldTransform_; }

		const KamataEngine::Vector3& GetVelocity() const { return velocity_; }

	// 判定結果を反映して移動させる関数
	    void MoveWithCollision(const CollisionMapInfo& info);

		// 天井に接触している場合の応答処理関数
	    void OnCeilingCollision(const CollisionMapInfo& info);

		void OnGroundToggle(const CollisionMapInfo& info); // ⑥接地状態の切り替え処理

		// ⑤壁に接触している場合の処理の関数を宣言】
	    void OnWallCollision(const CollisionMapInfo& info);

		// 地面判定のために下にずらす微小な数値（調整して決めてください）
	    static inline const float kSubsideDistance = 0.01f;

		//ワールド座標を取得するゲッター関数
	    Vector3 GetWorldPosition() const;

		//AABBを取得するゲッター関数
	     AABB GetAABB();

		 void OnCollision(const Enemy* enemy);

		 void OnCollision(class ShieldEnemy* shieldEnemy);

		 bool IsDead() const { return isDead_; }
		 int GetHP() const { return hp_; }
		 static constexpr int GetMaxHP() { return kMaxHP; }

		 bool IsAttack() const { return IsSpinAttacking(); }
		 bool IsDodging() const { return behavior_ == Behavior::kDodge; }
		 float GetDodgeCooldownRatio() const { return 1.0f - dodgeCooldownTimer_ / kDodgeCooldownDuration; }
		 bool IsSpinAttacking() const { return behavior_ == Behavior::kSpinAttack; }
		 bool IsSpinAttackHit(const AABB& target) const;
		 bool IsKnockback() const { return behavior_ == Behavior::kKnockback; }
		 bool IsOnGround() const { return onGround_; }

		bool IsFacingRight() const {
		     // 右向きの時は 1.57(プラス) になるので、0.0f より大きければ「右向き」と判定する
		    return lr_ == LR::kRight;
	    }

		void ActivateKnockback();
		void StartBossHitKnockback();
		void CompleteBossHitKnockback();

		void KnockbackRequest() {
		    knockbackRequest_ = true; // フラグを立てるだけ！
	    }

	private:
	    enum Corner {
		    kRightBottom, // 右下 (0)
		    kLeftBottom,  // 左下 (1)
		    kRightTop,    // 右上 (2)
		    kLeftTop,     // 左上 (3)

		    kNumCorner // 要素数 (4)
	    };

		//指定した角の座標を計算して得るメンバ関数
	    Vector3 CornerPosition(const Vector3& center, Corner corner);


	// ワールド変換データ
	KamataEngine::WorldTransform worldTransform_;
	// 歩行アニメーション用の描画専用変換（物理・当たり判定には影響させない）
	KamataEngine::WorldTransform worldTransformRender_;
	KamataEngine::WorldTransform worldTransformLeftLeg_;
	KamataEngine::WorldTransform worldTransformRightLeg_;
	KamataEngine::ObjectColor objectColor_;
	// モデル（外部から受け取ったポインタを保持）
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Model* modelLeftLeg_ = nullptr;
	KamataEngine::Model* modelRightLeg_ = nullptr;

	KamataEngine::Model* modelHammer_ = nullptr;
	KamataEngine::WorldTransform worldTransformHammer_;
	float hammerAngle_ = 0.0f;
	bool hammerHitActive_ = false;
	AABB hammerAttackAabb_{};

	KamataEngine::Camera* camera_ = nullptr;

	//マップチップフィールドのポインタを保持するメンバ変数 ---
	MapChipField* mapChipField_ = nullptr;

	::Vector3 velocity_ = {}; // プレイヤーの速度（例として追加）
	float walkAnimationTime_ = 0.0f;
	float autoRunSpeed_ = 0.0f; // 画面端でカメラに追いつく速度
	bool controlsLocked_ = false;
	bool scrollPushActive_ = false; // 今フレームにスクロールから押されているか

	static inline const float kWidth = 3.0f;
	static inline const float kHeight = 3.0f;

	static inline const float kBlank = 0.001f;


	// 通常の左右移動の加速
	static inline constexpr float kMoveAcceleration = 0.012f;
	//playerの移動速度委の変更はここの数値をいじれば変わるよ　加速
	static inline const float kAcceleration = 0.02f;

	//減少
	static inline const float kAttenuation = 0.2f;

	// 最大速度の制限
	static inline const float kLimitVelocity = 0.58f;
	static inline const float kBossHitKnockbackSpeed = 2.0f;
	static inline const float kBossHitKnockbackUpSpeed = 1.2f;
	static inline constexpr float kDodgeCooldownDuration = 5.0f;
	float dodgeCooldownTimer_ = 0.0f;

	// 着地時の速度減衰率
	static inline const float kAttenuationLanding = 0.1f;

	// 着地時の速度減衰率
	static inline const float kAttenuationWall = 0.2f;

	//static inline const float kLimitRunSpeed = 0.6f; // 走るときの最大速度（例として追加）

	LR lr_ = LR::kRight;

	// 現在の振るまい（状態）を管理する変数（初期値は通常状態）
	Behavior behavior_ = Behavior::kRoot;

	Behavior behaviorRequest_ = Behavior::kUnknown;

	bool knockbackRequest_ = false;
	bool bossHitKnockback_ = false;

	float turnFirstRotationY_ = 0.0f; // 振り向き開始時の角度
	float turnTimer_ = 0.0f;          // 振り向きの経過時間(0.0〜1.0)
	float spinStartRotationZ_ = 0.0f; // 回転攻撃開始時のZ回転

	// 振り向きにかける時間（スライドの設定に合わせて調整してください）
	// 例: 0.1f なら約6フレームで振り向き完了
	static inline const float kTurnSpeed = 0.1f;

	bool onGround_ = true; // 地面にいるかどうかのフラグ

	bool hasAirAttacked_ = false;

	// --- ジャンプ用の定数 ---
	static inline const float kJumpSpeed = 0.8f; // ジャンプ初速
	static inline const float kGravity = 0.02f;  // 重力
	static inline const float kGroundY = 3.5f;   // 地面の高さ

	//衝突判定関数
	void MapCollision(CollisionMapInfo& info);

	//新しく小分けにする4つの方向の関数
	void MapCollisionUp(CollisionMapInfo& info);    // 上方向
	void MapCollisionDown(CollisionMapInfo& info);  // 下方向
	void MapCollisionRight(CollisionMapInfo& info); // 右方向
	void MapCollisionLeft(CollisionMapInfo& info);  // 左方向

	bool isDead_ = false;
	static inline constexpr int kMaxHP = 3;
	int hp_ = kMaxHP;
	int damageInvulnerabilityFrames_ = 0;
	static inline constexpr int kDamageInvulnerabilityDuration = 60;
	void ApplyDamage();
	
	// 通常行動の更新関数
	void BehaviorRootUpdate();

	void BehaviorRootInitialize();
	void BehaviorDodgeInitialize();
	void BehaviorSpinAttackInitialize();
	void BehaviorDodgeUpdate();
	void BehaviorSpinAttackUpdate();
	void UpdateHammerTransform(const KamataEngine::WorldTransform& rootTransform);

	void BehaviorKnockbackInitialize();
	void BehaviorKnockbackUpdate();
	void ApplyScrollPush();

	// 画面左端から約1%以内に来たときだけ、スクロールに押される。
	static inline const float kScrollPushScreenX = -0.95f;
	static inline const float kScrollCatchUpRate = 0.1f;
	static inline constexpr uint32_t kHammerAttackDuration = 36;

	// 攻撃ギミックの経過時間カウンターを追加
	uint32_t attackParameter_ = 0;

	KnockbackPhase knockbackPhase_; // 現在のノックバックフェーズ
	float knockbackTimer_ = 0.0f;   // ノックバックの時間を計るタイマー
};
