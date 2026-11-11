#include "GuardEffect.h"
#include <cassert>
#include"Math.h"
#include <random>
#include <algorithm>

KamataEngine::Model* GuardEffect::model_ = nullptr;
KamataEngine::Camera* GuardEffect::camera_ = nullptr;

GuardEffect* GuardEffect::Create(const KamataEngine::Vector3& position) {
	// インスタンス生成
	GuardEffect* instance = new GuardEffect();

	// newの失敗を検出
	assert(instance);

	// インスタンスの初期化
	instance->Initialize(position);

	// 初期化したインスタンスを返す
	return instance;
}

void GuardEffect::Initialize(const KamataEngine::Vector3& position) {

	//状態遷移用・消滅用の変数を初期状態にリセット
	behavior_ = Behavior::kSpread; // 初期状態は「拡大（スプレッド）」
	frameCount_ = 0.0f;            // カウンタを0にリセット
	isDead_ = false;               // 消滅フラグを初期化

	objectColor_.Initialize();

	color_ = {1.0f, 1.0f, 1.0f, 1.0f};


	// ---------------------------------------------------------
	// 前回の丸いエフェクトの初期化】
	// ---------------------------------------------------------
	circleWorldTransform_.Initialize();
	circleWorldTransform_.translation_ = position;
	circleWorldTransform_.scale_ = {3.0f, 3.0f, 3.0f}; // 前回のサイズ指定

	// ---------------------------------------------------------
	// 楕円エフェクトの初期化】
	// ---------------------------------------------------------
	// 【1. 共通の初期化】
	std::random_device seedGenerator;
	std::mt19937_64 randomEngine;
	randomEngine.seed(seedGenerator());

	// 【2. 乱数範囲ごとの初期化】
	float pi = 3.141592f;
	std::uniform_real_distribution<float> rotationDistribution(-pi, pi);

	// 範囲for文を使って複数の楕円を一括で初期化
	for (KamataEngine::WorldTransform& worldTransform : ellipseWorldTransforms_) {

		// スライドの手順通り、まず最初に行列を初期化（リセット）
		worldTransform.Initialize();

		// そのあとで、スライドの指示通りの数値を代入
		worldTransform.scale_ = {0.2f, 6.0f, 1.0f}; // 楕円の幅と長さ

		// 【3. 乱数の取得】
		float randomZ = rotationDistribution(randomEngine);
		worldTransform.rotation_ = {0.0f, 0.0f, randomZ}; // -π~+πの乱数

		worldTransform.translation_ = position; // 発生座標
	}
}

void GuardEffect::Update() {
	frameCount_ += 1.0f;

	// 状態（Behavior）ごとの切り替え機構
	switch (behavior_) {
	case Behavior::kSpread: // ① 拡大アニメーション（スプレッド）

		// 👑【スプレッド】10フレームかけてサイズを勢いよく拡大させる
		circleWorldTransform_.scale_.x += 0.3f;
		circleWorldTransform_.scale_.y += 0.3f;
		circleWorldTransform_.scale_.z += 0.3f;

		for (KamataEngine::WorldTransform& worldTransform : ellipseWorldTransforms_) {
			// 楕円は縦（y）にシュッと伸びるように拡大
			worldTransform.scale_.x += 0.05f;
			worldTransform.scale_.y += 0.6f;
		}

		// 10フレーム経過したら、フェード（縮小）状態へ切り替える
		if (frameCount_ >= 10.0f) {
			behavior_ = Behavior::kFadeOut;
			frameCount_ = 0.0f; // 次の状態のためにタイマーを0にリセット
		}
		break;

	case Behavior::kFadeOut: // ② フェードアウト（だんだん小さくなって消える）

		

		color_.w = std::clamp(1.0f - (frameCount_ / 30.0f), 0.0f, 1.0f);

		// 計算した色をオブジェクトカラーに設定・転送する
		objectColor_.SetColor(color_);

		// 30フレーム経過したら、完全に消滅フラグを立てる！
		if (frameCount_ >= 30.0f) {
			isDead_ = true; // これでGameScene側が自動削除してくれます
		}
		break;
	}

	// ---------------------------------------------------------
	// 行列の更新（前回の完璧なループ処理はここに残します）
	// ---------------------------------------------------------
	// 丸いエフェクトの更新
	UpdateWorldTransform(circleWorldTransform_);

	// 楕円エフェクトの更新
	for (KamataEngine::WorldTransform& worldTransform : ellipseWorldTransforms_) {
		UpdateWorldTransform(worldTransform);
	}
}

void GuardEffect::Draw() {
	if (model_ && camera_) {
		
		// ② 楕円エフェクトの描画
		for (const KamataEngine::WorldTransform& worldTransform : ellipseWorldTransforms_) {
			model_->Draw(worldTransform, *camera_, &objectColor_);
		}

		// ① 丸いエフェクトの描画
		model_->Draw(circleWorldTransform_, *camera_, &objectColor_);
	}
}