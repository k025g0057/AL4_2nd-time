#pragma once

#include "KamataEngine.h"

// UI用サウンドは一度だけ読み込み、タイトル・ポーズ・クリア画面で共有する。
inline void PlayUiCursorMoveSound() {
	static const auto soundHandle = KamataEngine::Audio::GetInstance()->LoadWave("ui_sounds/cursor_move.wav");
	KamataEngine::Audio::GetInstance()->PlayWave(soundHandle);
}

inline void PlayUiConfirmSound() {
	static const auto soundHandle = KamataEngine::Audio::GetInstance()->LoadWave("ui_sounds/confirm.wav");
	KamataEngine::Audio::GetInstance()->PlayWave(soundHandle);
}
inline void PlayDodgeSound() {
	static const auto soundHandle = KamataEngine::Audio::GetInstance()->LoadWave("ui_sounds/kaihi.wav");
	KamataEngine::Audio::GetInstance()->PlayWave(soundHandle);
}
inline void PlayPlayerDamageSound() {
	static const auto soundHandle = KamataEngine::Audio::GetInstance()->LoadWave("ui_sounds/player_damage.wav");
	KamataEngine::Audio::GetInstance()->PlayWave(soundHandle);
}

inline void PlayBossRoarSound() {
	static const auto soundHandle = KamataEngine::Audio::GetInstance()->LoadWave("ui_sounds/boss_roar.wav");
	KamataEngine::Audio::GetInstance()->PlayWave(soundHandle);
}

inline void PlayResultSound() {
	KamataEngine::Audio* audio = KamataEngine::Audio::GetInstance();
	static const auto soundHandle = audio->LoadWave("ui_sounds/result_se.wav");
	audio->PlayWave(soundHandle);
}

inline void PlayBossSummonSound() {
	static const auto soundHandle = KamataEngine::Audio::GetInstance()->LoadWave("ui_sounds/boss_summon.wav");
	KamataEngine::Audio::GetInstance()->PlayWave(soundHandle, false, 0.35f);
}

inline void PlayBossWarningSound() {
	static const auto soundHandle = KamataEngine::Audio::GetInstance()->LoadWave("ui_sounds/boss_warning.wav");
	KamataEngine::Audio::GetInstance()->PlayWave(soundHandle);
}
