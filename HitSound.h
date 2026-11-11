#pragma once

#include "KamataEngine.h"

// UI効果音と同じKamataEngineのWAV再生を使う。
inline void PlayHammerHitSound() {
	static const auto soundHandle = KamataEngine::Audio::GetInstance()->LoadWave("ui_sounds/hammer_hit.wav");
	KamataEngine::Audio::GetInstance()->PlayWave(soundHandle);
}
