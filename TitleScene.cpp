#include "TitleScene.h"
#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <numbers>
#include "Math.h"
#include "UiSound.h"
#include "Bgm.h"

using namespace KamataEngine;

void TitleScene::Initialize() {
	StopGameBgm();
	StartTitleBgm();
	finished_ = false;
	phase_ = Phase::FadeIn;
	animationTimer_ = 0.0f;
	groundScrollPixels_ = 0.0f;
	enemySpawnTimer_ = 0.5f;
	attackCooldown_ = 0.0f;
	attackAnimationTimer_ = -1.0f;
	nextEnemyId_ = 1;
	attackTargetId_ = 0;
	menuSelection_ = 0;
	previewEnemies_.clear();
		backgroundProps_.clear();
	groundTiles_.clear();
	for (int i = 0; i < 32; ++i) {
		groundTiles_.emplace_back();
		groundTiles_.back().grass.Initialize();
		groundTiles_.back().dirt.Initialize();
	}


	modelSkyDome_ = Model::CreateFromOBJ("SkyDome", true);
	modelTree_ = Model::CreateFromOBJ("ForestTreeColor", true);
	if (modelTree_) {
		for (const auto& mesh : modelTree_->GetMeshes()) {
			Material* material = mesh->GetMaterial();
			if (!material) { continue; }
			if (material->name_ == "LeafDark") {
				material->diffuse_ = {0.18f, 0.88f, 0.18f};
				material->ambient_ = {0.12f, 0.36f, 0.12f};
			} else if (material->name_ == "LeafMid") {
				material->diffuse_ = {0.26f, 1.0f, 0.20f};
				material->ambient_ = {0.16f, 0.46f, 0.12f};
			} else if (material->name_ == "LeafLight") {
				material->diffuse_ = {0.36f, 1.0f, 0.22f};
				material->ambient_ = {0.22f, 0.52f, 0.14f};
			} else { continue; }
			material->Update();
		}
	}
	modelGrass_ = Model::CreateFromOBJ("ForestGrassColor", true);
	modelGround_ = Model::CreateFromOBJ("forest_ground", true);
	modelDirtBlock_ = Model::CreateFromOBJ("dirt_block", true);
	modelPlayerBody_ = Model::CreateFromOBJ("player_body", true);
	modelPlayerLeftLeg_ = Model::CreateFromOBJ("player_left_leg", true);
	modelPlayerRightLeg_ = Model::CreateFromOBJ("player_right_leg", true);
	modelPlayerHammer_ = Model::CreateFromOBJ("player_hammer", true);
	modelEnemy_ = Model::CreateFromOBJ("slime", true);
	modelTitleLogo_ = Model::CreateFromOBJ("title_logo", true);
	modelTitleBossBody_ = Model::CreateFromOBJ("boss_body", true);
	modelTitleBossWingNx_ = Model::CreateFromOBJ("boss_wing_nx", true);
	modelTitleBossWingPx_ = Model::CreateFromOBJ("boss_wing_px", true);
	menuStartSprite_ = Sprite::Create(TextureManager::Load("./Resources/title_menu/menu_start_selected.png"), {0.0f, 0.0f});
	menuExitSprite_ = Sprite::Create(TextureManager::Load("./Resources/title_menu/menu_exit_selected.png"), {0.0f, 0.0f});
	if (menuStartSprite_) { menuStartSprite_->SetSize({560.0f, 144.0f}); }
	if (menuExitSprite_) { menuExitSprite_->SetSize({560.0f, 144.0f}); }

	camera_.Initialize();
	camera_.farZ = 1000.0f;
	camera_.translation_ = {0.0f, 0.0f, -100.0f};
	camera_.UpdateMatrix();

	worldTransformSky_.Initialize();
	worldTransformPlayer_.Initialize();
	worldTransformLeftLeg_.Initialize();
	worldTransformRightLeg_.Initialize();
	worldTransformHammer_.Initialize();
	titleLogoTransform_.Initialize();
	titleBossBodyTransform_.Initialize();
	titleBossWingNxTransform_.Initialize();
	titleBossWingPxTransform_.Initialize();
	worldTransformSky_.translation_ = camera_.translation_;
	UpdateWorldTransform(worldTransformSky_);

	// 遠景・中景・近景の木をずらして置き、奥ほどゆっくり流す。
	const auto addTree = [this](float x, float width, float height, float parallax, float z) {
		backgroundProps_.emplace_back();
		BackgroundProp& prop = backgroundProps_.back();
		prop.transform.Initialize();
		prop.screenX = x;
		prop.width = width;
		prop.height = height;
		prop.groundY = groundTopPixels_ + 2.0f;
		prop.parallax = parallax;
		prop.z = z;
	};
	addTree(60.0f, 200.0f, 245.0f, 0.22f, 28.0f);
	addTree(360.0f, 230.0f, 280.0f, 0.32f, 22.0f);
	addTree(680.0f, 195.0f, 245.0f, 0.22f, 28.0f);
	addTree(990.0f, 240.0f, 285.0f, 0.32f, 22.0f);
	addTree(1260.0f, 205.0f, 250.0f, 0.22f, 28.0f);
	addTree(230.0f, 260.0f, 300.0f, 0.52f, 12.0f);
	addTree(790.0f, 250.0f, 290.0f, 0.52f, 12.0f);
	addTree(1360.0f, 250.0f, 290.0f, 0.52f, 12.0f);

	// 道の縁に草を間隔を空けて配置する。
	for (float x : {40.0f, 190.0f, 390.0f, 610.0f, 835.0f, 1070.0f, 1260.0f}) {
		backgroundProps_.emplace_back();
		BackgroundProp& prop = backgroundProps_.back();
		prop.transform.Initialize();
		prop.screenX = x;
		prop.width = 48.0f;
		prop.height = 62.0f;
		prop.groundY = groundTopPixels_ + 1.0f;
		prop.parallax = 0.9f;
		prop.z = 0.8f;
		prop.isGrass = true;
	}
	UpdatePlayerPose();

	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::kFadeIn, 2.0f);
}

void TitleScene::SetScreenTransform(WorldTransform& transform, float screenX, float screenY,
	float width, float height, float z, float depthScale) {
	const float screenWidth = static_cast<float>(DirectXCommon::GetInstance()->GetBackBufferWidth());
	const float screenHeight = static_cast<float>(DirectXCommon::GetInstance()->GetBackBufferHeight());
	const float depth = z - camera_.translation_.z;
	const float projectionScaleX = camera_.matProjection.m[0][0];
	const float projectionScaleY = camera_.matProjection.m[1][1];
	if (screenWidth <= 0.0f || screenHeight <= 0.0f || depth <= 0.001f ||
		projectionScaleX <= 0.0f || projectionScaleY <= 0.0f) {
		return;
	}
	const float pixelsPerUnitX = screenWidth * projectionScaleX / (2.0f * depth);
	const float pixelsPerUnitY = screenHeight * projectionScaleY / (2.0f * depth);
	transform.translation_.x = camera_.translation_.x + (screenX - screenWidth * 0.5f) / pixelsPerUnitX;
	transform.translation_.y = camera_.translation_.y + (screenHeight * 0.5f - screenY) / pixelsPerUnitY;
	transform.translation_.z = z;
	transform.scale_ = {width / pixelsPerUnitX, height / pixelsPerUnitY, depthScale};
	transform.rotation_ = {};
	UpdateWorldTransform(transform);
}

void TitleScene::UpdatePlayerPose() {
	constexpr float kPlayerScale = 8.0f;
	const float walkPhase = animationTimer_ * 2.0f * std::numbers::pi_v<float> * 1.6f;
	const float legSwing = std::sin(walkPhase) * 0.45f;
	const float screenHeight = static_cast<float>(DirectXCommon::GetInstance()->GetBackBufferHeight());
	const float viewDepth = -camera_.translation_.z;
	const float projectionScaleY = camera_.matProjection.m[1][1];
	const float pixelsPerUnitY = screenHeight * projectionScaleY / (2.0f * viewDepth);
	const float swing = std::abs(legSwing);
	const float lowestLegOffset = -0.04f - 0.26f * std::cos(swing) - 0.125543f * std::sin(swing);
	const float playerRootScreenY = groundTopPixels_ + lowestLegOffset * kPlayerScale * pixelsPerUnitY;

	SetScreenTransform(worldTransformPlayer_, playerScreenX_, playerRootScreenY, 1.0f, 1.0f, 0.0f, 1.0f);
	worldTransformPlayer_.scale_ = {kPlayerScale, kPlayerScale, kPlayerScale};
	worldTransformPlayer_.rotation_.y = std::numbers::pi_v<float> / 2.0f;
	worldTransformPlayer_.rotation_.z = std::sin(walkPhase) * 0.06f;
	UpdateWorldTransform(worldTransformPlayer_);

	auto updateLeg = [this, walkPhase](WorldTransform& leg, float pivotX, float swing) {
		leg.scale_ = worldTransformPlayer_.scale_;
		leg.rotation_ = worldTransformPlayer_.rotation_;
		leg.rotation_.x += swing;
		leg.translation_ = worldTransformPlayer_.translation_;
		const float yaw = worldTransformPlayer_.rotation_.y;
		const float pivotOffsetX = pivotX * worldTransformPlayer_.scale_.x;
		leg.translation_.x += std::cos(yaw) * pivotOffsetX;
		leg.translation_.y -= 0.04f * worldTransformPlayer_.scale_.y;
		leg.translation_.z -= std::sin(yaw) * pivotOffsetX;
		UpdateWorldTransform(leg);
	};
	updateLeg(worldTransformLeftLeg_, -0.085f, legSwing);
	updateLeg(worldTransformRightLeg_, 0.085f, -legSwing);

	worldTransformHammer_.scale_ = worldTransformPlayer_.scale_;
	worldTransformHammer_.rotation_ = worldTransformPlayer_.rotation_;
	worldTransformHammer_.rotation_.y += std::numbers::pi_v<float> / 2.0f;
	float hammerAngle = 0.0f;
	if (attackAnimationTimer_ >= 0.0f) {
		constexpr float kAttackDuration = 0.62f;
		const float t = (std::clamp)(attackAnimationTimer_ / kAttackDuration, 0.0f, 1.0f);
		if (t < 0.42f) {
			const float prep = t / 0.42f;
			hammerAngle = -0.9f * prep;
		} else if (t < 0.72f) {
			const float swingProgress = (t - 0.42f) / 0.30f;
			hammerAngle = -0.9f + 1.9f * swingProgress;
		} else {
			const float recover = (t - 0.72f) / 0.28f;
			hammerAngle = 1.0f * (1.0f - recover);
		}
	}
	worldTransformHammer_.rotation_.z += hammerAngle;
	worldTransformHammer_.translation_ = worldTransformPlayer_.translation_;
	constexpr float gripX = -0.33f;
	constexpr float gripY = -0.13f;
	constexpr float gripZ = 0.14f;
	const float yaw = worldTransformPlayer_.rotation_.y;
	worldTransformHammer_.translation_.x += std::cos(yaw) * gripX * worldTransformPlayer_.scale_.x +
		std::sin(yaw) * gripZ * worldTransformPlayer_.scale_.z;
	worldTransformHammer_.translation_.y += gripY * worldTransformPlayer_.scale_.y;
	worldTransformHammer_.translation_.z += -std::sin(yaw) * gripX * worldTransformPlayer_.scale_.x +
		std::cos(yaw) * gripZ * worldTransformPlayer_.scale_.z;
	UpdateWorldTransform(worldTransformHammer_);
}

void TitleScene::Update() {
	constexpr float kDeltaTime = 1.0f / 60.0f;
	constexpr float kGroundTileWidth = 128.0f;
	constexpr float kEnemySpawnInterval = 2.8f;
	constexpr float kEnemySpeed = 2.2f;
	constexpr float kAttackDuration = 0.62f;

	animationTimer_ += kDeltaTime;
	groundScrollPixels_ = std::fmod(groundScrollPixels_ + 2.4f, kGroundTileWidth);
	const float screenWidth = static_cast<float>(DirectXCommon::GetInstance()->GetBackBufferWidth());
	for (BackgroundProp& prop : backgroundProps_) {
		prop.screenX -= 2.4f * prop.parallax;
		if (prop.screenX < -prop.width * 0.5f) {
			prop.screenX += screenWidth + prop.width;
		}
	}

	enemySpawnTimer_ += kDeltaTime;
	if (enemySpawnTimer_ >= kEnemySpawnInterval && previewEnemies_.size() < 3) {
		enemySpawnTimer_ = 0.0f;
		previewEnemies_.emplace_back();
		PreviewEnemy& enemy = previewEnemies_.back();
		enemy.transform.Initialize();
		enemy.screenX = screenWidth + 70.0f;
		enemy.id = nextEnemyId_++;
	}

	if (attackCooldown_ > 0.0f) {
		attackCooldown_ = (std::max)(0.0f, attackCooldown_ - kDeltaTime);
	}
	if (attackAnimationTimer_ >= 0.0f) {
		attackAnimationTimer_ += kDeltaTime;
		const float progress = attackAnimationTimer_ / kAttackDuration;
		if (progress >= 0.56f && attackTargetId_ != 0) {
			for (PreviewEnemy& enemy : previewEnemies_) {
				if (enemy.id == attackTargetId_ && enemy.deathTimer < 0.0f) {
					enemy.deathTimer = 0.0f;
					break;
				}
			}
			attackTargetId_ = 0;
		}
		if (attackAnimationTimer_ >= kAttackDuration) {
			attackAnimationTimer_ = -1.0f;
			attackCooldown_ = 0.35f;
		}
	}

	for (PreviewEnemy& enemy : previewEnemies_) {
		if (enemy.deathTimer >= 0.0f) {
			enemy.deathTimer += kDeltaTime;
		} else {
			enemy.screenX -= kEnemySpeed;
		}
	}
	for (auto it = previewEnemies_.begin(); it != previewEnemies_.end();) {
		if (it->deathTimer >= 0.38f || it->screenX < -100.0f) {
			it = previewEnemies_.erase(it);
		} else {
			++it;
		}
	}

	if (attackAnimationTimer_ < 0.0f && attackCooldown_ <= 0.0f) {
		PreviewEnemy* target = nullptr;
		for (PreviewEnemy& enemy : previewEnemies_) {
			if (enemy.deathTimer < 0.0f && enemy.screenX > playerScreenX_ + 25.0f &&
				enemy.screenX - playerScreenX_ <= 120.0f &&
				(!target || enemy.screenX < target->screenX)) {
				target = &enemy;
			}
		}
		if (target) {
			attackAnimationTimer_ = 0.0f;
			attackTargetId_ = target->id;
		}
	}

	UpdatePlayerPose();
	for (PreviewEnemy& enemy : previewEnemies_) {
		constexpr float kSummonedSlimeScale = 5.0f;
		constexpr float kSlimeModelBottomOffset = 0.333f;
		const float squash = enemy.deathTimer >= 0.0f
			? (std::max)(0.08f, 1.0f - enemy.deathTimer / 0.38f)
			: 1.0f;
		const float screenHeight = static_cast<float>(DirectXCommon::GetInstance()->GetBackBufferHeight());
		const float viewDepth = -camera_.translation_.z;
		const float projectionScaleY = camera_.matProjection.m[1][1];
		const float pixelsPerUnitY = screenHeight * projectionScaleY / (2.0f * viewDepth);
		const float enemyRootScreenY = groundTopPixels_ -
			kSlimeModelBottomOffset * kSummonedSlimeScale * squash * pixelsPerUnitY;
		SetScreenTransform(enemy.transform, enemy.screenX, enemyRootScreenY, 1.0f, 1.0f, 0.0f, 1.0f);
		enemy.transform.scale_ = {kSummonedSlimeScale, kSummonedSlimeScale * squash, kSummonedSlimeScale};
		enemy.transform.rotation_.y = -std::numbers::pi_v<float> / 2.0f;
		UpdateWorldTransform(enemy.transform);
	}
	worldTransformSky_.translation_ = camera_.translation_;
	UpdateWorldTransform(worldTransformSky_);

	switch (phase_) {
	case Phase::FadeIn:
		if (fade_) {
			fade_->Update();
			if (fade_->IsFinished()) {
				phase_ = Phase::Main;
			}
		}
		break;
	case Phase::Main:
		if (Input::GetInstance()->TriggerKey(DIK_A) || Input::GetInstance()->TriggerKey(DIK_D)) {
			menuSelection_ = 1 - menuSelection_;
			PlayUiCursorMoveSound();
		}
		if (Input::GetInstance()->TriggerKey(DIK_RETURN) || Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			PlayUiConfirmSound();
			if (menuSelection_ == 1) {
				PostQuitMessage(0);
				break;
			}
			StopTitleBgm();
			if (fade_) {
				fade_->Start(Fade::Status::kFadeOut, 1.0f);
			}
			phase_ = Phase::FadeOut;
		}
		break;
	case Phase::FadeOut:
		if (fade_) {
			fade_->Update();
			if (fade_->IsFinished()) {
				finished_ = true;
			}
		}
		break;
	}

	camera_.UpdateMatrix();
}

void TitleScene::DrawBackground() {
	constexpr float kGroundTileWidth = 128.0f;
	constexpr float kGroundTileHeight = 76.0f;
	int tileIndex = -1;
	for (GroundTile& tile : groundTiles_) {
		const float x = static_cast<float>(tileIndex++) * kGroundTileWidth -
			groundScrollPixels_ + kGroundTileWidth * 0.5f;
		if (modelGround_) {
			SetScreenTransform(tile.grass, x, groundTopPixels_ + kGroundTileHeight * 0.5f,
				kGroundTileWidth, kGroundTileHeight, 2.0f, 4.0f);
			modelGround_->Draw(tile.grass, camera_);
		}
		if (modelDirtBlock_) {
			SetScreenTransform(tile.dirt, x, groundTopPixels_ + kGroundTileHeight * 1.5f,
				kGroundTileWidth, kGroundTileHeight, 2.0f, 4.0f);
			modelDirtBlock_->Draw(tile.dirt, camera_);
		}
	}

	for (BackgroundProp& prop : backgroundProps_) {
		const float modelCenterY = prop.groundY - prop.height * 0.5f;
		if (prop.isGrass) {
			if (modelGrass_) {
				SetScreenTransform(prop.transform, prop.screenX, modelCenterY,
					prop.width, prop.height, prop.z, 0.35f);
				modelGrass_->Draw(prop.transform, camera_);
			}
		} else if (modelTree_) {
			SetScreenTransform(prop.transform, prop.screenX, modelCenterY,
				prop.width, prop.height, prop.z, 0.35f);
			modelTree_->Draw(prop.transform, camera_);
		}
	}
}
void TitleScene::DrawPlayer() {
	if (modelPlayerBody_) {
		modelPlayerBody_->Draw(worldTransformPlayer_, camera_);
	}
	if (modelPlayerLeftLeg_) {
		modelPlayerLeftLeg_->Draw(worldTransformLeftLeg_, camera_);
	}
	if (modelPlayerRightLeg_) {
		modelPlayerRightLeg_->Draw(worldTransformRightLeg_, camera_);
	}
	if (modelPlayerHammer_) {
		modelPlayerHammer_->Draw(worldTransformHammer_, camera_);
	}
}

void TitleScene::Draw() {
	Model::PreDraw();
	if (modelSkyDome_) {
		modelSkyDome_->Draw(worldTransformSky_, camera_);
	}
	DrawBackground();
	for (PreviewEnemy& enemy : previewEnemies_) {
		if (modelEnemy_) {
			modelEnemy_->Draw(enemy.transform, camera_);
		}
	}
	DrawPlayer();
	const float screenWidth = static_cast<float>(DirectXCommon::GetInstance()->GetBackBufferWidth());
	const float screenHeight = static_cast<float>(DirectXCommon::GetInstance()->GetBackBufferHeight());
	const float logoScale = (std::min)(screenWidth / 1280.0f, screenHeight / 720.0f);
	if (modelTitleLogo_) {
		SetScreenTransform(titleLogoTransform_, screenWidth * 0.47f, 138.0f * logoScale, 690.0f * logoScale, 690.0f * logoScale, -8.0f, 0.8f);
		titleLogoTransform_.rotation_.y = -0.08f;
		UpdateWorldTransform(titleLogoTransform_);
		modelTitleLogo_->Draw(titleLogoTransform_, camera_);
	}
	if (modelTitleBossBody_) {
		SetScreenTransform(titleBossBodyTransform_, screenWidth * 0.47f + 408.0f * logoScale, 138.0f * logoScale,
			58.0f * logoScale, 52.0f * logoScale, -7.0f, 1.0f);
		titleBossBodyTransform_.scale_.z = titleBossBodyTransform_.scale_.x;
		titleBossBodyTransform_.rotation_.y = -std::numbers::pi_v<float> / 2.0f;
		UpdateWorldTransform(titleBossBodyTransform_);
		modelTitleBossBody_->Draw(titleBossBodyTransform_, camera_);

		auto drawTitleBossWing = [&](Model* wingModel, WorldTransform& wingTransform, const Vector3& localOffset, float direction) {
			if (!wingModel) { return; }
			wingTransform.scale_ = titleBossBodyTransform_.scale_;
			wingTransform.scale_.x *= 1.5f;
			if (direction < 0.0f) { wingTransform.scale_.x *= -1.0f; }
			const float yaw = titleBossBodyTransform_.rotation_.y;
			const float wingYaw = direction > 0.0f ? yaw + std::numbers::pi_v<float> : yaw;
			const float flap = -0.55f;
			const float cosYaw = std::cos(yaw);
			const float sinYaw = std::sin(yaw);
			const Vector3 offset = {
				localOffset.x * titleBossBodyTransform_.scale_.x * cosYaw + localOffset.z * titleBossBodyTransform_.scale_.z * sinYaw,
				localOffset.y * titleBossBodyTransform_.scale_.y,
				-localOffset.x * titleBossBodyTransform_.scale_.x * sinYaw + localOffset.z * titleBossBodyTransform_.scale_.z * cosYaw,
			};
			wingTransform.rotation_ = {0.0f, wingYaw, flap};
			wingTransform.translation_ = {
				titleBossBodyTransform_.translation_.x + offset.x,
				titleBossBodyTransform_.translation_.y + offset.y,
				titleBossBodyTransform_.translation_.z + offset.z,
			};
			const float cosWingYaw = std::cos(wingYaw);
			const float sinWingYaw = std::sin(wingYaw);
			const float cosFlap = std::cos(flap);
			const float sinFlap = std::sin(flap);
			Matrix4x4& world = wingTransform.matWorld_;
			world.m[0][0] = wingTransform.scale_.x * cosFlap * cosWingYaw;
			world.m[0][1] = wingTransform.scale_.x * sinFlap;
			world.m[0][2] = -wingTransform.scale_.x * cosFlap * sinWingYaw;
			world.m[0][3] = 0.0f;
			world.m[1][0] = -wingTransform.scale_.y * sinFlap * cosWingYaw;
			world.m[1][1] = wingTransform.scale_.y * cosFlap;
			world.m[1][2] = wingTransform.scale_.y * sinFlap * sinWingYaw;
			world.m[1][3] = 0.0f;
			world.m[2][0] = wingTransform.scale_.z * sinWingYaw;
			world.m[2][1] = 0.0f;
			world.m[2][2] = wingTransform.scale_.z * cosWingYaw;
			world.m[2][3] = 0.0f;
			world.m[3][0] = wingTransform.translation_.x;
			world.m[3][1] = wingTransform.translation_.y;
			world.m[3][2] = wingTransform.translation_.z;
			world.m[3][3] = 1.0f;
			wingTransform.TransferMatrix();
			wingModel->Draw(wingTransform, camera_);
		};
		drawTitleBossWing(modelTitleBossWingNx_, titleBossWingNxTransform_, {-0.24f, 0.08f, -0.18f}, -1.0f);
		drawTitleBossWing(modelTitleBossWingPx_, titleBossWingPxTransform_, {0.24f, 0.08f, -0.18f}, 1.0f);
	}
	Model::PostDraw();
	if (phase_ == Phase::Main) {
		Sprite* menuSprite = menuSelection_ == 0 ? menuStartSprite_ : menuExitSprite_;
		if (menuSprite) {
			menuSprite->SetPosition({screenWidth * 0.5f - 280.0f, screenHeight - 150.0f});
			Sprite::PreDraw();
			menuSprite->Draw();
			Sprite::PostDraw();
		}
	}

	if (phase_ == Phase::FadeIn || phase_ == Phase::FadeOut) {
		if (fade_) {
			fade_->Draw();
		}
	}
}

TitleScene::~TitleScene() {
	StopTitleBgm();
	delete fade_;
	delete modelSkyDome_;
	delete modelTree_;
	delete modelGrass_;
	delete modelGround_;
	delete modelDirtBlock_;
	delete modelPlayerBody_;
	delete modelPlayerLeftLeg_;
	delete modelPlayerRightLeg_;
	delete modelPlayerHammer_;
	delete modelEnemy_;
	delete modelTitleLogo_;
	delete modelTitleBossBody_;
	delete modelTitleBossWingNx_;
	delete modelTitleBossWingPx_;
	delete menuStartSprite_;
	delete menuExitSprite_;
}
