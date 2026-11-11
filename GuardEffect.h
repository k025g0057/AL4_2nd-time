#pragma once
#include "KamataEngine.h"
#include <array>

    /// <summary>
/// ヒットエフェクトを管理するクラス
/// </summary>
class GuardEffect {
public:
	static GuardEffect* Create(const KamataEngine::Vector3& position);
	/// <summary>
	/// 初期化
	/// </summary.>
	void Initialize(const KamataEngine::Vector3& position);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	// 全インスタンス共通のモデルとカメラをセットする静的関数
	static void SetModel(KamataEngine::Model* model) { model_ = model; }
	static void SetCamera(KamataEngine::Camera* camera) { camera_ = camera; }

	bool IsDead() const { return isDead_; }

private:

	enum class Behavior {
		kSpread,  // ① 拡大アニメーション（スプレッド）
		kFadeOut, // ② フェードアウトで消える
	};
	
	static KamataEngine::Model* model_;   // ヒットエフェクトのモデル
	static KamataEngine::Camera* camera_; // カメラ

	KamataEngine::WorldTransform circleWorldTransform_;

	static const size_t kNumEllipses = 2;

	//std::array で複数のトランスフォームを管理！
	std::array<KamataEngine::WorldTransform, kNumEllipses> ellipseWorldTransforms_;

	Behavior behavior_ = Behavior::kSpread; // 現在の状態
	float frameCount_ = 0.0f;               // カウンタ（タイマー）
	bool isDead_ = false;                   // インスタンス消滅フラグ


	KamataEngine::ObjectColor objectColor_;
	// 色の数値
	KamataEngine::Vector4 color_;
};