#include "Fade.h"
#include <algorithm>

using namespace KamataEngine;

namespace {
// Share one screen-covering sprite across scenes. The title fade creates it once;
// reusing it avoids a second Sprite::Create allocation during scene transition.
Sprite* sharedFadeSprite = nullptr;
}

Fade::~Fade() = default;

void Fade::Initialize() {
	if (!sharedFadeSprite) {
		const uint32_t whiteTexture = TextureManager::Load("./Resources/white1x1.png");
		sharedFadeSprite = Sprite::Create(whiteTexture, {0, 0}, {0, 0, 0, 1.0f}, {0, 0});
	}
	sprite_ = sharedFadeSprite;
	if (sprite_) {
		const float width = static_cast<float>(DirectXCommon::GetInstance()->GetBackBufferWidth());
		const float height = static_cast<float>(DirectXCommon::GetInstance()->GetBackBufferHeight());
		sprite_->SetSize({width, height});
	}
}

void Fade::Update() {
	if (status_ == Status::kNone) {
		return;
	}

	counter_ += 1.0f / 60.0f;
	if (counter_ >= duration_) {
		counter_ = duration_;
	}

	if (!sprite_) {
		return;
	}
	if (status_ == Status::kFadeIn) {
		sprite_->SetColor(Vector4(0, 0, 0, std::clamp(1.0f - (counter_ / duration_), 0.0f, 1.0f)));
	} else if (status_ == Status::kFadeOut) {
		sprite_->SetColor(Vector4(0, 0, 0, std::clamp(counter_ / duration_, 0.0f, 1.0f)));
	}
}

void Fade::Draw() {
	if (status_ == Status::kNone || !sprite_) {
		return;
	}

	Sprite::PreDraw();
	sprite_->Draw();
	Sprite::PostDraw();
}

void Fade::Start(Status status, float duration) {
	status_ = status;
	duration_ = duration;
	counter_ = 0.0f;

	if (!sprite_) {
		return;
	}
	if (status_ == Status::kFadeIn) {
		sprite_->SetColor(Vector4(0.0f, 0.0f, 0.0f, 1.0f));
	} else if (status_ == Status::kFadeOut) {
		sprite_->SetColor(Vector4(0.0f, 0.0f, 0.0f, 0.0f));
	}
}

void Fade::Stop() {
	status_ = Status::kNone;
}

bool Fade::IsFinished() const {
	switch (status_) {
	case Status::kFadeIn:
	case Status::kFadeOut:
		return counter_ >= duration_;
	}
	return true;
}