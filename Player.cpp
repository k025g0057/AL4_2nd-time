#include "Player.h"
#include "UiSound.h"
#include "Math.h"
#include <cmath>
#include <numbers>
#include <algorithm>
#include <array>   
#include "MapChipField.h"


using namespace KamataEngine;

void Player::Initialize(Model* model, Model* modelLeftLeg, Model* modelRightLeg, Model* modelHammer, Camera* camera, const Vector3& position) {
	// モデルを保持
	model_ = model;
	modelLeftLeg_ = modelLeftLeg;
	modelRightLeg_ = modelRightLeg;

	modelHammer_ = modelHammer;

	camera_ = camera;
	hp_ = kMaxHP;
	damageInvulnerabilityFrames_ = 0;
	dodgeCooldownTimer_ = 0.0f;
	isDead_ = false;

	// ワールド変換の初期化
	worldTransform_.Initialize();
	worldTransformRender_.Initialize();
	worldTransformLeftLeg_.Initialize();
	worldTransformRightLeg_.Initialize();
	objectColor_.Initialize();
	objectColor_.SetColor({1.0f, 1.0f, 1.0f, 1.0f});

	worldTransformHammer_.Initialize();

	// スケールの設定
	worldTransform_.scale_ = {5.0f, 5.0f, 5.0f};

	// 向き（回転）の設定
	worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f;

	// --- ここを修正：固定の数値 {1.5f, ...} ではなく position を使う ---
	worldTransform_.translation_ = position;

	// 初期化時にも行列を計算・転送しておく
	UpdateWorldTransform(worldTransform_);

	UpdateHammerTransform(worldTransform_);
}

void Player::Update() {
	if (dodgeCooldownTimer_ > 0.0f) {
		dodgeCooldownTimer_ -= 1.0f / 60.0f;
		if (dodgeCooldownTimer_ < 0.0f) {
			dodgeCooldownTimer_ = 0.0f;
		}
	}

	if (damageInvulnerabilityFrames_ > 0) {
		--damageInvulnerabilityFrames_;
	}

	if (knockbackRequest_) {
		behaviorRequest_ = Behavior::kKnockback;
		knockbackRequest_ = false; // フラグをリセット
	}

	if (onGround_) {
		hasAirAttacked_ = false;
	}
	
	if (behaviorRequest_ != Behavior::kUnknown) {
		// 遷移前の状態の終了処理（必要に応じて）
		behavior_ = behaviorRequest_;
		behaviorRequest_ = Behavior::kUnknown;

		// 各状態の初期化処理
		switch (behavior_) {
		case Behavior::kRoot:
			BehaviorRootInitialize();
			break;
		case Behavior::kDodge:
			BehaviorDodgeInitialize();
			break;
		case Behavior::kSpinAttack:
			BehaviorSpinAttackInitialize();
			break;
		case Behavior::kKnockback: // 👈【修正】ノックバック初期化を確実に呼ぶ！
			BehaviorKnockbackInitialize();
			break;
		}
	}

	// 各状態の更新処理
	switch (behavior_) {
	case Behavior::kRoot:
		BehaviorRootUpdate();
		break;
	case Behavior::kDodge:
		BehaviorDodgeUpdate();
		break;
	case Behavior::kSpinAttack:
		BehaviorSpinAttackUpdate();
		break;
	case Behavior::kKnockback: //ノックバック更新を毎フレーム呼ぶ！
		BehaviorKnockbackUpdate();
		BehaviorRootUpdate();
		break;
	}

	
}



void Player::Move() {
	// --- 1. ジャンプ・重力処理 (スライドの if (onGround_) 形式) ---
	if (onGround_) {
		// 接地中：ジャンプ入力チェック
		if (!controlsLocked_ && Input::GetInstance()->PushKey(DIK_W)) {
			velocity_.y = kJumpSpeed;
			onGround_ = false; // 空中へ
		}
	} else {
		// 空中：重力を加算
		velocity_.y -= kGravity;
	}

	// --- 2. 移動入力処理 (左右移動) ---
	if (!controlsLocked_ && (Input::GetInstance()->PushKey(DIK_D) || Input::GetInstance()->PushKey(DIK_A))) {


		LR lastLR = lr_;
		KamataEngine::Vector3 acceleration = {0.0f, 0.0f, 0.0f};

		if (Input::GetInstance()->PushKey(DIK_D)) {
			if (velocity_.x < 0.0f) {
				velocity_.x *= (1.0f - kAttenuation);
			}
			acceleration.x += kMoveAcceleration;
			lr_ = LR::kRight;
		} else if (Input::GetInstance()->PushKey(DIK_A)) {
			if (velocity_.x > 0.0f) {
				velocity_.x *= (1.0f - kAttenuation);
			}
			acceleration.x -= kMoveAcceleration;
			lr_ = LR::kLeft;
		}

		if (lastLR != lr_) {
			turnTimer_ = 0.0f;
			turnFirstRotationY_ = worldTransform_.rotation_.y;
		}
		velocity_.x += acceleration.x;
	} else {
		velocity_.x *= (1.0f - kAttenuation);
	}
}

void Player::Draw(Camera* camera) {
	// モデルがセットされている場合のみ描画
	if (model_) {
		const bool isWalking = onGround_ && behavior_ == Behavior::kRoot && std::abs(velocity_.x) > 0.05f;
		if (isWalking) {
			walkAnimationTime_ += 1.0f / 60.0f;
		} else {
			walkAnimationTime_ = 0.0f;
		}

		// 物理用の変換を保ったまま、描画用だけ歩行の弾みと傾きを加える。
		worldTransformRender_.scale_ = worldTransform_.scale_;
		worldTransformRender_.rotation_ = worldTransform_.rotation_;
		worldTransformRender_.translation_ = worldTransform_.translation_;
		float legSwing = 0.0f;
		if (isWalking) {
			const float phase = walkAnimationTime_ * 2.0f * std::numbers::pi_v<float> * 1.6f;
			worldTransformRender_.translation_.y += std::abs(std::sin(phase)) * 0.035f;
			worldTransformRender_.rotation_.z += std::sin(phase) * 0.1f;
			legSwing = std::sin(phase) * 0.45f;
		}
		UpdateWorldTransform(worldTransformRender_);
		UpdateHammerTransform(worldTransformRender_);

		float alpha = 1.0f;
		if (damageInvulnerabilityFrames_ > 0 && (damageInvulnerabilityFrames_ / 3) % 2 == 0) {
			alpha = 0.25f;
		}
		objectColor_.SetColor({1.0f, 1.0f, 1.0f, alpha});
		model_->Draw(worldTransformRender_, *camera, &objectColor_);

		// 足は胴体と別モデルなので、歩行中だけ左右逆向きに振る。
		// OBJの原点は脚の付け根に合わせてある。
		auto drawLeg = [&](Model* legModel, WorldTransform& legTransform, float pivotX, float swing) {
			if (!legModel) {
				return;
			}

			legTransform.scale_ = worldTransformRender_.scale_;
			legTransform.rotation_ = worldTransformRender_.rotation_;
			legTransform.rotation_.x += swing;
			legTransform.translation_ = worldTransformRender_.translation_;

			const float yaw = worldTransformRender_.rotation_.y;
			const float pivotOffsetX = pivotX * worldTransformRender_.scale_.x;
			const float pivotOffsetY = -0.04f * worldTransformRender_.scale_.y;
			legTransform.translation_.x += std::cos(yaw) * pivotOffsetX;
			legTransform.translation_.y += pivotOffsetY;
			legTransform.translation_.z -= std::sin(yaw) * pivotOffsetX;

			UpdateWorldTransform(legTransform);
			legModel->Draw(legTransform, *camera, &objectColor_);
		};
		drawLeg(modelLeftLeg_, worldTransformLeftLeg_, -0.085f, legSwing);
		drawLeg(modelRightLeg_, worldTransformRightLeg_, 0.085f, -legSwing);
		if (modelHammer_) {
			modelHammer_->Draw(worldTransformHammer_, *camera, &objectColor_);
		}
	}
}

//指定した角の座標を計算する関数
Vector3 Player::CornerPosition(const Vector3& center, Corner corner) {
	Vector3 offsetTable[kNumCorner] = {
	    {+kWidth / 2.0f, -kHeight / 2.0f, 0}, // kRightBottom
	    {-kWidth / 2.0f, -kHeight / 2.0f, 0}, // kLeftBottom
	    {+kWidth / 2.0f, +kHeight / 2.0f, 0}, // kRightTop
	    {-kWidth / 2.0f, +kHeight / 2.0f, 0}  // kLeftTop
	};

	return center + offsetTable[static_cast<uint32_t>(corner)];
}

void Player::MapCollision(CollisionMapInfo& info) {

	MapCollisionUp(info);

	// ※ 今後、下方向・右方向・左方向の関数の中身を作ったら、ここに追記していく
	 MapCollisionDown(info);
	 MapCollisionRight(info);
     MapCollisionLeft(info);
}

void Player::MapCollisionUp(CollisionMapInfo& info) {
	// 移動後の4つの角の座標を計算して格納する配列
	std::array<Vector3, kNumCorner> positionsNew;

	// for文でまとめて計算する
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(worldTransform_.translation_ + info.velocity, static_cast<Corner>(i));
	}

	if (info.velocity.y <= 0) {
		return; 
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext; // 👑 変数を追加
	bool hit = false;
	MapChipField::IndexSet indexSet;

	// --- ① 左上点の判定 ---
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex + 1); // 👑 1つ上をチェック

	// 今のマスがブロック && 1つ上がブロックではない（天井の表面）
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	// --- ② 右上点の判定 ---
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex + 1); // 👑 1つ上をチェック

	// 今のマスがブロック && 1つ上がブロックではない（天井の表面）
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	if (hit) {
		// 移動後の自キャラ上端座標のインデックス
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftTop]);

		// 現在座標（移動前）が壁の外か判定
		MapChipField::IndexSet indexSetNow;
		indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_ + Vector3(0.0f, (kHeight / 2.0f), 0.0f));

		// 移動前と移動後でマスの行（yIndex）が変わっていれば（下から新しく侵入したら）
		if (indexSetNow.yIndex != indexSet.yIndex) {
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);

			// Y移動量の計算（天井の底面に合わせる）
			float yVelocity = rect.bottom - (worldTransform_.translation_.y + (kHeight / 2.0f)) - kBlank;
			info.velocity.y = std::max<float>(0.0f, yVelocity);
			info.isCeilingHit = true;
		}
	}
}

void Player::MapCollisionDown(CollisionMapInfo& info) {
	// ①【必須】移動後の4つの角の座標を計算して格納する配列
	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(worldTransform_.translation_ + info.velocity, static_cast<Corner>(i));
	}

	// 落下中（ velocity.y がマイナス ）でなければ判定をスキップする
	if (info.velocity.y >= 0.0f) {
		return;
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext; //
	bool hit = false;
	MapChipField::IndexSet indexSet;

	// 👑【指定された処理：①左下点の判定】
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex - 1);

	// 隣接セルがともにブロックであればヒット（床の表面のときだけ true にする）
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	// 👑【指定された処理を右側にも適用：②右下点の判定】
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex - 1);

	// 隣接セルがともにブロックであればヒット（右側も同様にチェック）
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	// ブロックにヒット？
	if (hit) {
		// めり込みを排除する方向に移動量を設定する（移動後の自キャラ下端座標のインデックスを取得）
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom]);

		// 現在座標が壁の外か判定するためのインデックス
		MapChipField::IndexSet indexSetNow;
		// 移動前の自キャラ下端座標（現在の世界座標から身長の半分を下へ引いた位置）
		indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_ + Vector3(0.0f, -(kHeight / 2.0f), 0.0f));

		// 移動前と移動後でマスの行（yIndex）が変わっていれば（＝上から新しくブロックのマスに侵入した瞬間なら）
		if (indexSetNow.yIndex != indexSet.yIndex) {
			// めり込み先ブロックの範囲矩形
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);

			// Y移動量の計算（ブロックの天面にきれいに乗せるための調整値）
			float yVelocity = rect.top - (worldTransform_.translation_.y - (kHeight / 2.0f)) + kBlank;

			// 移動量をブロックにめりこまない範囲（0か、調整された速度）に制限する
			info.velocity.y = std::min<float>(0.0f, yVelocity);

			// 地面に当たったことを記録する
			info.isLanding = true;
		}
	}
}


// 判定結果を反映して実際に移動させる
void Player::MoveWithCollision(const CollisionMapInfo& info) {
	// 判定によって変化した（制限された）移動量分だけ、実際の座標に加算して移動させる
	worldTransform_.translation_.x += info.velocity.x;
	worldTransform_.translation_.y += info.velocity.y;
	worldTransform_.translation_.z += info.velocity.z;
	// プレイヤー本体が画面右端から外へ出ないよう、カメラの投影範囲で制限する。
	if (camera_) {
		const float viewDepth = worldTransform_.translation_.z - camera_->translation_.z;
		const float projectionScaleX = camera_->matProjection.m[0][0];
		if (viewDepth > 0.0f && projectionScaleX > 0.0f) {
			constexpr float kRightEdgeScreenX = 0.95f;
			const float rightEdgeWorldX = camera_->translation_.x +
				kRightEdgeScreenX * viewDepth / projectionScaleX;
			const float maxPlayerCenterX = rightEdgeWorldX - kWidth * 0.5f;
			if (worldTransform_.translation_.x > maxPlayerCenterX) {
				worldTransform_.translation_.x = maxPlayerCenterX;
				if (velocity_.x > 0.0f) {
					velocity_.x = 0.0f;
				}
			}
		}
	}
}

// ④天井に接触している場合の処理（応答処理）

void Player::OnCeilingCollision(const CollisionMapInfo& info) {
	// 天井に当たった？
	if (info.isCeilingHit) {
		// デバッグログを仕込んで判定がうまくいっていることを確認する
		KamataEngine::DebugText::GetInstance()->ConsolePrintf("hit ceiling\n");

		// Y方向の速度を0にすることで、上昇が止まって落下し始める
		velocity_.y = 0.0f;
	}
}

//⑥接地状態の切り替え処理
void Player::OnGroundToggle(const CollisionMapInfo& info) {
	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(worldTransform_.translation_ + info.velocity, static_cast<Corner>(i));
	}

	// 自キャラが接地状態？
	if (onGround_) {

	
		MapChipType mapChipType;
		// 真下の当たり判定を行う
		bool hit = false;

		// 左下点の判定（微小な数値分、下へずらして判定）
		MapChipField::IndexSet indexSetLeftBottom = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom] + Vector3(0, -kSubsideDistance, 0));
		mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSetLeftBottom.xIndex, indexSetLeftBottom.yIndex);
		if (mapChipType == MapChipType::kBlock) {
			hit = true;
		}

		// 右下点の判定（微小な数値分、下へずらして判定）
		MapChipField::IndexSet indexSetRightBottom = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightBottom] + Vector3(0, -kSubsideDistance, 0));
		mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSetRightBottom.xIndex, indexSetRightBottom.yIndex);
		if (mapChipType == MapChipType::kBlock) {
			hit = true;
		}

		if (hit) {
			// ジャンプボタン（スペースキー）が押されたかチェック
			if (!controlsLocked_ && Input::GetInstance()->TriggerKey(DIK_SPACE)) {
				// 接地フラグを false にする
				onGround_ = false;
			}
			// それ以外（ジャンプしていない）なら
			else {
				// Y方向の移動量（速度）をゼロにする
				velocity_.y = 0.0f;
			}
		}
		// もし足元のブロックが完全になくなったら（＝崖から足がはみ出たら）
		else {
			// 即座に空中状態に落とす
			onGround_ = false;
		}

	} else {
		// 【空中状態の処理：今のコードをそのままキープ】
		if (info.isLanding) {
			onGround_ = true;
			velocity_.y = 0.0f;
		} else {
			velocity_.y -= kAcceleration;
		}
	}
}

void Player::MapCollisionRight(CollisionMapInfo& info) {
	// 移動後の4つの角の座標を計算して格納する配列
	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(worldTransform_.translation_ + info.velocity, static_cast<Corner>(i));
	}

	// 右への移動（ info.velocity.x がプラス ）でなければ判定をスキップする
	if (info.velocity.x <= 0.0f) {
		return;
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext; // 👑 変数を追加
	bool hit = false;
	MapChipField::IndexSet indexSet;

	// --- ① 右上点の判定 ---
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex - 1, indexSet.yIndex); // 1つ右をチェック

	// 今のマスがブロック && 1つ右がブロックではない（右壁の表面）
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	// --- ② 右下点の判定 ---
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex - 1, indexSet.yIndex); // 1つ右をチェック

	// 今のマスがブロック && 1つ右がブロックではない（右壁の表面）
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	if (hit) {
		// 移動後の自キャラ右端座標のインデックス
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightTop]);

		// 現在座標（移動前）が壁の外か判定
		MapChipField::IndexSet indexSetNow;
		indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_ + Vector3((kWidth / 2.0f), 0.0f, 0.0f));

		// 移動前と移動後でマスの列（xIndex）が変わっていれば（左から右へ新しく侵入したら）
		if (indexSetNow.xIndex != indexSet.xIndex) {
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);

			// X移動量の計算（壁の左端に合わせる）
			float xVelocity = rect.left - (worldTransform_.translation_.x + (kWidth / 2.0f)) - kBlank;
			info.velocity.x = std::max<float>(0.0f, xVelocity);
			info.isWallHit = true;
		}
	}
}

void Player::MapCollisionLeft(CollisionMapInfo& info) {
	// 移動後の4つの角の座標を計算して格納する配列
	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(worldTransform_.translation_ + info.velocity, static_cast<Corner>(i));
	}

	// 👑【左方向用に変更：左移動あり？】
	// 左への移動（ info.velocity.x がマイナス ）でなければ判定をスキップする
	if (info.velocity.x >= 0.0f) {
		return;
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext; // 👑 変数を追加
	bool hit = false;
	MapChipField::IndexSet indexSet;

	// --- ① 左上点の判定 ---
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex + 1, indexSet.yIndex); // 👑 1つ左をチェック

	// 今のマスがブロック && 1つ左がブロックではない（左壁の表面）
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	// --- ② 左下点の判定 ---
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex + 1, indexSet.yIndex); // 👑 1つ左をチェック

	// 今のマスがブロック && 1つ左がブロックではない（左壁の表面）
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	// ブロックにヒット？
	if (hit) {
		// 移動後の自キャラ左端座標のインデックス
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftTop]);

		// 現在座標（移動前）が壁の外か判定
		MapChipField::IndexSet indexSetNow;
		indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_ + Vector3(-(kWidth / 2.0f), 0.0f, 0.0f));

		// 移動前と移動後でマスの列（xIndex）が変わっていれば（右から左へ新しく侵入したら）
		if (indexSetNow.xIndex != indexSet.xIndex) {
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);

			// X移動量の計算（壁の右端に合わせる）
			float xVelocity = rect.right - (worldTransform_.translation_.x - (kWidth / 2.0f)) + kBlank;
			info.velocity.x = std::min<float>(0.0f, xVelocity);
			info.isWallHit = true;
		}
	}
}

// ⑤壁に接触している場合の処理
void Player::OnWallCollision(const CollisionMapInfo& info) {
	// 壁接触による減速
	if (info.isWallHit) {
		// スクロールに押されている最中に壁に当たったら、挟まれたと判定する。
		if (scrollPushActive_) {
			isDead_ = true;
		}
		velocity_.x *= (1.0f - kAttenuationWall);
	}
	scrollPushActive_ = false;
}

//ワールド座標を取得する関数の実装
Vector3 Player::GetWorldPosition() const {
	// ワールド座標を入れる変数
	Vector3 worldPos;

	// ワールド行列の平行移動成分（m[3][0], m[3][1], m[3][2]）を取得
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];

	return worldPos;
}

AABB Player::GetAABB() {
	// プレイヤーの現在の中心座標を取得
	Vector3 worldPos = GetWorldPosition();

	AABB aabb;

	// プレイヤー本体の地形衝突サイズ（kWidth / kHeight）を敵との当たり判定にも使います。
	// サイズを変える場合は Player.h の kWidth / kHeight を調整してください。
	float width = kWidth;  // プレイヤー本体の横幅
	float height = kHeight; // プレイヤー本体の縦幅

	// スライドの計算式通りに、最小点(min)と最大点(max)を設定
	aabb.min = {worldPos.x - width / 2.0f, worldPos.y - height / 2.0f, worldPos.z - width / 2.0f};
	aabb.max = {worldPos.x + width / 2.0f, worldPos.y + height / 2.0f, worldPos.z + width / 2.0f};

	return aabb;
}

bool Player::IsSpinAttackHit(const AABB& target) const {
	return hammerHitActive_ && IsCollision(hammerAttackAabb_, target);
}

void Player::OnCollision(const Enemy* enemy) {
	// 引数を使っていないことによる警告を抑制する処理
	(void)enemy;

	if (IsDodging()) {
		return;
	}

	ApplyDamage();
}

void Player::ApplyDamage() {
	if (isDead_ || damageInvulnerabilityFrames_ > 0) {
		return;
	}

	--hp_;
	PlayPlayerDamageSound();
	damageInvulnerabilityFrames_ = kDamageInvulnerabilityDuration;
	if (hp_ <= 0) {
		isDead_ = true;
	}
}

void Player::ApplyScrollPush() {
	scrollPushActive_ = false;
	if (!camera_ || autoRunSpeed_ <= 0.0f) {
		return;
	}

	// プレイヤーの画面上のX座標をNDC（-1=左端、+1=右端）で求める。
	const float viewDepth = worldTransform_.translation_.z - camera_->translation_.z;
	const float projectionScaleX = camera_->matProjection.m[0][0];
	if (viewDepth <= 0.0f || projectionScaleX <= 0.0f) {
		return;
	}
	const float screenX =
		(worldTransform_.translation_.x - camera_->translation_.x) * projectionScaleX / viewDepth;
	if (screenX > kScrollPushScreenX) {
		return;
	}

	// 画面端より外へ出た分だけ速度を上げて追いつかせる。
	const float outsideDistance = (kScrollPushScreenX - screenX) * viewDepth / projectionScaleX;
	const float catchUpSpeed = std::min<float>(
		kLimitVelocity,
		autoRunSpeed_ + outsideDistance * kScrollCatchUpRate);
	scrollPushActive_ = velocity_.x < catchUpSpeed;
	velocity_.x = std::max<float>(velocity_.x, catchUpSpeed);
}

// 👑通常行動の更新処理
void Player::BehaviorRootUpdate() {
	if (behavior_ == Behavior::kKnockback) {
		// 移動入力を無視して、ここから下のマップ衝突判定や移動処理（CollisionMapInfoなど）だけを実行させる
		goto SKIP_INPUT;
	}

	if (!controlsLocked_ && Input::GetInstance()->TriggerKey(DIK_LSHIFT) && dodgeCooldownTimer_ <= 0.0f) {
		PlayDodgeSound();
		behaviorRequest_ = Behavior::kDodge;
		dodgeCooldownTimer_ = kDodgeCooldownDuration;
	} else if (!controlsLocked_ && Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		// 地面にいる、または、空中だけどまだ一度も空中攻撃をしていない場合のみ発動
		if (onGround_ || !hasAirAttacked_) {

			// もし空中での発動だったなら、空中攻撃フラグを立てて制限をかける
			if (!onGround_) {
				hasAirAttacked_ = true;
			}

			// 攻撃行動の初期化とリクエスト
			behaviorRequest_ = Behavior::kSpinAttack;
		}
	}


	Move();

	SKIP_INPUT:
	ApplyScrollPush();

	// 衝突情報を初期化
	CollisionMapInfo collisionMapInfo;
	// 移動量に速度の値をコピー
	collisionMapInfo.velocity = velocity_;

	// ② マップ衝突チェック（大元を呼び出し、内部でMapCollisionUpが走るようにします）
	MapCollision(collisionMapInfo);

	// ③ 判定によって安全になった移動量を、実際の座標（translation_）に足し算して動かす
	MoveWithCollision(collisionMapInfo);

	OnGroundToggle(collisionMapInfo);

	// ④ 天井に接触していた場合の応答（本当に当たっていたら、上昇速度 velocity_.y を 0 にする）
	OnCeilingCollision(collisionMapInfo);
	// ⑤ 壁に接触している場合の応答を呼び出す
	OnWallCollision(collisionMapInfo);

	// --- 3. 滑らかな振り向き (Lerp) ---
	if (turnTimer_ < 1.0f) {
		turnTimer_ += kTurnSpeed;
		if (turnTimer_ > 1.0f)
			turnTimer_ = 1.0f;
		float targetAngle = (lr_ == LR::kRight) ? (std::numbers::pi_v<float> / 2.0f) : (-std::numbers::pi_v<float> / 2.0f);
		float diffAngle = targetAngle - turnFirstRotationY_;
		if (diffAngle >= std::numbers::pi_v<float>) {
			diffAngle -= 2.0f * std::numbers::pi_v<float>;
		} else if (diffAngle < -std::numbers::pi_v<float>) {
			diffAngle += 2.0f * std::numbers::pi_v<float>;
		}
		worldTransform_.rotation_.y = turnFirstRotationY_ + diffAngle * turnTimer_;
	}

	// --- 4. 速度制限 ---
	const float horizontalSpeedLimit = bossHitKnockback_ ? kBossHitKnockbackSpeed : kLimitVelocity;
	velocity_.x = std::clamp(velocity_.x, -horizontalSpeedLimit, horizontalSpeedLimit);

	// --- 7. 行列更新 ---
	UpdateWorldTransform(worldTransform_);
}

// 回避：溜めを挟まず、旧突進アニメーションで即座に前進する。
// 回避中は敵との判定をGameScene側で無効にするが、地形との衝突は維持する。
void Player::BehaviorDodgeUpdate() {
	++attackParameter_;
	velocity_ = {0.0f, 0.0f, 0.0f};
	if (!onGround_) {
		velocity_.y -= kGravity;
	}
	velocity_.x = (lr_ == LR::kRight) ? 0.8f : -0.8f;

	// 縮みは回避開始直後だけにし、4フレーム後から通常サイズを保つ。
	const float squashT = (std::min)(1.0f, static_cast<float>(attackParameter_) / 4.0f);
	worldTransform_.scale_.z = Easing::EaseOut(1.5f, 5.0f, squashT);
	worldTransform_.scale_.y = Easing::EaseOut(8.0f, 5.0f, squashT);

	ApplyScrollPush();
	CollisionMapInfo collisionMapInfo;
	collisionMapInfo.velocity = velocity_;
	MapCollision(collisionMapInfo);
	MoveWithCollision(collisionMapInfo);
	OnGroundToggle(collisionMapInfo);
	OnCeilingCollision(collisionMapInfo);
	OnWallCollision(collisionMapInfo);
	UpdateWorldTransform(worldTransform_);

	if (attackParameter_ >= 20) {
		behaviorRequest_ = Behavior::kRoot;
	}
}

// ハンマー攻撃：振り上げてから振り下ろし、打撃面の範囲で敵を攻撃する。
void Player::BehaviorSpinAttackUpdate() {
	++attackParameter_;
	velocity_.x = 0.0f;
	if (!onGround_) {
		velocity_.y -= kGravity;
	} else {
		velocity_.y = 0.0f;
	}

	const auto smoothStep = [](float t) {
		t = std::clamp(t, 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	};
	// 予備動作で後ろへ引き、攻撃方向へ振り下ろす。左右の向きに応じて符号を反転する。
	const float facingSign = (lr_ == LR::kRight) ? 1.0f : -1.0f;
	if (attackParameter_ <= 10) {
		const float t = static_cast<float>(attackParameter_) / 10.0f;
		hammerAngle_ = facingSign * 1.05f * smoothStep(t);
	} else if (attackParameter_ <= 18) {
		const float t = static_cast<float>(attackParameter_ - 10) / 8.0f;
		hammerAngle_ = facingSign * (1.05f - 2.40f * smoothStep(t));
	} else {
		const float t = static_cast<float>(attackParameter_ - 18) / static_cast<float>(kHammerAttackDuration - 18);
		hammerAngle_ = facingSign * -1.35f * (1.0f - smoothStep(t));
	}

	// 振り下ろしが当たる短い間だけ、ハンマー頭部の当たり判定を有効にする。
	hammerHitActive_ = attackParameter_ >= 14 && attackParameter_ <= 24;

	if (attackParameter_ >= kHammerAttackDuration) {
		hammerAngle_ = 0.0f;
		hammerHitActive_ = false;
		behaviorRequest_ = Behavior::kRoot;
	}

	ApplyScrollPush();
	CollisionMapInfo collisionMapInfo;
	collisionMapInfo.velocity = velocity_;
	MapCollision(collisionMapInfo);
	MoveWithCollision(collisionMapInfo);
	OnGroundToggle(collisionMapInfo);
	OnCeilingCollision(collisionMapInfo);
	OnWallCollision(collisionMapInfo);
	UpdateWorldTransform(worldTransform_);
	UpdateHammerTransform(worldTransform_);
}

// 通常行動の初期化処理（中身は空っぽでOK）
void Player::BehaviorRootInitialize() {
	worldTransform_.scale_ = {5.0f, 5.0f, 5.0f};
	worldTransform_.rotation_.z = spinStartRotationZ_;
	hammerAngle_ = 0.0f;
	hammerHitActive_ = false;
}

void Player::BehaviorDodgeInitialize() {
	attackParameter_ = 0;
	velocity_ = {0.0f, 0.0f, 0.0f};
	worldTransform_.scale_ = {5.0f, 8.0f, 1.5f};
}

void Player::BehaviorSpinAttackInitialize() {
	attackParameter_ = 0;
	velocity_.x = 0.0f;
	spinStartRotationZ_ = worldTransform_.rotation_.z;
	hammerAngle_ = 0.0f;
	hammerHitActive_ = false;
	UpdateHammerTransform(worldTransform_);
}

void Player::UpdateHammerTransform(const WorldTransform& rootTransform) {
	// 元のplayer.objから切り出したハンマーを、元モデルと同じ縮尺で表示する。
	worldTransformHammer_.scale_ = rootTransform.scale_;
	worldTransformHammer_.rotation_ = rootTransform.rotation_;
	// 元モデルの横向きの面ではなく、ハンマー頭部の先端で叩けるよう90度回す。
	worldTransformHammer_.rotation_.y += std::numbers::pi_v<float> / 2.0f;
	// 画面に垂直な軸で回し、ハンマーを上から下へ振り下ろす。
	worldTransformHammer_.rotation_.z += hammerAngle_;
	worldTransformHammer_.translation_ = rootTransform.translation_;

	// 抽出OBJの原点を柄の握り位置に置いたため、その位置をプレイヤー原点から変換する。
	constexpr float gripX = -0.33f;
	constexpr float gripY = -0.13f;
	constexpr float gripZ = 0.14f;
	const float yaw = rootTransform.rotation_.y;
	worldTransformHammer_.translation_.x += std::cos(yaw) * gripX * rootTransform.scale_.x + std::sin(yaw) * gripZ * rootTransform.scale_.z;
	worldTransformHammer_.translation_.y += gripY * rootTransform.scale_.y;
	worldTransformHammer_.translation_.z += -std::sin(yaw) * gripX * rootTransform.scale_.x + std::cos(yaw) * gripZ * rootTransform.scale_.z;
	UpdateWorldTransform(worldTransformHammer_);

	// 90度回転でハンマー頭部が敵のいる奥行き面から外れるため、頭部中心をプレイヤーのZ面へ戻す。
	const Vector3 hammerHeadCenter = {0.0f, 0.75f, -0.64f};
	const auto& initialMatrix = worldTransformHammer_.matWorld_.m;
	const float headCenterWorldZ =
		hammerHeadCenter.x * initialMatrix[0][2] +
		hammerHeadCenter.y * initialMatrix[1][2] +
		hammerHeadCenter.z * initialMatrix[2][2] +
		initialMatrix[3][2];
	worldTransformHammer_.translation_.z += rootTransform.translation_.z - headCenterWorldZ;
	UpdateWorldTransform(worldTransformHammer_);

	// 柄は攻撃判定に含めず、元モデルのハンマー頭部だけを判定する。
	const Vector3 localCorners[8] = {
		{-0.245f, 0.57f, -0.82f}, {0.245f, 0.57f, -0.82f},
		{-0.245f, 0.93f, -0.82f}, {0.245f, 0.93f, -0.82f},
		{-0.245f, 0.57f, -0.46f}, {0.245f, 0.57f, -0.46f},
		{-0.245f, 0.93f, -0.46f}, {0.245f, 0.93f, -0.46f}};
	const auto& matrix = worldTransformHammer_.matWorld_.m;
	for (uint32_t i = 0; i < 8; ++i) {
		const Vector3& corner = localCorners[i];
		const Vector3 worldCorner = {
			corner.x * matrix[0][0] + corner.y * matrix[1][0] + corner.z * matrix[2][0] + matrix[3][0],
			corner.x * matrix[0][1] + corner.y * matrix[1][1] + corner.z * matrix[2][1] + matrix[3][1],
			corner.x * matrix[0][2] + corner.y * matrix[1][2] + corner.z * matrix[2][2] + matrix[3][2]};
		if (i == 0) {
			hammerAttackAabb_.min = worldCorner;
			hammerAttackAabb_.max = worldCorner;
		} else {
			hammerAttackAabb_.min.x = (std::min)(hammerAttackAabb_.min.x, worldCorner.x);
			hammerAttackAabb_.min.y = (std::min)(hammerAttackAabb_.min.y, worldCorner.y);
			hammerAttackAabb_.min.z = (std::min)(hammerAttackAabb_.min.z, worldCorner.z);
			hammerAttackAabb_.max.x = (std::max)(hammerAttackAabb_.max.x, worldCorner.x);
			hammerAttackAabb_.max.y = (std::max)(hammerAttackAabb_.max.y, worldCorner.y);
			hammerAttackAabb_.max.z = (std::max)(hammerAttackAabb_.max.z, worldCorner.z);
		}
	}

	// 敵との接触より先にハンマーが届くよう、頭部判定を向いている方向へ延長する。
	constexpr float kForwardAttackReach = 3.0f;
	if (lr_ == LR::kRight) {
		hammerAttackAabb_.max.x += kForwardAttackReach;
	} else {
		hammerAttackAabb_.min.x -= kForwardAttackReach;
	}
}


//ノックアックの行動初期化
//  👑【修正】ノックバックの行動初期化
void Player::BehaviorKnockbackInitialize() {
	knockbackPhase_ = KnockbackPhase::kFling;
	knockbackTimer_ = 0.0f; // タイマーリセット
	hammerAngle_ = 0.0f;
	hammerHitActive_ = false;

	// 🛡️【見た目の修正】攻撃中に変わってしまったプレイヤーのスケールや回転を、即座に通常状態の正しい形にリセットする
	worldTransform_.scale_ = {5.0f, 5.0f, 5.0f};
	if (lr_ == LR::kRight) {
		worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f;
	} else {
		worldTransform_.rotation_.y = -std::numbers::pi_v<float> / 2.0f;
	}
	worldTransform_.rotation_.z = spinStartRotationZ_;

	// ボスに攻撃を当てた場合は、向きに関係なく画面左端まで飛ばす。
	if (bossHitKnockback_) {
		velocity_.x = -kBossHitKnockbackSpeed;
		velocity_.y = kBossHitKnockbackUpSpeed;
		onGround_ = false;
		return;
	}

	// 👑【重要】向いている方向の「真後ろ」へ強い初速を与える！
	float knockbackSpeed = 1.2f; // 吹き飛ぶ強さ（好みに合わせて調整してください。4.0fは速すぎたため少し抑えています）

	if (lr_ == LR::kRight) {
		velocity_.x = -knockbackSpeed; // 右を向いていたら「左」へ飛ばされる
	} else {
		velocity_.x = knockbackSpeed; // 左を向いていたら「右」へ飛ばされる
	}

	// ノックバック開始時は、上方向にも少し跳ね上がらせると自然になります
	velocity_.y = 0.2f;
}

// 👑【修正】ノックバック中の毎フレーム更新
void Player::BehaviorKnockbackUpdate() {
	if (bossHitKnockback_) {
		if (!camera_) {
			return;
		}

		const float viewDepth = worldTransform_.translation_.z - camera_->translation_.z;
		const float projectionScaleX = camera_->matProjection.m[0][0];
		if (viewDepth > 0.0f && projectionScaleX > 0.0f) {
			const float leftEdgeWorldX =
				camera_->translation_.x - 0.95f * viewDepth / projectionScaleX;
			if (worldTransform_.translation_.x <= leftEdgeWorldX) {
				worldTransform_.translation_.x = leftEdgeWorldX;
				velocity_.x = 0.0f;
			} else {
				velocity_.x = -kBossHitKnockbackSpeed;
			}
		}

		if (!onGround_) {
			velocity_.y -= kGravity;
		} else {
			velocity_.y = 0.0f;
		}
		return;
	}

	knockbackTimer_ += 1.0f; // 時間を進める

	switch (knockbackPhase_) {
	case KnockbackPhase::kFling: // ①「強い初速で弾き飛ばされる」フェーズ
		// 重力を適用して、放物線を描いて落ちるようにする
		if (!onGround_) {
			velocity_.y -= kGravity;
		}

		// 10フレーム（約0.16秒）間経過、または地面に着地したら体勢立て直しへ
		if (knockbackTimer_ >= 10.0f || (knockbackTimer_ > 2.0f && onGround_)) {
			knockbackPhase_ = KnockbackPhase::kRecover;
			knockbackTimer_ = 0.0f; // タイマーをリセットして次のフェーズへ
		}
		break;

	case KnockbackPhase::kRecover: // ②「移動が停止し、体勢を立て直す」フェーズ
		// 摩擦のように速度を減衰させる
		velocity_.x *= 0.8f;
		if (!onGround_) {
			velocity_.y -= kGravity;
		} else {
			velocity_.y = 0.0f;
		}

		// 10フレームかけて体勢を戻したら、通常状態へ復帰
		if (knockbackTimer_ >= 10.0f) {
			// 通常状態に戻るリクエストを送る
			behaviorRequest_ = Behavior::kRoot;
		}
		break;
	}
}

void Player::StartBossHitKnockback() {
	bossHitKnockback_ = true;
	behaviorRequest_ = Behavior::kKnockback;
}

void Player::CompleteBossHitKnockback() {
	if (!bossHitKnockback_) {
		return;
	}

	bossHitKnockback_ = false;
	behaviorRequest_ = Behavior::kRoot;
}

void Player::OnCollision(ShieldEnemy* shieldEnemy) {
	(void)shieldEnemy;

	if (IsDodging()) {
		return;
	}

	// ノックバック中は接触しても追加で死亡しないようにする。
	// ガードされて弾き飛ばされている最中（あるいは体勢を立て直している最中）は、
	// まだ敵の体に重なり続けているため、ここでも死亡フラグを立てずにスルーします。
	if (behavior_ == Behavior::kKnockback) {
		return;
	}

	// 🛡️ 3. それ以外の通常時（kRootなど）にぶつかった場合
	// 攻撃もしていないのに敵の体に触れたということは「激突してアウト」なので、
	// 通常の敵（Enemy）と同じようにデスフラグを立ててゲームオーバーにします！
	ApplyDamage();
}

