#pragma once
#include <cmath>

//  イージング関数を管理するヘッダーファイル
namespace Easing {

// 🏎️ EaseIn：動き出しがゆっくりで、後半に加速する
inline float EaseIn(float start, float end, float t) {
	if (t < 0.0f)
		t = 0.0f;
	if (t > 1.0f)
		t = 1.0f;

	float rate = t * t; // 2乗の計算
	return start + (end - start) * rate;
}

// EaseOut：最初は速く、後半に減速しながらピタッと止まる（学校のスライドで使われているタイプ）
inline float EaseOut(float start, float end, float t) {
	if (t < 0.0f)
		t = 0.0f;
	if (t > 1.0f)
		t = 1.0f;

	float rate = 1.0f - (1.0f - t) * (1.0f - t); // 反転させた2乗の計算
	return start + (end - start) * rate;
}

} // namespace Easing