#include "ShieldEnemy.h"
#include <numbers>
#include <algorithm> 
#include "Math.h"
#include "Player.h"
#include "GameScene.h"

void ShieldEnemy::Initialize(Model* model, Camera* camera, const Vector3& position) {
	// モデルとカメラを保持
	model_ = model;
	camera_ = camera;

	// ワールド変換の初期化
	worldTransform_.Initialize();
	objectColor_.Initialize();
	objectColor_.SetColor({1.0f, 1.0f, 1.0f, 1.0f});

	// スケールの設定 (プレイヤーに合わせて3.0fにしていますが、敵のサイズに合わせて変更してください)
	worldTransform_.scale_ = {4.0f, 4.0f, 4.0f};

	// 向き（回転）の設定 (最初は左を向かせる場合は -std::numbers::pi_v<float> / 2.0f など)
	worldTransform_.rotation_.y = -std::numbers::pi_v<float> / 2.0f;

	// 引数で受け取った初期位置を設定
	worldTransform_.translation_ = position;

	//アニメーションのタイマーを初期化
	walkTimer_ = 0.0f;

	isDead_ = false;

	// 初期状態は当たり判定を有効（false）にする
	isCollisionDisabled_ = false;

	// 初期状態は歩行
	behavior_ = Behavior::kRoot;
	BehaviorRootInitialize();

	// 初期化時に行列を計算・転送
	UpdateWorldTransform(worldTransform_);

}

//  毎フレームの更新処理（状態によって呼び分ける）
void ShieldEnemy::Update() {
	// 状態遷移リクエストの処理
	if (behaviorRequest_ != Behavior::kUnknown) {
		// 振るまいを更新
		behavior_ = behaviorRequest_;

		// リクエストをリセット
		behaviorRequest_ = Behavior::kUnknown;

		// 各状態の初期化関数を呼び出す
		switch (behavior_) {
		case Behavior::kRoot:
			BehaviorRootInitialize();
			break;
		case Behavior::kDeath:
			BehaviorDeathInitialize(); // ここでデス演出が始まる
			break;
		case Behavior::kGuard: // 👈【追加】
			BehaviorGuardInitialize();
			break;
		}
	}

	// 状態ごとの更新
	switch (behavior_) {
	case Behavior::kRoot:
		BehaviorRootUpdate();
		break;
	case Behavior::kDeath:
		BehaviorDeathUpdate();
		break;
	case Behavior::kGuard: // 👈【追加】
		BehaviorGuardUpdate();
		break;
	}
}
#pragma region 歩行ビヘイビア (kRoot)
void ShieldEnemy::BehaviorRootInitialize() {
	// 歩行開始時の初期化があればここに書く（今回は特になし）
}

void ShieldEnemy::BehaviorRootUpdate() {
	// 元々の移動・アニメーション用タイマーを進める処理
	walkTimer_ += 1.0f / 60.0f;

	// アニメーション用のサイン波を計算（0.5秒周期）
	float param = std::sin(std::numbers::pi_v<float> * 2.0f * walkTimer_ / 0.5f);

	// 回転角度（degree）を計算
	float degree = kWalkMotionAngleStart + (kWalkMotionAngleEnd - kWalkMotionAngleStart) * (param + 1.0f) / 2.0f;

	// 角度をラジアンに変換
	float toRadian = std::numbers::pi_v<float> / 180.0f;

	// ---------------------------------------------------------
	// 【シールドエネミー独自のアニメーション（左右の揺れ）】
	// ---------------------------------------------------------

	// 🛡️ X軸の回転（前後のペコペコ）を無効化（0にする）
	worldTransform_.rotation_.x = 0.0f;

	// 🛡️【ここが重要！】Z軸の回転（左右のパタパタ）に計算したラジアンを設定する！
	// これで、左にパタン、右にパタンと傾きながら歩きます。
	worldTransform_.rotation_.z = degree * toRadian;

	// ---------------------------------------------------------
	// 移動処理（最初のEnemyと完全に一緒！）
	// ---------------------------------------------------------
	worldTransform_.translation_.x += velocity_.x;
	worldTransform_.rotation_.y = (-std::numbers::pi_v<float> / 2.0f) + (degree * toRadian);
	worldTransform_.translation_.z += velocity_.z;

	// ワールド変換データの更新
	UpdateWorldTransform(worldTransform_);
}
#pragma endregion

#pragma region デス演出ビヘイビア (kDeath)
void ShieldEnemy::BehaviorDeathInitialize() {
	// デス演出が始まった瞬間にタイマーをゼロクリアする
	deathTimer_ = 0;

	// デス演出が始まったら、即座に当たり判定を無効にする！
	isCollisionDisabled_ = true;
}

void ShieldEnemy::BehaviorDeathUpdate() {
	// アニメーションのタイマーを加算する（1フレームごとに1増える）
	deathTimer_++;

	// デス演出の総フレーム数（例: 30フレーム = 0.5秒間演出する）
	const float kDeathTime = 30.0f;

	// イージングに渡すための 0.0f 〜 1.0f の割合（t）を計算
	float t = static_cast<float>(deathTimer_) / kDeathTime;

	// Y軸まわりの回転角をイージングで変化させる（高速に5回転＝10π ぐるぐる回す）
	float startRotY = -std::numbers::pi_v<float> / 2.0f;                   // 初期角度（左向き）
	float endRotY = startRotY + (std::numbers::pi_v<float> * 2.0f * 5.0f); // 5回転プラス
	worldTransform_.rotation_.y = Easing::EaseOut(startRotY, endRotY, t);

	//X軸まわりの回転角をイージングで変化させる（90度ひっくり返す）
	float startRotX = worldTransform_.rotation_.x;    // 今の歩行アニメーションの角度からスタート
	float endRotX = std::numbers::pi_v<float> / 2.0f; // 90度（真上・真下を向く形にひっくり返る）
	worldTransform_.rotation_.x = Easing::EaseIn(startRotX, endRotX, t);

	//【おまけの工夫】少し上に浮き上がらせてから消えるとさらに見栄えが良くなります！
	// 嫌な場合はこの1行は消しても大丈夫です
	worldTransform_.translation_.y += 0.1f;

	// ワールドトランスフォームの行列更新
	UpdateWorldTransform(worldTransform_);

	//  アニメーションのタイマーが一定時間に達したら
	if (deathTimer_ >= static_cast<uint32_t>(kDeathTime)) {
		// デスフラグを立てる（これで GameScene の remove_if が検知して消滅させてくれます）
		isDead_ = true;
	}
}
#pragma endregion

void ShieldEnemy::Draw(Camera* camera) {
	// 被弾してガードした時と、やられアニメーション中に点滅させる。
	if (model_) {
		const bool isBlinking = behavior_ == Behavior::kDeath || behavior_ == Behavior::kGuard;
		const uint32_t animationFrame = behavior_ == Behavior::kDeath
			? deathTimer_
			: static_cast<uint32_t>(guardTimer_ * 60.0f);
		const float alpha = isBlinking && (animationFrame / 3) % 2 == 0 ? 0.25f : 1.0f;
		objectColor_.SetColor({1.0f, 1.0f, 1.0f, alpha});
		model_->Draw(worldTransform_, *camera, &objectColor_);
	}
}

AABB ShieldEnemy::GetAABB() {
	// プレイヤーの現在の中心座標を取得
	Vector3 worldPos = GetWorldPosition(); // たぶんこれが原因


	AABB aabb;

	// 💡 キャラクターの「横幅」を 2.0f、「縦幅」を 2.0f と仮定して計算しています。
	// 当たり判定の箱を大きくしたり小さくしたりしたい場合は、この「2.0f」の数値を調整してください。
	float width = 2.0f;  // 横幅
	float height = 2.0f; // 縦幅

	//最小点(min)と最大点(max)を設定
	aabb.min = {worldPos.x - width / 2.0f, worldPos.y - height / 2.0f, worldPos.z - width / 2.0f};
	aabb.max = {worldPos.x + width / 2.0f, worldPos.y + height / 2.0f, worldPos.z + width / 2.0f};

	return aabb;
}

void ShieldEnemy::OnCollision(Player* player) {
	// すでにデス演出中なら何もしない
	if (behavior_ == Behavior::kDeath) {
		return;
	}

	// プレイヤーが攻撃中なら
	if (player->IsAttack()) {

		// 👑 1. プレイヤーの正確な向きを取得
		bool isPlayerRight = player->IsFacingRight(); // 右向きならtrue
		bool isPlayerLeft = !isPlayerRight;           // 左向きならtrue

		// 👑 2. エネミーの向きを取得（左向きならtrue）
		bool isEnemyLeft = (worldTransform_.rotation_.y < 0.0f);
		bool isEnemyRight = !isEnemyLeft;

		// 👑 3. 学校のスライド通りの完全なガード条件式（2重のifを綺麗にまとめました）
		if ((isPlayerRight && isEnemyLeft) || (isPlayerLeft && isEnemyRight)) {

			// 🛡️ 条件が成り立てばガード成功！
			Vector3 position = GetWorldPosition();
			if (gameScene_) {
				gameScene_->CreateGuardEffect(position); // ガードエフェクトを生成！
			}

			player->KnockbackRequest(); 

			behaviorRequest_ = Behavior::kGuard;

			// 早期returnで敵のデスを回避
			return;
		}

		// ⚔️ 条件が成り立たなければ（背後からの攻撃なら）デス演出へ！
		behaviorRequest_ = Behavior::kDeath;

		if (gameScene_) {
			gameScene_->CreateHitEffectAtContact(player->GetWorldPosition(), GetAABB());
		}
	}
}

Vector3 ShieldEnemy::GetWorldPosition() {
	// ワールド座標を入れる変数
	Vector3 worldPos;

	// ワールド行列の平行移動成分（m[3][0], m[3][1], m[3][2]）を取得
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];

	return worldPos;
}

void ShieldEnemy::SetGameScene(GameScene* gameScene) { gameScene_ = gameScene; }

void ShieldEnemy::BehaviorGuardInitialize() {
	guardTimer_ = 0.0f; // アニメーション用タイマーをリセット
}

// 🛡️ ガード状態の更新（のけぞりアニメーション）
void ShieldEnemy::BehaviorGuardUpdate() {
	// ガードリアクションの時間（例：0.5秒間 ＝ 30フレームで終了して歩行に戻る）
	const float kGuardDuration = 0.5f;
	guardTimer_ += 1.0f / 60.0f;

	if (guardTimer_ >= kGuardDuration) {
		guardTimer_ = kGuardDuration;
		// 👑 演出時間が終わったら、何事もなかったかのように歩行状態（kRoot）に戻る
		behaviorRequest_ = Behavior::kRoot;
	}

	// -------------------------------------------------------------
	// 👑【スライド再現】サイン関数を使って「のけぞり」を計算
	// -------------------------------------------------------------
	// 0.0 から 1.0 まで進む進捗率
	float progress = guardTimer_ / kGuardDuration;

	// sin波 (0度 から 180度(PI) まで変化。0 から 始まり、途中で 1.0 になって 0 に戻る)
	float sinWave = std::sin(progress * std::numbers::pi_v<float>);

	// のけぞりの強さ（ラジアン）。ここを調整するとお辞儀やのけぞりの激しさが変わります
	float startDipping = -0.1f;  // ガード直後にちょっとだけ「下を向く（前傾）」
	float maxReclining = 0.5f;   // そのあとカキーンと「上を向く（のけぞり）」

	// 3Dモデルの回転をリセットし、ベースの向き（左向き）を設定
	worldTransform_.rotation_.x = 0.0f;
	worldTransform_.rotation_.y = -std::numbers::pi_v<float> / 2.0f;
	worldTransform_.rotation_.z = 0.0f;

	// 左を向いている（Y = -90度）敵に対して「カメラから見てのけぞる」には、
	// 手前・奥の軸である Z軸 を回す、あるいは回転順序を考慮して適用します。
	// サイン波を使って「最初少し前傾し、グッと天井を仰ぎ、最後に綺麗に元の姿勢(0)に戻る」動きを作ります。
	worldTransform_.rotation_.z = (startDipping * (1.0f - sinWave)) + (maxReclining * sinWave);

	//敵を少し後ろにのけぞらせてる
	 worldTransform_.translation_.x -= velocity_.x * 0.5f; 

	// 行列更新をして画面に反映
	UpdateWorldTransform(worldTransform_);
}
