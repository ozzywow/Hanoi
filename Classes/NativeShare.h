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

	// 문구 + 링크를 **별도 아이템**으로 넘기는 공유.
	// 하나의 문자열로 합쳐 보내면 iOS 공유 시트의 "복사"가 문구까지 클립보드에 올려
	// 주소창 붙여넣기가 깨진다. 링크를 NSURL 아이템으로 분리하면
	//   ① 메신저에는 문구 + 링크 미리보기가 함께 뜨고
	//   ② "복사"·Safari 등 URL 전용 액티비티는 순수 URL만 집어간다.
	// Android는 ACTION_SEND의 EXTRA_TEXT가 단일 문자열이라 "text\nurl"로 합쳐 보낸다.
	static void share(const std::string& text, const std::string& url);
};
