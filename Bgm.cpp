#include "Bgm.h"
#include "KamataEngine.h"

namespace {
uint32_t titleBgmSoundHandle = 0;
bool titleBgmSoundLoaded = false;
uint32_t titleBgmVoiceHandle = 0;
bool titleBgmVoiceActive = false;
uint32_t gameBgmSoundHandle = 0;
bool gameBgmSoundLoaded = false;
uint32_t gameBgmVoiceHandle = 0;
bool gameBgmVoiceActive = false;
}

void StartTitleBgm() {
	if (titleBgmVoiceActive) {
		return;
	}
	KamataEngine::Audio* audio = KamataEngine::Audio::GetInstance();
	if (!titleBgmSoundLoaded) {
		titleBgmSoundHandle = audio->LoadWave("ui_sounds/title_bgm.wav");
		titleBgmSoundLoaded = true;
	}
	titleBgmVoiceHandle = audio->PlayWave(titleBgmSoundHandle, true, 0.55f);
	titleBgmVoiceActive = true;
}

void StopTitleBgm() {
	if (titleBgmVoiceActive) {
		KamataEngine::Audio::GetInstance()->StopWave(titleBgmVoiceHandle);
		titleBgmVoiceActive = false;
	}
}

void StartGameBgm() {
	if (gameBgmVoiceActive) {
		return;
	}
	KamataEngine::Audio* audio = KamataEngine::Audio::GetInstance();
	if (!gameBgmSoundLoaded) {
		gameBgmSoundHandle = audio->LoadWave("ui_sounds/game_bgm.wav");
		gameBgmSoundLoaded = true;
	}
	gameBgmVoiceHandle = audio->PlayWave(gameBgmSoundHandle, true, 0.55f);
	gameBgmVoiceActive = true;
}

void StopGameBgm() {
	if (gameBgmVoiceActive) {
		KamataEngine::Audio::GetInstance()->StopWave(gameBgmVoiceHandle);
		gameBgmVoiceActive = false;
	}
}
