#include "Skydome.h"
#include "GameScene.h" 
#include "KamataEngine.h"
#include "Math.h" 

using namespace KamataEngine;

void Skydome::Initialize(KamataEngine::Model* model) {
	model_ = model;
	worldTransform_.Initialize();
	worldTransform_.translation_ = {0.0f, 0.0f, 0.0f};

	// 共通関数で計算と転送を行う
	UpdateWorldTransform(worldTransform_);
}

void Skydome::Update(const KamataEngine::Vector3& cameraPosition) {
	// カメラを球の中心に保ち、無限スクロールでも空が途切れないようにする。
	worldTransform_.translation_ = cameraPosition;
	UpdateWorldTransform(worldTransform_);
}

void Skydome::Draw(KamataEngine::Camera* camera) {
	// モデルがセットされている場合のみ描画を実行
	if (model_) {
		model_->Draw(worldTransform_, *camera);
	}
}
