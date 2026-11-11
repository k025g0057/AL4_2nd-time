#pragma once
#include "KamataEngine.h"
#include <random>

// --- スライド「グローバル」：前方宣言 ---
class Player;

/// <summary>
/// カメラコントローラ
/// </summary>
class CameraController {
public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(KamataEngine::Camera* camera);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();
	void StartShake(int frames, float strength);
	void UpdateShake();

	// --- スライド：外部からポインタをセットするためのsetter ---
	void SetTarget(Player* target) { target_ = target; }

	void Reset();

	// 前回のカメラ用Setterも必要に応じて追加（スライドにはありませんが、運用上必要になります）---------------
	//void SetCamera(KamataEngine::Camera* camera) { camera_ = camera; }

	// 矩形
	struct Rect {
		float left = 0.0f;   // 左端
		float right = 1.0f;  // 右端
		float bottom = 0.0f; // 下端
		float top = 1.0f;    // 上端
	};

	void SetMovableArea(const Rect& movableArea) { movableArea_ = movableArea; }

	// 強制スクロールの速度（ワールド座標 / フレーム）
	void SetScrollSpeed(float speed) { scrollSpeed_ = speed; }
	float GetScrollSpeed() const { return scrollSpeed_; }
	void PauseScroll();
	void SetTransitionScrollLock(bool locked);

private:
	float Lerp(float a, float b, float t) { return (b - a) * t + a; }

	KamataEngine::Camera* camera_ = nullptr;

	// --- スライド：追従対象となるプレイヤーのポインタ ---
	Player* target_ = nullptr;
	//追従対象とカメラの座標の差（オフセット） ---
	KamataEngine::Vector3 targetOffset_ = {0.0f, 23.0f, -70.0f};

	KamataEngine::Vector3 targetPos_ = {0.0f, 0.0f, 0.0f};

	static inline const float kInterpolationRate = 0.1f;

	// 目的地となる座標を追加
	//  --- カメラの移動範囲制限 ---
	Rect movableArea_ = {-50.0f, 348.0f, -100.0f, 100.0f};

	// 速度掛け率
	static inline const float kVelocityBias = 20.0f;
	float scrollSpeed_ = 0.05f;
	bool isScrollPaused_ = false;
	bool isTransitionScrollLocked_ = false;
	float scrollPauseTimer_ = 0.0f;
	int shakeFrames_ = 0;
	float shakeStrength_ = 0.0f;
	KamataEngine::Vector3 shakeOffset_{};
	std::mt19937 randomEngine_{std::random_device{}()};

	static inline const Rect kMargin = {-50.0f, 348.0f, -100.0f, 100.0f};

};
