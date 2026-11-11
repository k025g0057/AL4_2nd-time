#pragma once
#include "KamataEngine.h"

class Fade {
public:

	enum class Status {
		kNone,    
		kFadeIn, 
		kFadeOut, 
	};

	Fade() = default;
	~Fade();
	Fade(const Fade&) = delete;
	Fade& operator=(const Fade&) = delete;

	void Start(Status status, float duration);

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	void Stop();

	bool IsFinished() const;

private:
	KamataEngine::Sprite* sprite_ = nullptr;

	// 現在のフェード状態
	Status status_ = Status::kNone;

	float duration_ = 0.0f; // フェードの持続時間（秒）

	float counter_ = 0.0f; // フェードの経過時間（秒）

	
};