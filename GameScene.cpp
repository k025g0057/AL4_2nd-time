#include "GameScene.h"
#include "Math.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>
#include "MapChipField.h"
#include "Collision.h"
#include"imgui.h"
#include "StageManager.h"
#include "UiSound.h"
#include "Bgm.h"
#include "HitSound.h"

using namespace KamataEngine;

// X軸・Y軸の両方の回転に対応したアフィン行列計算
Matrix4x4 MakeAffineMatrix(const KamataEngine::Vector3& scale, const KamataEngine::Vector3& rotate, const KamataEngine::Vector3& translate) {
	// X軸の回転
	float sinX = std::sin(rotate.x);
	float cosX = std::cos(rotate.x);

	// Y軸の回転
	float sinY = std::sin(rotate.y);
	float cosY = std::cos(rotate.y);

	// Z軸の回転
	float sinZ = std::sin(rotate.z);
	float cosZ = std::cos(rotate.z);

	Matrix4x4 result;

	// 👑【修正】X・Y・Z軸すべての回転（順序: X->Y->Z）とスケールを正しく合成した行列式
	result.m[0][0] = scale.x * (cosY * cosZ);
	result.m[0][1] = scale.x * (cosY * sinZ);
	result.m[0][2] = scale.x * (-sinY);
	result.m[0][3] = 0.0f;

	result.m[1][0] = scale.y * (sinX * sinY * cosZ - cosX * sinZ);
	result.m[1][1] = scale.y * (sinX * sinY * sinZ + cosX * cosZ);
	result.m[1][2] = scale.y * (sinX * cosY);
	result.m[1][3] = 0.0f;

	result.m[2][0] = scale.z * (cosX * sinY * cosZ + sinX * sinZ);
	result.m[2][1] = scale.z * (cosX * sinY * sinZ - sinX * cosZ);
	result.m[2][2] = scale.z * (cosX * cosY);
	result.m[2][3] = 0.0f;

	// 平行移動成分
	result.m[3][0] = translate.x;
	result.m[3][1] = translate.y;
	result.m[3][2] = translate.z;
	result.m[3][3] = 1.0f;

	return result;
}
void GameScene::Initialize(StageManager* stageDataManager) {
	StopTitleBgm();
	StartGameBgm();
	finished_ = false;
	reloadRequested_ = false;
	isPaused_ = false;
	pauseMenuSelection_ = 0;
	elapsedGameplayTime_ = 0.0f;
	clearMenuSelection_ = 0;
	defeatedBossTransform_.Initialize();

	stageManager_ = stageDataManager;
	bossHp_ = kBossMaxHp;
	bossPhase_ = BossPhase::kPhaseOne;
	bossTransitionToPhaseThree_ = false;
	bossPhaseTimer_ = 0.0f;
	bossSummonTimer_ = 0.0f;
	bossSummonRoarTimer_ = 0.0f;
	bossTackleState_ = BossTackleState::kWaiting;
	bossTackleTimer_ = 0.0f;
	bossTackleHitPlayer_ = false;

	// リソース読み込み
	textureHandle_ = TextureManager::Load("./Resources/block/block.png");
	textureHandleWhite_ = TextureManager::Load("./Resources/white1x1.png");
	bossTackleWarningSprite_ = Sprite::Create(0, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.18f}, {0.0f, 0.0f});
	bossAttackAlertSprite_ = Sprite::Create(TextureManager::Load("./Resources/boss_attack_alert.png"), {0.0f, 0.0f});
	pauseMenuSprites_[0] = Sprite::Create(TextureManager::Load("./Resources/pause_menu/pause_resume.png"), {0.0f, 0.0f});
	pauseMenuSprites_[1] = Sprite::Create(TextureManager::Load("./Resources/pause_menu/pause_restart.png"), {0.0f, 0.0f});
	pauseMenuSprites_[2] = Sprite::Create(TextureManager::Load("./Resources/pause_menu/pause_title.png"), {0.0f, 0.0f});
	pauseHintSprite_ = Sprite::Create(TextureManager::Load("./Resources/pause_menu/esc_hint.png"), {24.0f, 24.0f});
	if (pauseHintSprite_) { pauseHintSprite_->SetSize({96.0f, 48.0f}); }
	attackHintSprite_ = Sprite::Create(TextureManager::Load("./Resources/pause_menu/attack_hint.png"), {24.0f, 78.0f});
	if (attackHintSprite_) { attackHintSprite_->SetSize({240.0f, 48.0f}); }
	evadeHintSprite_ = Sprite::Create(TextureManager::Load("./Resources/pause_menu/evade_hint.png"), {24.0f, 132.0f});
	if (evadeHintSprite_) { evadeHintSprite_->SetSize({240.0f, 48.0f}); }
	bossNameSprite_ = Sprite::Create(TextureManager::Load("./Resources/boss_name.png"), {0.0f, 0.0f});
	bossHpBarFrameSprite_ = Sprite::Create(TextureManager::Load("./Resources/boss_hp_bar.png"), {0.0f, 0.0f});
	bossHpBarFillSprite_ = Sprite::Create(0, {0.0f, 0.0f}, {0.9f, 0.08f, 0.13f, 1.0f}, {0.0f, 0.0f});
	const uint32_t heartTexture = TextureManager::Load("./Resources/heart.png");
	for (Sprite*& heartSprite : playerHpSprites_) {
		heartSprite = Sprite::Create(heartTexture, {0.0f, 0.0f});
	}
	dodgeCooldownFrameSprite_ = Sprite::Create(0, {0.0f, 0.0f}, {0.12f, 0.36f, 0.18f, 1.0f}, {0.0f, 0.0f});
	dodgeCooldownBackSprite_ = Sprite::Create(0, {0.0f, 0.0f}, {0.08f, 0.10f, 0.09f, 1.0f}, {0.0f, 0.0f});
	dodgeCooldownFillSprite_ = Sprite::Create(0, {0.0f, 0.0f}, {0.20f, 0.85f, 0.32f, 1.0f}, {0.0f, 0.0f});
	clearMenuSprites_[0] = Sprite::Create(TextureManager::Load("./Resources/clear_menu/clear_retry_selected.png"), {0.0f, 0.0f});
	clearMenuSprites_[1] = Sprite::Create(TextureManager::Load("./Resources/clear_menu/clear_title_selected.png"), {0.0f, 0.0f});
	clearBackgroundShadeSprite_ = Sprite::Create(0, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.35f}, {0.0f, 0.0f});
	const uint32_t clearDigitsTexture = TextureManager::Load("./Resources/clear_menu/clear_time_digits.png");
	for (Sprite*& digitSprite : clearTimeDigitSprites_) {
		digitSprite = Sprite::Create(clearDigitsTexture, {0.0f, 0.0f});
	}
	modelBlock_ = Model::Create();
	modelForestTree_ = Model::CreateFromOBJ("ForestTreeColor", true);
	modelForestGrass_ = Model::CreateFromOBJ("ForestGrassColor", true);
	// 葉の色を緑に保つため、背景木の葉マテリアルだけ色を調整する。
	if (modelForestTree_) {
		for (const auto& mesh : modelForestTree_->GetMeshes()) {
			Material* material = mesh->GetMaterial();
			if (!material) {
				continue;
			}
			if (material->name_ == "LeafDark") {
				material->diffuse_ = {0.18f, 0.88f, 0.18f};
				material->ambient_ = {0.12f, 0.36f, 0.12f};
			} else if (material->name_ == "LeafMid") {
				material->diffuse_ = {0.26f, 1.0f, 0.20f};
				material->ambient_ = {0.16f, 0.46f, 0.12f};
			} else if (material->name_ == "LeafLight") {
				material->diffuse_ = {0.36f, 1.0f, 0.22f};
				material->ambient_ = {0.22f, 0.52f, 0.14f};
			} else {
				continue;
			}
			material->Update();
		}
	}
	modelSummonMagicCircle_ = Model::CreateFromOBJ("magic_circle", true);
	modelSkyDome_ = Model::CreateFromOBJ("SkyDome", true);
	skydome_ = new Skydome();
	skydome_->Initialize(modelSkyDome_);

	// カメラの初期化
	camera_.Initialize();
	camera_.farZ = 1000.0f;

	camera_.translation_ = {-10.0f, -10.0f, -100.0f};

	// デバッグカメラ
	debugCamera_ = new DebugCamera(1280, 720);

	// --- マップチップフィールドの生成とCSV読み込み ---
	// マップチップフィールドの生成
	mapChipField_ = new MapChipField();

	// 現在のステージデータを取得する
	const StageData& stageData = stageManager_->GetCurrentStageData();

	// ステージファイルパスの生成
	std::string stageFileName = "Resources/block/" + stageData.name + ".csv";

	// ステージファイルの読み込み
	mapChipField_->LoadMapChipCsv(stageFileName);
	mapChipField_->GenerateRandomGround();


	// モデル読み込み
	modelMap_ = Model::CreateFromOBJ("block", true);
	modelBossBullet_ = Model::CreateFromOBJ("boss_bullet", true);
	modelGround_ = Model::CreateFromOBJ("forest_ground", true);
	modelDirtBlock_ = Model::CreateFromOBJ("dirt_block", true);
	modelPlayer_ = Model::CreateFromOBJ("player", true);
	modelPlayerBody_ = Model::CreateFromOBJ("player_body", true);
	modelPlayerLeftLeg_ = Model::CreateFromOBJ("player_left_leg", true);
	modelPlayerRightLeg_ = Model::CreateFromOBJ("player_right_leg", true);
	modelPlayerHammer_ = Model::CreateFromOBJ("player_hammer", true);

	modelHitEffect_ = KamataEngine::Model::CreateFromOBJ("particle", true);
	modelHitSpike_ = KamataEngine::Model::CreateFromOBJ("hit_spike", true);
	modelHitStar_ = KamataEngine::Model::CreateFromOBJ("hit_star", true);
	HitEffect::SetModel(modelHitEffect_);
	HitEffect::SetSpikeModel(modelHitSpike_);
	HitEffect::SetStarModel(modelHitStar_);
	HitEffect::SetCamera(&camera_);

	modelGuardEffect_ = Model::CreateFromOBJ("baria", true);
	GuardEffect::SetModel(modelGuardEffect_);
	GuardEffect::SetCamera(&camera_);

	// "enemy" フォルダから OBJ モデルを生成（テクスチャ読み込みも有効に）
	enemyModel_ = Model::CreateFromOBJ("enemy", true);
	bossBodyModel_ = Model::CreateFromOBJ("boss_body", true);
	bossWingNxModel_ = Model::CreateFromOBJ("boss_wing_nx", true);
	bossWingPxModel_ = Model::CreateFromOBJ("boss_wing_px", true);
	independentWingNxTransform_.Initialize();
	independentWingPxTransform_.Initialize();
	summonedEnemyModel_ = Model::CreateFromOBJ("slime", true);

	// "shieldenemy" フォルダから OBJ モデルを生成
	shieldEnemyModel_ = Model::CreateFromOBJ("shieldEnemy", true);

	// --- 表示ブロックの生成 (ここが重要！) ---
	// 以前の (i+j)%2 のループは消して、この一行にまとめます
	GenerateFieldObjects();


	assert(modelMap_);
	assert(modelGround_);
	assert(modelDirtBlock_);
	Playermodel = modelMap_;

	cameraController_ = std::make_unique<CameraController>();
	cameraController_->Initialize(&camera_);
	cameraController_->SetScrollSpeed(0.18f);
	player_->SetAutoRunSpeed(cameraController_->GetScrollSpeed());


	CameraController::Rect area;
	area.bottom = -50.0f; // マップの下端（調整はお好みでOK）
	area.top = 50.0f;     // マップの上端（調整はお好みでOK）

	cameraController_->SetMovableArea(area);


	// --- スライド：カメラコントローラの初期化手順 ---
	// 3. 追従対象をセット（Playerのポインタを渡す）
	cameraController_->SetTarget(player_);
	// 4. リセット（瞬間合わせ）
	cameraController_->Reset();
	gameplayCameraPosition_ = camera_.translation_;
	InitializeForestBackground();

	// ボスは専用の胴体モデルを使い、画面上の右側に固定する。
	Vector3 bossPosition = player_->GetWorldPosition();
	boss_ = new Enemy();
	// boss.objは使わず、胴体モデルをEnemy本体として使う。左右の羽は描画時に別モデルで重ねる。
	boss_->Initialize(bossBodyModel_, &camera_, bossPosition);
	boss_->SetAsScreenBoss(8.0f);
	// ボス本体の通常時の縦揺れを止める。
	boss_->SetScreenBossBodyMotionEnabled(false);
	// 専用ボスモデルは左右に羽が広がる向きなので、画面正面向きで表示する。
	 boss_->SetScreenBossRotationY(-std::numbers::pi_v<float> / 2.0f);
	boss_->Update(); // 初回描画より前に画面右側へ移動しておく。
	// 開始時はカメラをボスに正対させ、画面中央へ寄せる。
	const Vector3 bossWorldPosition = boss_->GetWorldPosition();
	introCameraPosition_ = gameplayCameraPosition_;
	introCameraPosition_.x = bossWorldPosition.x;
	introCameraPosition_.y = bossWorldPosition.y;
	camera_.translation_ = introCameraPosition_;
	camera_.UpdateMatrix();
	skydome_->Update(camera_.translation_);
	boss_->SetScreenBossIntroMode(true);
	boss_->SetScreenBossPosition(0.72f, 1.15f); // 右上から登場
	boss_->Update();

	fade_ = new Fade();
	fade_->Initialize();

	// まずはボス登場演出を見せる。
	phase_ = Phase::kFadeIn;
	fade_->Start(Fade::Status::kFadeIn, 1.0f); // 1秒かけて画面を明るくする
}





void GameScene::Update() {
	if (isPaused_) {
		if (Input::GetInstance()->TriggerKey(DIK_ESCAPE)) {
			isPaused_ = false;
			return;
		}
		if (Input::GetInstance()->TriggerKey(DIK_W)) {
			pauseMenuSelection_ = (pauseMenuSelection_ + 2) % 3;
			PlayUiCursorMoveSound();
		} else if (Input::GetInstance()->TriggerKey(DIK_S)) {
			pauseMenuSelection_ = (pauseMenuSelection_ + 1) % 3;
			PlayUiCursorMoveSound();
		}
		if (Input::GetInstance()->TriggerKey(DIK_RETURN) || Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			PlayUiConfirmSound();
			switch (pauseMenuSelection_) {
			case 0:
				isPaused_ = false;
				break;
			case 1:
				reloadRequested_ = true;
				break;
			case 2:
				finished_ = true;
				break;
			}
		}
		return;
	}
	if (phase_ == Phase::kPlay && Input::GetInstance()->TriggerKey(DIK_ESCAPE)) {
		isPaused_ = true;
		pauseMenuSelection_ = 0;
		return;
	}
	if (phase_ == Phase::kStageClear) {
		if (Input::GetInstance()->TriggerKey(DIK_W) || Input::GetInstance()->TriggerKey(DIK_S)) {
			clearMenuSelection_ = (clearMenuSelection_ + 1) % 2;
			PlayUiCursorMoveSound();
		}
		if (Input::GetInstance()->TriggerKey(DIK_RETURN) || Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			PlayUiConfirmSound();
			if (clearMenuSelection_ == 0) {
				reloadRequested_ = true;
			} else {
				finished_ = true;
			}
		}
		return;
	}
	if (phase_ == Phase::kPlay) {
		elapsedGameplayTime_ += 1.0f / 60.0f;
	}
#ifdef USE_IMGUI
	ImGui::Begin("Window");
	ImGui::Text("GameScene finished_: %s", finished_ ? "TRUE" : "FALSE");
	ImGui::Text("deathParticles_: %s", deathParticles_ && deathParticles_->IsFinished() ? "TRUE" : "FALSE");
	ImGui::Text("Boss HP: %d / %d", bossHp_, kBossMaxHp);
	ImGui::Text("Player HP: %d / %d", player_ ? player_->GetHP() : 0, Player::GetMaxHP());
	ImGui::Text("Boss body model pointer: %s", bossBodyModel_ ? "non-null" : "null");
	ImGui::Text("Boss wing NX model pointer: %s", bossWingNxModel_ ? "non-null" : "null");
	ImGui::Text("Boss wing PX model pointer: %s", bossWingPxModel_ ? "non-null" : "null");
	ImGui::Text("Boss body meshes: %llu", static_cast<unsigned long long>(bossBodyModel_ ? bossBodyModel_->GetMeshes().size() : 0));
	ImGui::Text("Boss wing NX meshes: %llu", static_cast<unsigned long long>(bossWingNxModel_ ? bossWingNxModel_->GetMeshes().size() : 0));
	ImGui::Text("Boss wing PX meshes: %llu", static_cast<unsigned long long>(bossWingPxModel_ ? bossWingPxModel_->GetMeshes().size() : 0));
	ImGui::Text("Wing NX vertices/indices: %llu / %llu", static_cast<unsigned long long>(bossWingNxModel_ && !bossWingNxModel_->GetMeshes().empty() ? bossWingNxModel_->GetMeshes()[0]->GetVertexCount() : 0), static_cast<unsigned long long>(bossWingNxModel_ && !bossWingNxModel_->GetMeshes().empty() ? bossWingNxModel_->GetMeshes()[0]->GetIndices().size() : 0));
	ImGui::Text("Wing PX vertices/indices: %llu / %llu", static_cast<unsigned long long>(bossWingPxModel_ && !bossWingPxModel_->GetMeshes().empty() ? bossWingPxModel_->GetMeshes()[0]->GetVertexCount() : 0), static_cast<unsigned long long>(bossWingPxModel_ && !bossWingPxModel_->GetMeshes().empty() ? bossWingPxModel_->GetMeshes()[0]->GetIndices().size() : 0));

	if (phase_ == Phase::kPlay) {
		ImGui::Text("Current Phase: kPlay");
	} else if (phase_ == Phase::kDeath) {
		ImGui::Text("Current Phase: kDeath");
	} else {
		ImGui::Text("Current Phase: Unknown");
	}

#ifdef _DEBUG
	if (ImGui::Button("Reload Map")) {
		reloadRequested_ = true;
	}
#endif

	ImGui::End();
#endif
	for (const std::unique_ptr<SummonMagicCircle>& circlePointer : summonMagicCircles_) {
		SummonMagicCircle& circle = *circlePointer;
		circle.age += 1.0f;
		const float appear = (std::clamp)(circle.age / 12.0f, 0.0f, 1.0f);
		const float appearEase = appear * appear * (3.0f - 2.0f * appear);
		const float fade = (std::clamp)((72.0f - circle.age) / 20.0f, 0.0f, 1.0f);
		circle.worldTransform.scale_.x = 5.0f * appearEase;
		circle.worldTransform.scale_.y = 1.75f * appearEase;
		circle.worldTransform.matWorld_ = MakeAffineMatrix(circle.worldTransform.scale_, circle.worldTransform.rotation_, circle.worldTransform.translation_);
		circle.worldTransform.TransferMatrix();
		circle.objectColor.SetColor({1.0f, 1.0f, 1.0f, fade});
	}
	summonMagicCircles_.remove_if(
		[](const std::unique_ptr<SummonMagicCircle>& circle) { return circle->age >= 72.0f; });
	// ヒットストップ中はゲーム世界を止め、ヒットエフェクトだけを進める。
	if (phase_ == Phase::kPlay && hitStopFrames_ > 0) {
		--hitStopFrames_;
		for (HitEffect* effect : hitEffects_) {
			effect->Update();
		}
		cameraController_->UpdateShake();
		if (skydome_) {
			skydome_->Update(camera_.translation_);
		}
		return;
	}

	switch (phase_) {
	case Phase::kFadeIn:
		// ● フェードイン中はフェードの更新を行う
		if (fade_) {
			fade_->Update();

			// ● フェードイン中にフェードが終わったら次のフェーズ（None）に切り替える
			if (fade_->IsFinished()) {
				phase_ = Phase::kBossIntroDrop;
				bossIntroTimer_ = 0.0f;
			}
		}
		break;

	case Phase::kBossIntroDrop: {
		bossIntroTimer_ += 1.0f / 60.0f;
		const float t = (std::min)(bossIntroTimer_ / 0.9f, 1.0f);
		const float easedT = t * t * (3.0f - 2.0f * t);
		boss_->SetScreenBossPosition(0.72f * (1.0f - easedT), 1.15f * (1.0f - easedT));
		if (t >= 1.0f) {
			boss_->SetScreenBossPosition(0.0f, 0.0f);
			phase_ = Phase::kBossIntroLand;
			bossIntroTimer_ = 0.0f;
			cameraController_->StartShake(20, 0.8f);
		}
		break;
	}

	case Phase::kBossIntroLand: {
		bossIntroTimer_ += 1.0f / 60.0f;
		const float t = (std::min)(bossIntroTimer_ / 0.28f, 1.0f);
		const float bounce = std::sin(t * std::numbers::pi_v<float>) * 0.06f;
		boss_->SetScreenBossPosition(0.0f, bounce);
		cameraController_->UpdateShake();
		if (t >= 1.0f) {
			boss_->SetScreenBossPosition(0.0f, 0.0f);
			boss_->SetScreenBossRoaring(true);
			PlayBossRoarSound();
			phase_ = Phase::kBossIntroRoar;
			bossIntroTimer_ = 0.0f;
			cameraController_->StartShake(24, 0.28f);
		}
		break;
	}

	case Phase::kBossIntroRoar:
		bossIntroTimer_ += 1.0f / 60.0f;
		cameraController_->UpdateShake();
		if (bossIntroTimer_ >= 1.8f) {
			phase_ = Phase::kBossIntroCameraReturn;
			bossIntroTimer_ = 0.0f;
			boss_->SetScreenBossRoaring(false);
			boss_->SetScreenBossIntroMode(false);
		}
		break;

	case Phase::kBossIntroCameraReturn: {
		bossIntroTimer_ += 1.0f / 60.0f;
		const float t = (std::min)(bossIntroTimer_ / 0.75f, 1.0f);
		const float easedT = t * t * (3.0f - 2.0f * t);
		camera_.translation_.x = introCameraPosition_.x + (gameplayCameraPosition_.x - introCameraPosition_.x) * easedT;
		camera_.translation_.y = introCameraPosition_.y + (gameplayCameraPosition_.y - introCameraPosition_.y) * easedT;
		boss_->SetScreenBossPosition(0.78f * easedT, -0.5f * easedT);
		if (t >= 1.0f) {
			boss_->SetScreenBossPosition(0.78f, -0.5f);
			player_->SetAutoRunSpeed(cameraController_->GetScrollSpeed());
			elapsedGameplayTime_ = 0.0f;
			phase_ = Phase::kPlay;
		}
		break;
	}



	case Phase::kPlay: //  通常のゲームプレイ中の処理

		// Playerの更新
		if (player_) {
			const bool phaseTransitionActive = bossPhase_ == BossPhase::kTransition;
			player_->SetControlsLocked(phaseTransitionActive);
			cameraController_->SetTransitionScrollLock(phaseTransitionActive);
			// カメラとプレイヤーの少し先まで床を確保して、進行に合わせて列を追加する。
			const KamataEngine::Vector3 playerPosition = player_->GetWorldPosition();
			float leadingX = camera_.translation_.x;
			if (playerPosition.x > leadingX) {
				leadingX = playerPosition.x;
			}
			const float generationAhead = 160.0f;
			const float requiredWorldX = leadingX + generationAhead;
			const uint32_t requiredColumn = static_cast<uint32_t>(
				(requiredWorldX + MapChipField::kBlockWidth / 2.0f) / MapChipField::kBlockWidth);
			// ボスを倒した時点で、このステージの地形追加を止める。
			if (bossHp_ > 0) {
				mapChipField_->EnsureGeneratedThrough(requiredColumn);
				AppendGeneratedFieldColumns();
			}

			player_->Update();
		}

		UpdateBossAttack();
		UpdateEnemyBullets();

		// 移動が終わった後に、当たり判定を呼び出す！
		CheckAllCollisions();
		if (!player_->IsSpinAttacking()) {
			bossHitThisAttack_ = false;
		}

		// ブロックの行列更新
		for (auto& line : worldTransformBlocks_) {
			for (auto& block : line) {
				if (block == nullptr)
					continue;
				if (std::fabs(block->translation_.x - camera_.translation_.x) > 100.0f)
					continue;
				UpdateWorldTransform(*block);
			}
		}

		if (player_->IsDead()) {
			// 死亡演出フェーズに切り替え
			phase_ = Phase::kDeath;

			// 自キャラの座標を取得
			const WorldTransform& deathPlayerTransform = player_->GetWorldTransform();
			delete deathParticles_;
			deathParticles_ = new DeatParticles();
			deathParticles_->Initialize(modelPlayer_, modelHitEffect_, &camera_, deathPlayerTransform);

		}

		break;

	case Phase::kDeath: //  プレイヤーがやられた時の演出処理

		// デスパーティクル（爆発）だけを更新する
		if (deathParticles_) {
			deathParticles_->Update();
		}

		if (deathParticles_ && deathParticles_->IsFinished()) {
			phase_ = Phase::kFadeOut;

			if (fade_) {
				fade_->Start(Fade::Status::kFadeOut, 1.0f); // 1秒かけて暗くする
			}
		}

		break;

		case Phase::kFadeOut:
		// ● フェードアウト中はフェードの更新を行う
		if (fade_) {
			fade_->Update();

				// ● フェードアウト中にフェードが終わったらゲームシーンを終了する
			if (fade_->IsFinished()) {
				finished_ = true;
			}

		}
		break;
	}



	#ifdef _DEBUG
	if (phase_ == Phase::kPlay && Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		isDebugCameraActive_ = !isDebugCameraActive_;
	}
	#endif

		// 敵の更新
	for (Enemy* enemy : enemies_) {
		enemy->Update();
	}

	//シールドの敵の更新
	for (ShieldEnemy* shieldEnemy : shieldEnemies_) {
		shieldEnemy->Update();
	}

	for (HitEffect* effect : hitEffects_) {
		effect->Update(); // 1つずつ順番に動かす
	}

	hitEffects_.remove_if([](HitEffect* hitEffect) {
		if (hitEffect->IsDead()) {
			delete hitEffect; // メモリを解放（消滅）
			return true;      // リストから削除する
		}
		return false;
	});

	enemies_.remove_if([](Enemy* enemy) {
		if (enemy->IsDead()) { // 👈 先ほど Enemy.h に作った関数
			delete enemy;      // メモリから消す
			return true;       // リストから削除する
		}
		return false; // 生きているので残す
	});

	// シールドエネミーのリストから削除する処理
	shieldEnemies_.remove_if([](ShieldEnemy* shieldEnemy) {
		if (shieldEnemy->IsDead()) {
			delete shieldEnemy;
			return true;
		}
		return false;
	});

	// 👑 hitEffects_ の更新ループの下あたりに追加
	for (GuardEffect* effect : guardEffects_) {
		effect->Update();
	}

	// 👑 デスフラグが立ったガードエフェクトを削除
	guardEffects_.remove_if([](GuardEffect* effect) {
		if (effect->IsDead()) { // コピー元にIsDead関数がある前提です
			delete effect;
			return true;
		}
		return false;
	});


	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			// 穴あき（nullptr）の場所は飛ばす
			if (worldTransformBlock == nullptr)
				continue;

			worldTransformBlock->matWorld_ = MakeAffineMatrix(worldTransformBlock->scale_, worldTransformBlock->rotation_, worldTransformBlock->translation_);
			worldTransformBlock->TransferMatrix();
		}
	}


	if (isDebugCameraActive_) {
		debugCamera_->Update();
		camera_.matView = debugCamera_->GetCamera().matView;

		camera_.matProjection = debugCamera_->GetCamera().matProjection;

		camera_.TransferMatrix();
	} else {
		camera_.UpdateMatrix();
	}

		// 登場演出中はカメラ追従と強制スクロールを止める。
	const bool isBossIntro = phase_ == Phase::kFadeIn || phase_ == Phase::kBossIntroDrop ||
		phase_ == Phase::kBossIntroLand || phase_ == Phase::kBossIntroRoar || phase_ == Phase::kBossIntroCameraReturn;
	if (!isBossIntro) {
		cameraController_->Update();
	}
	if (phase_ == Phase::kPlay) {
		UpdateBossPhase();
	}
	if (boss_) {
		boss_->Update();
		bossWingAnimationTime_ += 1.0f / 60.0f;
	}
	if (skydome_) {
		skydome_->Update(camera_.translation_);
	}
	if (phase_ == Phase::kPlay && boss_ && bossHp_ <= 0 && boss_->IsDead()) {
		StopGameBgm();
		PlayResultSound();
		phase_ = Phase::kStageClear;
		clearMenuSelection_ = 0;
		const WorldTransform& bossTransform = boss_->GetWorldTransform();
		defeatedBossTransform_.scale_ = {bossTransform.scale_.x * 1.2f, bossTransform.scale_.y * 0.32f, bossTransform.scale_.z * 1.2f};
		defeatedBossTransform_.rotation_ = {0.0f, -std::numbers::pi_v<float> / 2.0f, 0.0f};
		defeatedBossTransform_.translation_ = bossTransform.translation_;
		defeatedBossTransform_.translation_.y -= 2.8f;
		defeatedBossTransform_.matWorld_ = MakeAffineMatrix(defeatedBossTransform_.scale_, defeatedBossTransform_.rotation_, defeatedBossTransform_.translation_);
		defeatedBossTransform_.TransferMatrix();
	}
}

void GameScene::Draw() {
	Model::PreDraw(Model::CullingMode::kNone);
	PrimitiveDrawer* primitiveDrawer = PrimitiveDrawer::GetInstance();
	primitiveDrawer->Reset();
	primitiveDrawer->SetCamera(&camera_);

	// 天球を背景に描き、その手前にブロックで組んだ林を描く。
	if (skydome_) {
		skydome_->Draw(&camera_);
	}
	DrawForestBackground();

	for (std::size_t rowIndex = 0; rowIndex < worldTransformBlocks_.size(); ++rowIndex) {
		std::vector<WorldTransform*>& worldTransformBlockLine = worldTransformBlocks_[rowIndex];
		for (std::size_t columnIndex = 0; columnIndex < worldTransformBlockLine.size(); ++columnIndex) {
			WorldTransform* worldTransformBlock = worldTransformBlockLine[columnIndex];
			// 穴あきの場所で Draw を呼ぶと落ちます。
			if (worldTransformBlock == nullptr) {
				continue;
			}
			if (std::fabs(worldTransformBlock->translation_.x - camera_.translation_.x) > 100.0f) {
				continue;
			}

			// 直上が空いているブロックだけ草付きにし、それより下は土ブロックにする。
			const bool isTopSurfaceBlock = rowIndex == 0 ||
				mapChipField_->GetMapChipTypeByIndex(static_cast<uint32_t>(columnIndex), static_cast<uint32_t>(rowIndex - 1)) != MapChipType::kBlock;
			Model* blockModel = isTopSurfaceBlock ? modelGround_ : modelDirtBlock_;
			if (blockModel) {
				blockModel->Draw(*worldTransformBlock, camera_);
			}
		}
	}
	DrawSummonMagicCircles();
	// Playerの描画
	if (phase_ != Phase::kDeath && phase_ != Phase::kFadeOut) {
		if (player_) {
			player_->Draw(&camera_);
		}
	}

	for (Enemy* enemy : enemies_) {
		enemy->Draw(&camera_);
	}
	if (boss_ && !boss_->IsDead()) {
		const WorldTransform& bossTransform = boss_->GetWorldTransform();
		boss_->Draw(&camera_);
		const float flapCycle = std::fmod(bossWingAnimationTime_ * 1.1f, 1.0f);
		constexpr float kUpperWingHold = 0.12f;
		const float flapPhase = flapCycle < kUpperWingHold ? 0.0f : (flapCycle - kUpperWingHold) / (1.0f - kUpperWingHold);
		// 上限位置で少し静止させ、そこから下降・上昇する。上限を越えて振らない。
		const float flapProgress = 0.5f - 0.5f * std::cos(flapPhase * 2.0f * std::numbers::pi_v<float>);
		const float flapAngle = flapProgress * 1.57f;
		auto drawBossWing = [&](Model* wingModel, WorldTransform& wingTransform, const Vector3& localOffset, float direction) {
			if (!wingModel) {
				return;
			}
			wingTransform.scale_ = bossTransform.scale_;
			wingTransform.scale_.x *= 1.5f;
			// NX (画面右側) は先端をカメラと反対へ向けるため、X方向を反転する。
			if (direction < 0.0f) {
				wingTransform.scale_.x *= -1.0f;
			}
			const float yaw = bossTransform.rotation_.y;
			// 画面右側のNXは、モデルの奥行き成分に負けないよう横振りを大きくして右へ寄せる。
			const float horizontalSwing = flapProgress * (direction < 0.0f ? 1.0f : 0.35f);
			const float wingYaw = (direction > 0.0f ? yaw + std::numbers::pi_v<float> : yaw) + horizontalSwing;
			// NXはX反転しているので、左右とも同じ負角で回すと羽先が下へ向かう。
			const float flap = -flapAngle;
			// 羽の付け根をローカルZ軸で回してから、ボスの向きを適用する。
			const Matrix4x4 attachmentBasis = MakeAffineMatrix(bossTransform.scale_, { 0.0f, yaw, 0.0f }, {});
			const Vector3 worldOffset = {
				localOffset.x * attachmentBasis.m[0][0] + localOffset.y * attachmentBasis.m[1][0] + localOffset.z * attachmentBasis.m[2][0],
				localOffset.x * attachmentBasis.m[0][1] + localOffset.y * attachmentBasis.m[1][1] + localOffset.z * attachmentBasis.m[2][1],
				localOffset.x * attachmentBasis.m[0][2] + localOffset.y * attachmentBasis.m[1][2] + localOffset.z * attachmentBasis.m[2][2],
			};
			wingTransform.rotation_ = { 0.0f, wingYaw, flap };
			wingTransform.translation_ = {
				bossTransform.translation_.x + worldOffset.x,
				bossTransform.translation_.y + worldOffset.y,
				bossTransform.translation_.z + worldOffset.z,
			};

			// ボスの向きを適用して羽をZ軸方向へ広げ、ローカルZ軸回転で羽先を上下させる。
			const float cosYaw = std::cos(wingYaw);
			const float sinYaw = std::sin(wingYaw);
			const float cosFlap = std::cos(flap);
			const float sinFlap = std::sin(flap);
			Matrix4x4& world = wingTransform.matWorld_;
			world.m[0][0] = wingTransform.scale_.x * cosFlap * cosYaw;
			world.m[0][1] = wingTransform.scale_.x * sinFlap;
			world.m[0][2] = -wingTransform.scale_.x * cosFlap * sinYaw;
			world.m[0][3] = 0.0f;
			world.m[1][0] = -wingTransform.scale_.y * sinFlap * cosYaw;
			world.m[1][1] = wingTransform.scale_.y * cosFlap;
			world.m[1][2] = wingTransform.scale_.y * sinFlap * sinYaw;
			world.m[1][3] = 0.0f;
			world.m[2][0] = wingTransform.scale_.z * sinYaw;
			world.m[2][1] = 0.0f;
			world.m[2][2] = wingTransform.scale_.z * cosYaw;
			world.m[2][3] = 0.0f;
			world.m[3][0] = wingTransform.translation_.x;
			world.m[3][1] = wingTransform.translation_.y;
			world.m[3][2] = wingTransform.translation_.z;
			world.m[3][3] = 1.0f;
			wingTransform.TransferMatrix();
			wingModel->Draw(wingTransform, camera_);
		};
		drawBossWing(bossWingNxModel_, independentWingNxTransform_, { -0.24f, 0.08f, -0.18f }, -1.0f);
		drawBossWing(bossWingPxModel_, independentWingPxTransform_, { 0.24f, 0.08f, -0.18f }, 1.0f);
	}
	if (phase_ == Phase::kStageClear && bossBodyModel_) {
		bossBodyModel_->Draw(defeatedBossTransform_, camera_);
	}
	if (phase_ == Phase::kStageClear) {
		if (bossWingNxModel_) { bossWingNxModel_->Draw(independentWingNxTransform_, camera_); }
		if (bossWingPxModel_) { bossWingPxModel_->Draw(independentWingPxTransform_, camera_); }
	}

	for (EnemyBullet* bullet : enemyBullets_) {
		bullet->Draw();
	}

	// 👑【追加】シールドエネミーの描画
	for (ShieldEnemy* shieldEnemy : shieldEnemies_) {
		shieldEnemy->Draw(&camera_);
	}

	for (HitEffect* effect : hitEffects_) {
		effect->Draw(); // 1つずつ順番に画面に描画する
	}

	for (GuardEffect* effect : guardEffects_) {
		effect->Draw();
	}

	if (deathParticles_) {
		deathParticles_->Draw(&camera_);
	}
	// 画面演出はImGuiではなくゲーム本体の3Dラインとして描く。
	// Releaseでも同じ描画経路を通る。
	if (phase_ == Phase::kBossIntroRoar) {
		DrawBossIntroConcentrationLines();
	}
	if (phase_ == Phase::kPlay) {
		DrawBossAttackTelegraph();
	}

	Model::PostDraw();
	if (phase_ == Phase::kPlay) {
		DrawBossTackleTelegraph();
	}

	if (phase_ == Phase::kFadeIn || phase_ == Phase::kFadeOut) {
		if (fade_) {
			fade_->Draw();
		}
	}

	if (!isPaused_ && phase_ == Phase::kPlay && pauseHintSprite_) {
		pauseHintSprite_->SetPosition({24.0f, 24.0f});
		Sprite::PreDraw();
		pauseHintSprite_->Draw();
		if (attackHintSprite_) {
			attackHintSprite_->SetPosition({24.0f, 78.0f});
			attackHintSprite_->Draw();
		}
		if (evadeHintSprite_) {
			evadeHintSprite_->SetPosition({24.0f, 132.0f});
			evadeHintSprite_->Draw();
		}
		Sprite::PostDraw();
	}
	if (!isPaused_ && phase_ == Phase::kPlay && player_) {
		const int currentHp = (std::clamp)(player_->GetHP(), 0, Player::GetMaxHP());
		Sprite::PreDraw();
		for (int i = 0; i < Player::GetMaxHP(); ++i) {
			Sprite* heart = playerHpSprites_[i];
			if (!heart) {
				continue;
			}
			heart->SetPosition({24.0f + i * 44.0f, 188.0f});
			heart->SetSize({40.0f, 40.0f});
			heart->SetColor(i < currentHp ? Vector4{1.0f, 1.0f, 1.0f, 1.0f} : Vector4{0.28f, 0.28f, 0.34f, 0.85f});
			heart->Draw();
		}
		if (dodgeCooldownFrameSprite_ && dodgeCooldownBackSprite_ && dodgeCooldownFillSprite_) {
			constexpr float barX = 24.0f;
			constexpr float barY = 236.0f;
			constexpr float barWidth = 240.0f;
			constexpr float barHeight = 18.0f;
			constexpr float inset = 3.0f;
			dodgeCooldownFrameSprite_->SetPosition({barX, barY});
			dodgeCooldownFrameSprite_->SetSize({barWidth, barHeight});
			dodgeCooldownFrameSprite_->Draw();
			dodgeCooldownBackSprite_->SetPosition({barX + inset, barY + inset});
			dodgeCooldownBackSprite_->SetSize({barWidth - inset * 2.0f, barHeight - inset * 2.0f});
			dodgeCooldownBackSprite_->Draw();
			const float fillRatio = (std::clamp)(player_->GetDodgeCooldownRatio(), 0.0f, 1.0f);
			if (fillRatio > 0.0f) {
				dodgeCooldownFillSprite_->SetPosition({barX + inset, barY + inset});
				dodgeCooldownFillSprite_->SetSize({(barWidth - inset * 2.0f) * fillRatio, barHeight - inset * 2.0f});
				dodgeCooldownFillSprite_->Draw();
			}
		}
		Sprite::PostDraw();
	}
	if (phase_ == Phase::kPlay && boss_ && bossHpBarFrameSprite_ && bossHpBarFillSprite_) {
		const float screenWidth = static_cast<float>(DirectXCommon::GetInstance()->GetBackBufferWidth());
		const float screenHeight = static_cast<float>(DirectXCommon::GetInstance()->GetBackBufferHeight());
		const float scale = (std::min)(screenWidth / 1280.0f, screenHeight / 720.0f);
		const float barWidth = 720.0f * scale;
		const float barHeight = 56.0f * scale;
		const float barX = (screenWidth - barWidth) * 0.5f;
		const float barY = 60.0f * scale + (screenHeight - 720.0f * scale) * 0.5f;
		const float nameWidth = 410.0f * scale;
		const float nameHeight = 44.0f * scale;
		const float nameX = (screenWidth - nameWidth) * 0.5f;
		const float viewportOffsetY = (screenHeight - 720.0f * scale) * 0.5f;
		if (bossNameSprite_) {
			bossNameSprite_->SetPosition({nameX, 8.0f * scale + viewportOffsetY});
			bossNameSprite_->SetSize({nameWidth, nameHeight});
		}
		int currentPhaseHp = 0;
		switch (bossPhase_) {
		case BossPhase::kPhaseOne:
			currentPhaseHp = bossHp_ - kBossPhaseTwoHp;
			break;
		case BossPhase::kPhaseTwo:
			currentPhaseHp = bossHp_ - kBossPhaseThreeHp;
			break;
		case BossPhase::kPhaseThree:
			currentPhaseHp = bossHp_;
			break;
		case BossPhase::kTransition:
			currentPhaseHp = 0;
			break;
		}
		currentPhaseHp = (std::clamp)(currentPhaseHp, 0, kBossPhaseMaxHp);
		float fillRatio = static_cast<float>(currentPhaseHp) / static_cast<float>(kBossPhaseMaxHp);
		if (bossPhase_ == BossPhase::kTransition) {
			fillRatio = bossTransitionRoarStarted_ ? 1.0f :
				(std::clamp)(bossPhaseTimer_ / kBossPhaseBarRefillDuration, 0.0f, 1.0f);
			const float easedProgress = 1.0f - (1.0f - fillRatio) * (1.0f - fillRatio) * (1.0f - fillRatio);
			fillRatio = easedProgress;
		}
		const float fillWidth = (barWidth - 68.0f * scale) * fillRatio;
		bossHpBarFrameSprite_->SetPosition({barX, barY});
		bossHpBarFrameSprite_->SetSize({barWidth, barHeight});
		Sprite::PreDraw();
		if (bossNameSprite_) { bossNameSprite_->Draw(); }
		bossHpBarFrameSprite_->Draw();
		if (fillWidth > 0.0f) {
			bossHpBarFillSprite_->SetPosition({barX + 34.0f * scale, barY + 10.0f * scale});
			bossHpBarFillSprite_->SetSize({fillWidth, 36.0f * scale});
			bossHpBarFillSprite_->Draw();
		}
		Sprite::PostDraw();
	}
	const bool bossAttackAlertVisible = bossTelegraphActive_ ||
		((bossPhase_ == BossPhase::kPhaseTwo || bossPhase_ == BossPhase::kPhaseThree) &&
			bossTackleState_ == BossTackleState::kTelegraph);
	if (phase_ == Phase::kPlay && !isPaused_ && boss_ && !boss_->IsDead() &&
		bossAttackAlertVisible && bossAttackAlertSprite_) {
		const float screenWidth = static_cast<float>(DirectXCommon::GetInstance()->GetBackBufferWidth());
		const float screenHeight = static_cast<float>(DirectXCommon::GetInstance()->GetBackBufferHeight());
		const Vector3 bossPosition = boss_->GetWorldPosition();
		const float depth = bossPosition.z - camera_.translation_.z;
		const float projectionScaleX = camera_.matProjection.m[0][0];
		const float projectionScaleY = camera_.matProjection.m[1][1];
		if (screenWidth > 0.0f && screenHeight > 0.0f && depth > 0.001f &&
			projectionScaleX > 0.0f && projectionScaleY > 0.0f) {
			const AABB bossBounds = boss_->GetAABB();
			const float ndcX = (bossPosition.x - camera_.translation_.x) * projectionScaleX / depth;
			const float headNdcY = (bossBounds.max.y + 0.75f - camera_.translation_.y) * projectionScaleY / depth;
			const float headScreenX = (ndcX + 1.0f) * 0.5f * screenWidth;
			const float headScreenY = (1.0f - headNdcY) * 0.5f * screenHeight;
			const float iconSize = 72.0f * (std::min)(screenWidth / 1280.0f, screenHeight / 720.0f);
			const float iconX = (std::clamp)(headScreenX - iconSize * 0.5f, 8.0f, screenWidth - iconSize - 8.0f);
			const float iconY = (std::clamp)(headScreenY - iconSize, 8.0f, screenHeight - iconSize - 8.0f);
			bossAttackAlertSprite_->SetPosition({iconX, iconY});
			bossAttackAlertSprite_->SetSize({iconSize, iconSize});
			Sprite::PreDraw();
			bossAttackAlertSprite_->Draw();
			Sprite::PostDraw();
		}
	}
	if (phase_ == Phase::kStageClear && clearMenuSprites_[clearMenuSelection_]) {
		const float screenWidth = static_cast<float>(DirectXCommon::GetInstance()->GetBackBufferWidth());
		const float screenHeight = static_cast<float>(DirectXCommon::GetInstance()->GetBackBufferHeight());
		Sprite* clearMenu = clearMenuSprites_[clearMenuSelection_];
		const float scale = (std::min)(screenWidth / 1280.0f, screenHeight / 720.0f);
		const float menuWidth = 665.0f * scale;
		const float menuHeight = 550.0f * scale;
		const Vector2 menuPosition = {(screenWidth - menuWidth) * 0.5f, (screenHeight - menuHeight) * 0.5f};
		if (clearBackgroundShadeSprite_) {
			clearBackgroundShadeSprite_->SetPosition({0.0f, 0.0f});
			clearBackgroundShadeSprite_->SetSize({screenWidth, screenHeight});
		}
		clearMenu->SetTextureRect({50.0f, 110.0f}, {665.0f, 550.0f});
		clearMenu->SetPosition(menuPosition);
		clearMenu->SetSize({menuWidth, menuHeight});
		Sprite::PreDraw();
		if (clearBackgroundShadeSprite_) { clearBackgroundShadeSprite_->Draw(); }
		clearMenu->Draw();
		const int totalSeconds = static_cast<int>(elapsedGameplayTime_);
		const int minutes = (std::min)(totalSeconds / 60, 99);
		const int seconds = totalSeconds % 60;
		const int digits[5] = {minutes / 10, minutes % 10, 10, seconds / 10, seconds % 10};
		const float positions[5] = {542.0f, 590.0f, 638.0f, 666.0f, 714.0f};
		for (int i = 0; i < 5; ++i) {
			Sprite* digit = clearTimeDigitSprites_[i];
			if (!digit) {
				continue;
			}
			digit->SetTextureRect({static_cast<float>(digits[i] * 48), 0.0f}, {48.0f, 72.0f});
			digit->SetPosition({positions[i] * scale + (screenWidth - 1280.0f * scale) * 0.5f, 312.0f * scale + (screenHeight - 720.0f * scale) * 0.5f});
			digit->SetSize({(i == 2 ? 20.0f : 42.0f) * scale, 60.0f * scale});
			digit->Draw();
		}
		Sprite::PostDraw();
	}
	if (isPaused_) {
		Sprite* pauseSprite = pauseMenuSprites_[pauseMenuSelection_];
		if (pauseSprite) {
			const float screenWidth = static_cast<float>(DirectXCommon::GetInstance()->GetBackBufferWidth());
			const float screenHeight = static_cast<float>(DirectXCommon::GetInstance()->GetBackBufferHeight());
			pauseSprite->SetPosition({0.0f, 0.0f});
			pauseSprite->SetSize({screenWidth, screenHeight});
			Sprite::PreDraw();
			pauseSprite->Draw();
			Sprite::PostDraw();
		}
	}
}

GameScene::~GameScene() {
	StopGameBgm();
	delete modelBlock_;
	delete modelBossBullet_;
	delete modelForestTree_;
	delete modelForestGrass_;
	delete modelSummonMagicCircle_;
	delete skydome_;
	delete modelSkyDome_;
	delete debugCamera_;
	delete mapChipField_;
	delete enemyModel_;
	delete summonedEnemyModel_;
	delete boss_;
	for (EnemyBullet* bullet : enemyBullets_) {
		delete bullet;
	}
	enemyBullets_.clear();
	delete player_;
	delete Playermodel;
	delete modelPlayerBody_;
	delete modelPlayerLeftLeg_;
	delete modelPlayerRightLeg_;
	delete modelPlayerHammer_;
	delete deathParticles_;
	delete fade_;
	delete modelHitEffect_;
	delete modelHitSpike_;
	delete modelHitStar_;
	delete bossTackleWarningSprite_;
	delete bossAttackAlertSprite_;
	delete pauseHintSprite_;
	delete attackHintSprite_;
	delete evadeHintSprite_;
	delete bossNameSprite_;
	delete bossHpBarFrameSprite_;
	delete bossHpBarFillSprite_;
	delete dodgeCooldownFrameSprite_;
	delete dodgeCooldownBackSprite_;
	delete dodgeCooldownFillSprite_;
	for (Sprite* heartSprite : playerHpSprites_) { delete heartSprite; }
	delete clearBackgroundShadeSprite_;
	for (Sprite* clearSprite : clearMenuSprites_) { delete clearSprite; }
	for (Sprite* digitSprite : clearTimeDigitSprites_) { delete digitSprite; }
	for (Sprite* pauseSprite : pauseMenuSprites_) {
		delete pauseSprite;
	}

	for (Enemy* enemy : enemies_) {
		delete enemy; // メモリを1体ずつ解放
	}
	enemies_.clear(); // リスト自体を空っぽにする

	// シールドエネミーのメモリ解放
	for (ShieldEnemy* shieldEnemy : shieldEnemies_) {
		delete shieldEnemy;
	}

	for (HitEffect* effect : hitEffects_) {
		delete effect; // メモリから1つずつ削除する
	}
	hitEffects_.clear(); // リストの要素の殻を空っぽにする

	for (GuardEffect* effect : guardEffects_) {
		delete effect;
	}
	guardEffects_.clear(); // リストの要素の殻を空っぽにする


	for(std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {

		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			delete worldTransformBlock;
		}
	}
	worldTransformBlocks_.clear();
	forestBackgroundBlocks_.clear();

}

void GameScene::GenerateFieldObjects() {
	// 既存のデータをクリア（念のため）
	for (auto& line : worldTransformBlocks_) {
		for (auto& block : line) {
			delete block;
		}
	}
	worldTransformBlocks_.clear();

	// マップチップの縦・横サイズに合わせてリサイズ
	// ※MapChipField::kNumBlockVertical などを public にしている前提
	const uint32_t initialColumnCount = mapChipField_->GetGeneratedColumnCount();
	worldTransformBlocks_.resize(MapChipField::kNumBlockVertical);
	for (uint32_t i = 0; i < MapChipField::kNumBlockVertical; ++i) {
		worldTransformBlocks_[i].resize(initialColumnCount);
		for (uint32_t j = 0; j < initialColumnCount; ++j) {

			// マップチップの種別を取得
			MapChipType chipType = mapChipField_->GetMapChipTypeByIndex(j, i);

			// 種別に応じて生成するオブジェクトを分岐する
			switch (chipType) {
			case MapChipType::kBlock: //  ブロックの場合
				worldTransformBlocks_[i][j] = new KamataEngine::WorldTransform();
				worldTransformBlocks_[i][j]->Initialize();

				{
					// ブロックの位置を設定
					::Vector3 pos = mapChipField_->GetMapChipPositionByIndex(j, i);
					worldTransformBlocks_[i][j]->translation_.x = pos.x;
					worldTransformBlocks_[i][j]->translation_.y = pos.y;
					worldTransformBlocks_[i][j]->translation_.z = pos.z;
				}

				// ブロックのサイズを設定
				worldTransformBlocks_[i][j]->scale_ = {MapChipField::kBlockWidth, MapChipField::kBlockHeight, MapChipField::kBlockWidth};
				break;

			case MapChipType::kPlayer: { // 🏃 プレイヤーの場合
				// 自キャラを二重に配置しようとしていないかチェック
				assert(player_ == nullptr && "自キャラを二重に配置しようとしています");

				// 自キャラの生成
				player_ = new Player();

				// 座標をマップチップ番号（j:横、i:縦）から計算して取得
				Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(j, i);

				// 座標を指定してキャラの初期化
				player_->Initialize(modelPlayerBody_, modelPlayerLeftLeg_, modelPlayerRightLeg_, modelPlayerHammer_, &camera_, playerPosition);//学校のスライドには書いてない

				// 自キャラにマップチップ情報をセット
				player_->SetMapChipField(mapChipField_);

				// プレイヤーがいるマスはブロックではないので nullptr を入れる←これも学校のスライドには書いてない
				worldTransformBlocks_[i][j] = nullptr;
				break;
			}

			case MapChipType::kEnemy: {
				// サブID（0か1か）を取得
				uint8_t subID = mapChipField_->GetMapChipSubIDByIndex(j, i);

				// スライドの指示通り、敵のcase分だけで関数化したものを呼び出す
				GenerateEnemyFromMapChip(subID, j, i);

				// 敵がいるマスはブロックではないので nullptr を入れる
				worldTransformBlocks_[i][j] = nullptr;
				break;
			}

			case MapChipType::kBlank:  //  空白の場合
			default:                   // その他（何もない場所）
				// ブロックの表示オブジェクトは生成しない（nullptrで埋める）
				worldTransformBlocks_[i][j] = nullptr;
				break;

			}
		}
	}
	generatedBlockColumnCount_ = initialColumnCount;
}

void GameScene::AppendGeneratedFieldColumns() {
	const uint32_t targetColumnCount = mapChipField_->GetGeneratedColumnCount();
	for (uint32_t column = generatedBlockColumnCount_; column < targetColumnCount; ++column) {
		for (uint32_t row = 0; row < MapChipField::kNumBlockVertical; ++row) {
			worldTransformBlocks_[row].push_back(nullptr);
			if (mapChipField_->GetMapChipTypeByIndex(column, row) != MapChipType::kBlock) {
				continue;
			}

			KamataEngine::WorldTransform* block = new KamataEngine::WorldTransform();
			block->Initialize();
			const ::Vector3 position = mapChipField_->GetMapChipPositionByIndex(column, row);
			block->translation_ = {position.x, position.y, position.z};
			block->scale_ = {MapChipField::kBlockWidth, MapChipField::kBlockHeight, MapChipField::kBlockWidth};
			worldTransformBlocks_[row][column] = block;
		}
	}
	generatedBlockColumnCount_ = targetColumnCount;
}

void GameScene::CheckAllCollisions() {

#pragma region 自キャラと敵キャラの当たり判定

	// 自キャラのAABBを取得
	AABB aabb1 = player_->GetAABB();

	// ボス弾とプレイヤーの判定。回避中は弾をすり抜ける。
	for (auto it = enemyBullets_.begin(); it != enemyBullets_.end();) {
		EnemyBullet* bullet = *it;
		if (!player_->IsDodging() && IsCollision(aabb1, bullet->GetAABB())) {
			player_->OnCollision(boss_);
			delete bullet;
			it = enemyBullets_.erase(it);
		} else {
			++it;
		}
	}

	// 自キャラと敵すべての当たり判定をループで処理
	for (Enemy* enemy : enemies_) {

		if (enemy->IsCollisionDisabled()) {
			continue;
		}

		// 敵のAABBを取得
		AABB aabb2 = enemy->GetAABB();

		// 回避中は敵との接触判定をしない（地形との判定はPlayer側で継続）。
		if (player_->IsDodging()) {
			continue;
		}

		// 回転攻撃はプレイヤー中心の球形範囲で命中を判定する。
		if (player_->IsSpinAttacking() && player_->IsSpinAttackHit(aabb2)) {
			PlayHammerHitSound();
			enemy->OnCollision(player_);
			hitStopFrames_ = 6;
			cameraController_->StartShake(30, 0.18f);
		} else if (IsCollision(aabb1, aabb2)) {
			player_->OnCollision(enemy);
		}
	}

	// 画面固定で管理しているボスも、プレイヤーとの接触を判定する。
	if (boss_ && !boss_->IsCollisionDisabled()) {
		const AABB bossAabb = boss_->GetAABB();
		const bool isBossTackling = (bossPhase_ == BossPhase::kPhaseTwo || bossPhase_ == BossPhase::kPhaseThree) &&
			bossTackleState_ == BossTackleState::kDashing;
		AABB bossContactAabb = bossAabb;
		if (isBossTackling && bossTacklePreviousAabbValid_) {
			// 高速移動でフレーム間にプレイヤーを通り越しても当たるよう、前後の範囲をつなぐ。
			bossContactAabb.min.x = (std::min)(bossTacklePreviousAabb_.min.x, bossAabb.min.x);
			bossContactAabb.min.y = (std::min)(bossTacklePreviousAabb_.min.y, bossAabb.min.y);
			bossContactAabb.min.z = (std::min)(bossTacklePreviousAabb_.min.z, bossAabb.min.z);
			bossContactAabb.max.x = (std::max)(bossTacklePreviousAabb_.max.x, bossAabb.max.x);
			bossContactAabb.max.y = (std::max)(bossTacklePreviousAabb_.max.y, bossAabb.max.y);
			bossContactAabb.max.z = (std::max)(bossTacklePreviousAabb_.max.z, bossAabb.max.z);
		}
		if (player_->IsSpinAttacking() && player_->IsSpinAttackHit(bossAabb)) {
			if (bossHp_ > 0 && !bossHitThisAttack_) {
				PlayHammerHitSound();
				--bossHp_;
				if (bossHp_ == kBossPhaseTwoHp && bossPhase_ == BossPhase::kPhaseOne) {
					bossPhase_ = BossPhase::kTransition;
					bossTransitionToPhaseThree_ = false;
					bossTransitionRoarStarted_ = false;
					bossPhaseTimer_ = 0.0f;
					bossTelegraphActive_ = false;
					boss_->SetScreenBossRoaring(false);
				} else if (bossHp_ == kBossPhaseThreeHp && bossPhase_ == BossPhase::kPhaseTwo) {
					bossPhase_ = BossPhase::kTransition;
					bossTransitionToPhaseThree_ = true;
					bossTransitionRoarStarted_ = false;
					bossPhaseTimer_ = 0.0f;
					bossTackleState_ = BossTackleState::kWaiting;
					bossTackleTimer_ = 0.0f;
					bossTackleHitPlayer_ = false;
					bossTacklePreviousAabbValid_ = false;
					bossTelegraphActive_ = false;
					boss_->SetScreenBossRoaring(false);
				}
				bossHitThisAttack_ = true;
				const bool bossDefeated = bossHp_ == 0;
				CreateHitEffectAtContact(player_->GetWorldPosition(), bossAabb, bossDefeated);
				hitStopFrames_ = 6;
				cameraController_->StartShake(30, 0.35f);
				if (bossHp_ > 0) {
					boss_->StartHurtAnimation();
				} else {
					boss_->StartDeathAnimation();
				}
				cameraController_->PauseScroll();
				player_->StartBossHitKnockback();
			}
		} else if (((bossPhase_ != BossPhase::kPhaseTwo && bossPhase_ != BossPhase::kPhaseThree) ||
			(isBossTackling && !bossTackleHitPlayer_)) &&
			!player_->IsDodging() && IsCollision(aabb1, bossContactAabb) && !player_->IsKnockback()) {
			player_->OnCollision(boss_);
			if (isBossTackling) {
				bossTackleHitPlayer_ = true;
				cameraController_->StartShake(12, 0.16f);
			}
		}
		bossTacklePreviousAabb_ = bossAabb;
		bossTacklePreviousAabbValid_ = isBossTackling;
	}

	for (ShieldEnemy* shieldEnemy : shieldEnemies_) {
		// すでに死んでいる、またはコリジョン無効ならスキップ
		if (shieldEnemy->IsCollisionDisabled()) {
			continue;
		}

		// シールドエネミーのAABBを取得
		AABB aabb2 = shieldEnemy->GetAABB();

		if (player_->IsDodging()) {
			continue;
		}

		if (player_->IsSpinAttacking() && player_->IsSpinAttackHit(aabb2)) {
			PlayHammerHitSound();
			shieldEnemy->OnCollision(player_);
			hitStopFrames_ = 6;
			cameraController_->StartShake(30, 0.25f);
		} else if (IsCollision(aabb1, aabb2)) {
			player_->OnCollision(shieldEnemy);
			shieldEnemy->OnCollision(player_);
		}
	}

#pragma endregion
}

void GameScene::ChangePhase() {
	switch (phase_) {
	case Phase::kPlay:
		// プレイフェーズの初期化
		break;

	case Phase::kDeath:
		if (deathParticles_) {
			deathParticles_->Update();
		}

		//パーティクルが終了したら、ゲームシーン全体の終了フラグを立てる
		if (deathParticles_ && deathParticles_->IsFinished()) {
			finished_ = true;
		}
		break;
	}
}

void GameScene::InitializeForestBackground() {
	forestBackgroundBlocks_.clear();
	const auto addBlock = [this](float x, float y, float width, float height, float parallax, float worldZ, const Vector4& tint) {
		auto block = std::make_unique<ForestBackgroundBlock>();
		block->worldTransform.Initialize();
		block->objectColor.Initialize();
		block->objectColor.SetColor(tint);
		block->layoutX = x;
		block->layoutY = y;
		block->layoutWidth = width;
		block->layoutHeight = height;
		block->parallax = parallax;
		block->worldZ = worldZ;
		forestBackgroundBlocks_.push_back(std::move(block));
	};
	const auto addGroundBlock = [this, &addBlock](float x, float y, float width, float height, float parallax, float worldZ, const Vector4& tint) {
		addBlock(x, y, width, height, parallax, worldZ, tint);
		forestBackgroundBlocks_.back()->isGroundTile = true;
	};
	const auto addCenteredBlock = [&addBlock](float centerX, float centerY, float width, float height, float parallax, float worldZ, const Vector4& tint) {
		addBlock(centerX - width * 0.5f, centerY - height * 0.5f, width, height, parallax, worldZ, tint);
	};
	const auto addTree = [this](float centerX, float groundY, float width, float height, float parallax, float worldZ, const Vector4& tint) {
		auto tree = std::make_unique<ForestBackgroundBlock>();
		tree->worldTransform.Initialize();
		tree->objectColor.Initialize();
		tree->objectColor.SetColor(tint);
		tree->layoutX = centerX - width * 0.5f;
		tree->layoutY = groundY - height;
		tree->layoutWidth = width;
		tree->layoutHeight = height;
		tree->parallax = parallax;
		tree->worldZ = worldZ;
		tree->usesTreeModel = true;
		forestBackgroundBlocks_.push_back(std::move(tree));
	};
	const auto addGrass = [this](float centerX, float groundY, float width, float height, float parallax, float worldZ, const Vector4& tint) {
		auto grass = std::make_unique<ForestBackgroundBlock>();
		grass->worldTransform.Initialize();
		grass->objectColor.Initialize();
		grass->objectColor.SetColor(tint);
		grass->layoutX = centerX - width * 0.5f;
		grass->layoutY = groundY - height;
		grass->layoutWidth = width;
		grass->layoutHeight = height;
		grass->parallax = parallax;
		grass->worldZ = worldZ;
		grass->usesGrassModel = true;
		forestBackgroundBlocks_.push_back(std::move(grass));
	};

	// OBJ内の材質色に奥行きごとの色味を重ね、空気遠近法を表現する。
	// 高さの違う地面レイヤー間から空が見えないよう、最背面に連続した下地を敷く。
	addGroundBlock(-640.0f, 560.0f, 2560.0f, 160.0f, 0.0f, 68.0f, Vector4{0.34f, 0.58f, 0.30f, 1.0f});
	forestBackgroundBlocks_.back()->isBackgroundGround = true;
	// 既存の背景地面。位置・高さ・サイズはそのまま維持する。
	addGroundBlock(-640.0f, 620.0f, 2560.0f, 100.0f, 0.0f, 67.0f, Vector4{0.34f, 0.58f, 0.30f, 1.0f});
	forestBackgroundBlocks_.back()->isBackgroundGround = true;

	const Vector4 farTint{1.10f, 1.10f, 1.06f, 1.0f};
	const Vector4 midTint{0.90f, 1.00f, 0.88f, 1.0f};
	const Vector4 nearTint{0.78f, 0.92f, 0.76f, 1.0f};
	const Vector4 farSoil{0.28f, 0.35f, 0.23f, 1.0f};
	const Vector4 farGrass{0.43f, 0.61f, 0.34f, 1.0f};
	const Vector4 midSoil{0.22f, 0.29f, 0.17f, 1.0f};
	const Vector4 midGrass{0.29f, 0.48f, 0.22f, 1.0f};
	const Vector4 nearSoil{0.18f, 0.24f, 0.13f, 1.0f};
	const Vector4 nearGrass{0.21f, 0.39f, 0.16f, 1.0f};

	// 遠景・中景・近景にOBJの木を配置。木同士が重なりにくい間隔を取る。
	for (float x : {120.0f, 640.0f, 1140.0f}) {
		addTree(x, 615.0f, 170.0f, 230.0f, 0.12f, 65.0f, farTint);
	}
	for (float x : {180.0f, 650.0f, 1120.0f}) {
		addTree(x, 655.0f, 210.0f, 275.0f, 0.28f, 40.0f, midTint);
	}
	for (float x : {320.0f, 960.0f}) {
		addTree(x, 695.0f, 250.0f, 310.0f, 0.48f, 18.0f, nearTint);
	}
	// 草もOBJモデルに置き換え、地面に沿って間隔を空けて配置する。
	for (float x : {100.0f, 400.0f, 700.0f, 1000.0f}) {
		addGrass(x, 695.0f, 82.0f, 92.0f, 0.48f, 17.5f, nearTint);
	}
	for (float x : {260.0f, 820.0f}) {
		addGrass(x, 655.0f, 58.0f, 70.0f, 0.28f, 39.5f, midTint);
	}
	// 木の根元が浮いて見えないよう、各奥行きの木の下に土と草地を敷く。
	addGroundBlock(0.0f, 625.0f, 1280.0f, 95.0f, 0.12f, 65.8f, farSoil);
	addGroundBlock(0.0f, 615.0f, 1280.0f, 10.0f, 0.12f, 65.8f, farGrass);
	addGroundBlock(0.0f, 665.0f, 1280.0f, 55.0f, 0.28f, 40.8f, midSoil);
	addGroundBlock(0.0f, 655.0f, 1280.0f, 10.0f, 0.28f, 40.8f, midGrass);
	addGroundBlock(0.0f, 705.0f, 1280.0f, 15.0f, 0.48f, 18.8f, nearSoil);
	addGroundBlock(0.0f, 695.0f, 1280.0f, 10.0f, 0.48f, 18.8f, nearGrass);
	// ここで画面用の配置図をワールド座標へ一度だけ変換する。
	// 以降の描画ではカメラに合わせてY/Zや大きさを貼り直さず、立体物として扱う。
	const float screenWidth = static_cast<float>(DirectXCommon::GetInstance()->GetBackBufferWidth());
	const float screenHeight = static_cast<float>(DirectXCommon::GetInstance()->GetBackBufferHeight());
	const float projectionScaleX = camera_.matProjection.m[0][0];
	const float projectionScaleY = camera_.matProjection.m[1][1];
	if (screenWidth <= 0.0f || screenHeight <= 0.0f || projectionScaleX <= 0.0f || projectionScaleY <= 0.0f) {
		return;
	}
	forestBackgroundOriginCameraX_ = camera_.translation_.x;
	for (const auto& block : forestBackgroundBlocks_) {
		const float depth = block->worldZ - camera_.translation_.z;
		if (depth <= 0.001f) {
			continue;
		}
		const float pixelsPerUnitX = screenWidth * projectionScaleX / (2.0f * depth);
		const float pixelsPerUnitY = screenHeight * projectionScaleY / (2.0f * depth);
		WorldTransform& transform = block->worldTransform;
		transform.translation_.x = camera_.translation_.x + (block->layoutX + block->layoutWidth * 0.5f - 640.0f) / pixelsPerUnitX;
		transform.translation_.y = camera_.translation_.y + (360.0f - block->layoutY - block->layoutHeight * 0.5f) / pixelsPerUnitY;
		transform.translation_.z = block->worldZ;
		transform.scale_ = {block->layoutWidth / pixelsPerUnitX, block->layoutHeight / pixelsPerUnitY, 0.0f};
		// 幅に応じた奥行きを持たせ、Y軸にも傾けてブロックの側面を見せる。
		transform.scale_.z = block->isBackgroundGround ? 0.002f : (std::max)(transform.scale_.x * (block->usesTreeModel ? 0.65f : (block->usesGrassModel ? 0.28f : 0.35f)), 0.35f);
		transform.rotation_ = {0.0f, 0.0f, 0.0f};
		block->baseWorldX = transform.translation_.x;
		block->tileWorldWidth = 1280.0f / pixelsPerUnitX;
		transform.matWorld_ = MakeAffineMatrix(transform.scale_, transform.rotation_, transform.translation_);
		transform.TransferMatrix();
	}
}

void GameScene::DrawForestBackground() {
	if (!modelMap_ || !modelBlock_) {
		return;
	}
	const float cameraDeltaX = camera_.translation_.x - forestBackgroundOriginCameraX_;
	for (const auto& block : forestBackgroundBlocks_) {
		const float period = block->tileWorldWidth;
		if (period <= 0.001f) {
			continue;
		}
		// カメラ移動の一部だけ背景に反映し、奥の木ほどゆっくり流す。
		const float unwrappedWorldX = block->baseWorldX + cameraDeltaX * (1.0f - block->parallax);
		const int centerTile = static_cast<int>(std::floor((camera_.translation_.x - unwrappedWorldX) / period));
		for (int tile = centerTile - 2; tile <= centerTile + 2; ++tile) {
			WorldTransform& transform = block->worldTransform;
			transform.translation_.x = unwrappedWorldX + static_cast<float>(tile) * period;
			// 地面は画面内に残っている間描画し、木や草は従来の余白設定でカリングする。
			const float halfBlockWidth = transform.scale_.x * 0.5f;
			const float cullMargin = block->isGroundTile ? 0.0f : period * -0.05f;
			if (transform.translation_.x + halfBlockWidth < camera_.translation_.x - period * 0.5f - cullMargin ||
				transform.translation_.x - halfBlockWidth > camera_.translation_.x + period * 0.5f + cullMargin) {
				continue;
			}
			transform.matWorld_ = MakeAffineMatrix(transform.scale_, transform.rotation_, transform.translation_);
			transform.TransferMatrix();
			if (block->usesTreeModel && modelForestTree_) {
				modelForestTree_->Draw(transform, camera_, &block->objectColor);
			} else if (block->usesGrassModel && modelForestGrass_) {
				modelForestGrass_->Draw(transform, camera_, &block->objectColor);
			} else {
				modelBlock_->Draw(transform, camera_, textureHandleWhite_, &block->objectColor);
			}
		}
	}
}

void GameScene::DrawSummonMagicCircles() {
	if (summonMagicCircles_.empty() || !modelSummonMagicCircle_) {
		return;
	}
	for (const std::unique_ptr<SummonMagicCircle>& circle : summonMagicCircles_) {
		modelSummonMagicCircle_->Draw(circle->worldTransform, camera_, &circle->objectColor);
	}
}

void GameScene::UpdateBossAttack() {
	constexpr float kPhaseOneShotInterval = 1.8f;
	constexpr float kPhaseTwoShotInterval = 2.8f;
	constexpr float kTelegraphDuration = 0.5f;
	constexpr float kBulletSpeed = 0.55f;
	if (!boss_ || bossHp_ <= 0 || boss_->IsDead()) {
		bossTelegraphActive_ = false;
		return;
	}
	if (bossPhase_ == BossPhase::kTransition) {
		bossTelegraphActive_ = false;
		return;
	}
	// タックル中は弾を重ねず、待機状態に戻ったら弾攻撃を再開する。
	if ((bossPhase_ == BossPhase::kPhaseTwo || bossPhase_ == BossPhase::kPhaseThree) &&
		bossTackleState_ != BossTackleState::kWaiting) {
		bossTelegraphActive_ = false;
		return;
	}
	const float shotInterval = (bossPhase_ == BossPhase::kPhaseTwo || bossPhase_ == BossPhase::kPhaseThree)
		? kPhaseTwoShotInterval : kPhaseOneShotInterval;

	bossShotTimer_ += 1.0f / 60.0f;
	if (bossShotTimer_ >= shotInterval - kTelegraphDuration && !bossTelegraphActive_) {
		// 予告線を出し始めた時点のプレイヤー位置を狙いとして固定する。
		Vector3 muzzlePosition = boss_->GetWorldPosition();
		muzzlePosition.x = boss_->GetAABB().min.x - 0.8f;
		const Vector3 target = player_->GetWorldPosition();
		const float dx = target.x - muzzlePosition.x;
		const float dy = target.y - muzzlePosition.y;
		const float dz = target.z - muzzlePosition.z;
		bossAttackLineLength_ = std::sqrt(dx * dx + dy * dy + dz * dz);
		if (bossAttackLineLength_ > 0.001f) {
			bossAttackDirection_ = {dx / bossAttackLineLength_, dy / bossAttackLineLength_, dz / bossAttackLineLength_};
		} else {
			bossAttackDirection_ = {-1.0f, 0.0f, 0.0f};
			bossAttackLineLength_ = 1.0f;
		}
		bossTelegraphActive_ = true;
		PlayBossWarningSound();
	}
	if (bossShotTimer_ < shotInterval) {
		return;
	}
	bossShotTimer_ = 0.0f;
	bossTelegraphActive_ = false;

	// 予告線と同じ方向へ直線弾を撃つ。
	Vector3 origin = boss_->GetWorldPosition();
	origin.x = boss_->GetAABB().min.x - 0.8f;
	const Vector3 velocity{bossAttackDirection_.x * kBulletSpeed, bossAttackDirection_.y * kBulletSpeed, bossAttackDirection_.z * kBulletSpeed};
	EnemyBullet* bullet = new EnemyBullet();
	bullet->Initialize(modelBossBullet_, &camera_, mapChipField_, origin, velocity);
	enemyBullets_.push_back(bullet);
}

void GameScene::UpdateBossPhase() {
	constexpr float kDeltaTime = 1.0f / 60.0f;
	constexpr float kTransitionDuration = 1.2f;
	constexpr float kSummonInterval = 7.0f;
	constexpr float kSummonRoarDuration = 0.8f;
	constexpr float kWaitBeforeTackle = 4.0f;
	constexpr float kTackleTelegraphDuration = 1.0f;
	constexpr float kTackleDashDuration = 1.0f;
	constexpr float kTackleReturnDuration = 0.9f;
	constexpr float kBossScreenX = 0.78f;
	constexpr float kBossScreenY = -0.5f;
	constexpr float kTackleExitScreenX = -1.2f;
	constexpr float kTackleReturnScreenX = 1.15f;
	constexpr float kTackleReturnScreenY = 1.15f;

	if (bossPhase_ == BossPhase::kTransition) {
		bossPhaseTimer_ += kDeltaTime;
		if (!bossTransitionRoarStarted_ && bossPhaseTimer_ >= kBossPhaseBarRefillDuration) {
			bossTransitionRoarStarted_ = true;
			bossPhaseTimer_ = 0.0f;
			boss_->SetScreenBossRoaring(true);
			PlayBossRoarSound();
			cameraController_->StartShake(bossTransitionToPhaseThree_ ? 120 : 72, 0.34f);
		 return;
		}
		const float roarDuration = bossTransitionToPhaseThree_ ? 2.0f : kTransitionDuration;
		if (bossTransitionRoarStarted_ && bossPhaseTimer_ >= roarDuration) {
			boss_->SetScreenBossRoaring(false);
			bossPhase_ = bossTransitionToPhaseThree_ ? BossPhase::kPhaseThree : BossPhase::kPhaseTwo;
			bossTransitionRoarStarted_ = false;
			bossTackleState_ = BossTackleState::kWaiting;
			bossTackleTimer_ = 0.0f;
			bossShotTimer_ = 0.0f;
			bossPhaseTimer_ = 0.0f;
			if (bossTransitionToPhaseThree_) {
				bossTransitionToPhaseThree_ = false;
				bossSummonTimer_ = 0.0f;
				bossSummonRoarTimer_ = 0.0f;
				// 咆哮の後に召喚し、フェーズ移行を見せる。
				PlayBossSummonSound();
				SummonNormalEnemy(0.78f);
				SummonNormalEnemy(0.98f);
			}
		}
		return;
	}

	if (!boss_ || bossHp_ <= 0 || boss_->IsDead()) {
		return;
	}

	if (bossPhase_ == BossPhase::kPhaseThree) {
		if (bossSummonRoarTimer_ > 0.0f) {
			bossSummonRoarTimer_ -= kDeltaTime;
			if (bossSummonRoarTimer_ <= 0.0f) {
				bossSummonRoarTimer_ = 0.0f;
				boss_->SetScreenBossRoaring(false);
			}
		}

		bossSummonTimer_ += kDeltaTime;
		if (bossSummonTimer_ >= kSummonInterval) {
			bossSummonTimer_ = 0.0f;
			size_t aliveEnemyCount = 0;
			for (Enemy* enemy : enemies_) {
				if (enemy && !enemy->IsDead()) {
					++aliveEnemyCount;
				}
			}
			constexpr size_t kMaxActiveEnemies = 6;
			if (aliveEnemyCount < kMaxActiveEnemies) {
				bossSummonRoarTimer_ = kSummonRoarDuration;
				boss_->SetScreenBossRoaring(true);
				PlayBossSummonSound();
				SummonNormalEnemy(0.96f);
			}
		}
	}

	// フェーズ3では召喚に加え、フェーズ2までの弾とタックルも続ける。
	if (bossPhase_ != BossPhase::kPhaseTwo && bossPhase_ != BossPhase::kPhaseThree) {
		return;
	}

	bossTackleTimer_ += kDeltaTime;
	switch (bossTackleState_) {
	case BossTackleState::kWaiting:
		boss_->SetScreenBossPosition(kBossScreenX, kBossScreenY);
		if (bossTackleTimer_ >= kWaitBeforeTackle) {
			bossTackleTimer_ = 0.0f;
			bossTackleState_ = BossTackleState::kTelegraph;
			PlayBossWarningSound();
			bossTelegraphActive_ = false;
			bossShotTimer_ = 0.0f;
			bossTackleHitPlayer_ = false;
			boss_->SetScreenBossRoaring(true);

			// プレイヤーの位置に関係なく、画面下部の固定高さを通過する。
			constexpr float kTackleTargetScreenY = -0.8f;
			bossTackleTargetX_ = kTackleExitScreenX;
			bossTackleTargetY_ = kTackleTargetScreenY;
		}
		break;

	case BossTackleState::kTelegraph:
		boss_->SetScreenBossPosition(kBossScreenX, kBossScreenY);
		boss_->SetScreenBossRoaring(true);
		if (bossTackleTimer_ >= kTackleTelegraphDuration) {
			bossTackleTimer_ = 0.0f;
			bossTackleState_ = BossTackleState::kDashing;
			bossTelegraphActive_ = false;
			boss_->SetScreenBossRoaring(false);
			bossTackleStartX_ = kBossScreenX;
			bossTackleStartY_ = kBossScreenY;
			cameraController_->StartShake(10, 0.12f);
		}
		break;

	case BossTackleState::kDashing: {
		const float t = (std::clamp)(bossTackleTimer_ / kTackleDashDuration, 0.0f, 1.0f);
		const float easedT = t * t * (3.0f - 2.0f * t);
		boss_->SetScreenBossPosition(
			bossTackleStartX_ + (bossTackleTargetX_ - bossTackleStartX_) * easedT,
			bossTackleStartY_ + (bossTackleTargetY_ - bossTackleStartY_) * easedT);
		if (t >= 1.0f) {
			bossTackleTimer_ = 0.0f;
			bossTackleState_ = BossTackleState::kReturning;
			// 左の画面外まで抜けた後、右上の画面外から定位置へ戻る。
			bossTackleStartX_ = kTackleReturnScreenX;
			bossTackleStartY_ = kTackleReturnScreenY;
			boss_->SetScreenBossPosition(bossTackleStartX_, bossTackleStartY_);
		}
		break;
	}

	case BossTackleState::kReturning: {
		const float t = (std::clamp)(bossTackleTimer_ / kTackleReturnDuration, 0.0f, 1.0f);
		const float easedT = t * t * (3.0f - 2.0f * t);
		boss_->SetScreenBossPosition(
			bossTackleStartX_ + (kBossScreenX - bossTackleStartX_) * easedT,
			bossTackleStartY_ + (kBossScreenY - bossTackleStartY_) * easedT);
		if (t >= 1.0f) {
			bossTackleTimer_ = 0.0f;
			bossTackleState_ = BossTackleState::kWaiting;
		}
		break;
	}
	}
}

void GameScene::DrawBossAttackTelegraph() {
	if (!bossTelegraphActive_ || !boss_) {
		return;
	}

	Vector3 startWorld = boss_->GetWorldPosition();
	startWorld.x = boss_->GetAABB().min.x - 0.8f;
	Vector3 endWorld{
		startWorld.x + bossAttackDirection_.x * bossAttackLineLength_,
		startWorld.y + bossAttackDirection_.y * bossAttackLineLength_,
		startWorld.z + bossAttackDirection_.z * bossAttackLineLength_};
	// 線を少しカメラ側へ出し、ボスや地形に埋もれにくくする。
	startWorld.z -= 0.08f;
	endWorld.z -= 0.08f;
	const float dx = endWorld.x - startWorld.x;
	const float dy = endWorld.y - startWorld.y;
	const float screenPlaneLength = std::sqrt(dx * dx + dy * dy);
	if (screenPlaneLength <= 0.001f) {
		return;
	}
	const float perpendicularX = -dy / screenPlaneLength;
	const float perpendicularY = dx / screenPlaneLength;
	// 半透明の外側線と濃い中心線を重ねる。
	const float blinkTime = bossShotTimer_;
	const float blink = 0.5f + 0.5f * std::sin(blinkTime * 28.0f);
	const float glowAlpha = 0.14f + blink * 0.08f;
	const float lineAlpha = 0.48f + blink * 0.22f;
	PrimitiveDrawer* drawer = PrimitiveDrawer::GetInstance();
	for (int offsetIndex = -2; offsetIndex <= 2; ++offsetIndex) {
		const float offset = static_cast<float>(offsetIndex) * 0.05f;
		const Vector3 offsetStart{startWorld.x + perpendicularX * offset, startWorld.y + perpendicularY * offset, startWorld.z};
		const Vector3 offsetEnd{endWorld.x + perpendicularX * offset, endWorld.y + perpendicularY * offset, endWorld.z};
		const float alpha = offsetIndex == 0 ? lineAlpha : glowAlpha;
		drawer->DrawLine3d(offsetStart, offsetEnd, {1.0f, 0.08f, 0.08f, alpha});
	}
}

void GameScene::DrawBossTackleTelegraph() {
	if (!bossTackleWarningSprite_ || !boss_ ||
		(bossPhase_ != BossPhase::kPhaseTwo && bossPhase_ != BossPhase::kPhaseThree) ||
		bossTackleState_ != BossTackleState::kTelegraph) {
		return;
	}
	const float screenWidth = static_cast<float>(DirectXCommon::GetInstance()->GetBackBufferWidth());
	const float screenHeight = static_cast<float>(DirectXCommon::GetInstance()->GetBackBufferHeight());
	if (screenWidth <= 0.0f || screenHeight <= 0.0f) {
		return;
	}
	// プレイヤーの高さを中心に、画面いっぱいの半透明な危険レーンをSprite 1枚で表示する。
	const float centerY = (1.0f - bossTackleTargetY_) * 0.5f * screenHeight;
	// ボス本体の高さも危険範囲に含め、見た目が赤い帯からはみ出さないようにする。
	constexpr float kHazardHalfHeightPixels = 100.0f;
	const float blink = 0.5f + 0.5f * std::sin(bossTackleTimer_ * 18.0f);
	bossTackleWarningSprite_->SetPosition({0.0f, centerY - kHazardHalfHeightPixels});
	bossTackleWarningSprite_->SetSize({screenWidth, kHazardHalfHeightPixels * 2.0f});
	bossTackleWarningSprite_->SetColor({1.0f, 0.02f, 0.02f, 0.16f + blink * 0.08f});
	Sprite::PreDraw();
	bossTackleWarningSprite_->Draw();
	Sprite::PostDraw();
}

void GameScene::UpdateEnemyBullets() {
	for (auto it = enemyBullets_.begin(); it != enemyBullets_.end();) {
		if ((*it)->Update()) {
			delete *it;
			it = enemyBullets_.erase(it);
		} else {
			++it;
		}
	}
}

void GameScene::CreateHitEffect(const KamataEngine::Vector3& position, bool isBossDefeat) {
	// 1. HitEffectの静的Create関数を呼び出して、新しいエフェクトを1個作る
	HitEffect* newHitEffect = HitEffect::Create(position, isBossDefeat);

	// 2. 先ほど作った大きな箱（リスト）に登録して、GameSceneに管理してもらう
	hitEffects_.push_back(newHitEffect);
}

void GameScene::DrawBossIntroConcentrationLines() {
	if (!boss_) {
		return;
	}
	const float screenWidth = static_cast<float>(DirectXCommon::GetInstance()->GetBackBufferWidth());
	const float screenHeight = static_cast<float>(DirectXCommon::GetInstance()->GetBackBufferHeight());
	const Vector3 bossPosition = boss_->GetWorldPosition();
	const float planeZ = bossPosition.z - 0.12f;
	const float depth = planeZ - camera_.translation_.z;
	const float projectionScaleX = camera_.matProjection.m[0][0];
	const float projectionScaleY = camera_.matProjection.m[1][1];
	if (screenWidth <= 0.0f || screenHeight <= 0.0f || depth <= 0.001f || projectionScaleX <= 0.0f || projectionScaleY <= 0.0f) {
		return;
	}
	auto screenToWorld = [&](float screenX, float screenY) {
		const float ndcX = screenX * 2.0f / screenWidth - 1.0f;
		const float ndcY = 1.0f - screenY * 2.0f / screenHeight;
		return Vector3{
			camera_.translation_.x + ndcX * depth / projectionScaleX,
			camera_.translation_.y + ndcY * depth / projectionScaleY,
			planeZ};
	};
	const float bossNdcX = (bossPosition.x - camera_.translation_.x) * projectionScaleX / depth;
	const float bossNdcY = (bossPosition.y - camera_.translation_.y) * projectionScaleY / depth;
	const float centerX = (bossNdcX + 1.0f) * 0.5f * screenWidth;
	const float centerY = (1.0f - bossNdcY) * 0.5f * screenHeight;
	constexpr int kRayCount = 112;
	constexpr int kStreakCount = 64;
	const float maxRadius = std::sqrt(screenWidth * screenWidth + screenHeight * screenHeight);
	const float time = bossIntroTimer_;
	const float rotation = time * 0.12f;
	const float pulse = 0.5f + 0.5f * std::sin(time * 18.0f);
	PrimitiveDrawer* drawer = PrimitiveDrawer::GetInstance();

	for (int i = 0; i < kRayCount; ++i) {
		const float angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / kRayCount + rotation;
		const float innerRadius = 95.0f + static_cast<float>((i * 47) % 105);
		const float rayPulse = 0.88f + 0.12f * std::sin(time * 13.0f + static_cast<float>(i) * 0.73f);
		const float outerRadius = maxRadius * (0.82f + static_cast<float>((i * 29) % 18) / 100.0f) * rayPulse;
		const float startX = centerX + std::cos(angle) * innerRadius;
		const float startY = centerY + std::sin(angle) * innerRadius;
		const float endX = centerX + std::cos(angle) * outerRadius;
		const float endY = centerY + std::sin(angle) * outerRadius;
		const float alpha = ((185.0f + 55.0f * (0.5f + 0.5f * std::sin(time * 16.0f + static_cast<float>(i)))) / 255.0f) * (0.9f + 0.1f * pulse);
		const Vector4 color = (i % 5 == 0) ? Vector4{1.0f, 1.0f, 1.0f, alpha} : Vector4{0.03f, 0.03f, 0.05f, alpha};
		drawer->DrawLine3d(screenToWorld(startX, startY), screenToWorld(endX, endY), color);
	}

	// 短い線を中心の外側から画面端へ流し、集中線に速度感を加える。
	for (int i = 0; i < kStreakCount; ++i) {
		const float phase = std::fmod(time * 1.35f + static_cast<float>(i) / kStreakCount, 1.0f);
		const float angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / kStreakCount + rotation * 0.55f;
		const float startRadius = 125.0f + phase * (maxRadius * 0.62f);
		const float streakLength = 22.0f + 48.0f * (0.5f + 0.5f * std::sin(time * 20.0f + static_cast<float>(i) * 1.7f));
		const float endRadius = startRadius + streakLength;
		const float startX = centerX + std::cos(angle) * startRadius;
		const float startY = centerY + std::sin(angle) * startRadius;
		const float endX = centerX + std::cos(angle) * endRadius;
		const float endY = centerY + std::sin(angle) * endRadius;
		const float fade = phase < 1.0f ? 1.0f - phase : 0.0f;
		const float alpha = fade * (i % 4 == 0 ? 0.82f : 0.68f);
		const Vector4 color = (i % 4 == 0) ? Vector4{1.0f, 1.0f, 1.0f, alpha} : Vector4{0.05f, 0.05f, 0.06f, alpha};
		drawer->DrawLine3d(screenToWorld(startX, startY), screenToWorld(endX, endY), color);
	}
}

void GameScene::CreateHitEffectAtContact(const KamataEngine::Vector3& attackCenter, const AABB& targetAabb, bool isBossDefeat) {
	// 攻撃中心に最も近い、敵の当たり判定表面をヒット位置として使う。
	const Vector3 hitPosition{
		std::clamp(attackCenter.x, targetAabb.min.x, targetAabb.max.x),
		std::clamp(attackCenter.y, targetAabb.min.y, targetAabb.max.y),
		std::clamp(attackCenter.z, targetAabb.min.z, targetAabb.max.z),
	};
	CreateHitEffect(hitPosition, isBossDefeat);
}

void GameScene::CreateGuardEffect(const KamataEngine::Vector3& position) {
	// 1. GuardEffectの静的Create関数を呼び出して、新しいエフェクトを1個作る
	GuardEffect* newGuardEffect = GuardEffect::Create(position);

	// 2. 先ほど作った大きな箱（リスト）に登録して、GameSceneに管理してもらう
	guardEffects_.push_back(newGuardEffect);
}

void GameScene::GenerateEnemyFromMapChip(uint8_t subID, uint32_t xIndex, uint32_t yIndex) {
	// マップチップのインデックスから、出現させるワールド座標を計算
	Vector3 pos = mapChipField_->GetMapChipPositionByIndex(xIndex, yIndex);

	switch (subID) {
	case 0: { //  通常の歩行敵
		Enemy* newEnemy = new Enemy();
		newEnemy->Initialize(enemyModel_, &camera_, pos);
		newEnemy->SetGameScene(this);
		enemies_.push_back(newEnemy);
		break;
	}
	case 1: { // シールドエネミー（盾敵）
		ShieldEnemy* newShieldEnemy = new ShieldEnemy();
		newShieldEnemy->Initialize(shieldEnemyModel_, &camera_, pos);
		newShieldEnemy->SetGameScene(this);
		shieldEnemies_.push_back(newShieldEnemy);
		break;
	}
	default:
		break;
	}
}

void GameScene::SummonNormalEnemy(float screenX) {
	// 召喚スライム専用サイズ。ここを変更しても通常敵とボスには影響しない。
	constexpr float kSummonedSlimeScale = 5.0f;
	constexpr float kSlimeModelBottomOffset = 0.333f;
	if (!player_ || !summonedEnemyModel_) {
		return;
	}

	Vector3 spawnPosition = player_->GetWorldPosition();
	const float viewDepth = spawnPosition.z - camera_.translation_.z;
	const float projectionScaleX = camera_.matProjection.m[0][0];
	if (viewDepth > 0.0f && projectionScaleX > 0.0f) {
		// カメラ右端付近に出し、通常敵の歩行速度でプレイヤー側へ向かわせる。
		spawnPosition.x = camera_.translation_.x + screenX * viewDepth / projectionScaleX;
	} else {
		spawnPosition.x += 24.0f;
	}
	if (mapChipField_) {
		const MapChipField::IndexSet index = mapChipField_->GetMapChipIndexSetByPosition(spawnPosition);
		float groundTop = 0.0f;
		bool foundGround = false;
		for (uint32_t yIndex = 0; yIndex < MapChipField::kNumBlockVertical; ++yIndex) {
			if (mapChipField_->GetMapChipTypeByIndex(index.xIndex, yIndex) != kBlock) {
				continue;
			}
			const MapChipField::Rect blockRect = mapChipField_->GetRectByIndex(index.xIndex, yIndex);
			if (blockRect.top <= spawnPosition.y + MapChipField::kBlockHeight &&
				(!foundGround || blockRect.top > groundTop)) {
				groundTop = blockRect.top;
				foundGround = true;
			}
		}
		if (foundGround) {
			// スライムOBJの下端が地面に接する位置に中心を置く。
			spawnPosition.y = groundTop + kSlimeModelBottomOffset * kSummonedSlimeScale;
		}
	}
	if (modelSummonMagicCircle_) {
		Vector3 circlePosition = spawnPosition;
		circlePosition.y = spawnPosition.y - kSlimeModelBottomOffset * kSummonedSlimeScale + 0.875f;
		circlePosition.z -= 0.12f;
		auto magicCircle = std::make_unique<SummonMagicCircle>();
		magicCircle->worldTransform.Initialize();
		magicCircle->worldTransform.translation_ = circlePosition;
		magicCircle->worldTransform.scale_ = {0.0f, 0.0f, 1.0f};
		magicCircle->objectColor.Initialize();
		magicCircle->objectColor.SetColor({1.0f, 1.0f, 1.0f, 1.0f});
		UpdateWorldTransform(magicCircle->worldTransform);
		summonMagicCircles_.push_back(std::move(magicCircle));
	}

	Enemy* summonedEnemy = new Enemy();
	summonedEnemy->Initialize(summonedEnemyModel_, &camera_, spawnPosition);
	summonedEnemy->SetScale(kSummonedSlimeScale);
	summonedEnemy->SetGameScene(this);
	summonedEnemy->SetGroundWalkingMapChipField(mapChipField_, kSlimeModelBottomOffset);
	enemies_.push_back(summonedEnemy);
	Vector3 summonParticlePosition = spawnPosition;
	summonParticlePosition.y -= kSlimeModelBottomOffset * kSummonedSlimeScale;
	hitEffects_.push_back(HitEffect::CreateSummon(summonParticlePosition));
}

