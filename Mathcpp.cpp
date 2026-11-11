#include "GameScene.h" // MakeAffineMatrixを使用するため
#include "Math.h"

void UpdateWorldTransform(KamataEngine::WorldTransform& worldTransform) {
	// アフィン変換行列を計算してメンバ変数に代入
	worldTransform.matWorld_ = MakeAffineMatrix(worldTransform.scale_, worldTransform.rotation_, worldTransform.translation_);
	// 定数バッファに転送
	worldTransform.TransferMatrix();
}