#pragma once
#include "KamataEngine.h"


class Skydome {

	public:
	void Initialize(KamataEngine::Model*model);

	void Update(const KamataEngine::Vector3& cameraPosition);

	void Draw(KamataEngine::Camera*camera);

	private:
		//ワールド変換データ
	    KamataEngine::WorldTransform worldTransform_;
		//モデル
	    KamataEngine::Model* model_ = nullptr;

	
};
