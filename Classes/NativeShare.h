#pragma once

#include <string>

// 네이티브 공유 시트 (친구 초대용).
//   iOS     : UIActivityViewController   (NativeShare_apple.mm)
//   Android : Intent.ACTION_SEND         (AppActivity.shareText, JNI)
//   Mac/Win : 미지원 → isSupported()=false (호출측이 클립보드 복사로 폴백)
class NativeShare
{
public:
	// 네이티브 공유 시트를 띄울 수 있는 플랫폼인지 (iOS/Android true)
	static bool isSupported();
	// 공유 시트 표시. isSupported()가 false면 no-op.
	static void share(const std::string& text);
};
