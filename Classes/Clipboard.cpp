#include "Clipboard.h"
#include "platform/CCPlatformConfig.h"

#if CC_TARGET_PLATFORM == CC_PLATFORM_WIN32
// ── Windows: Win32 클립보드 (UTF-8 → UTF-16) ──
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

void Clipboard::copy(const std::string& text)
{
	int wlen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
	if (wlen <= 0) return;
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (SIZE_T)wlen * sizeof(wchar_t));
	if (!hMem) return;
	wchar_t* dst = (wchar_t*)GlobalLock(hMem);
	MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, dst, wlen);
	GlobalUnlock(hMem);
	if (OpenClipboard(nullptr)) {
		EmptyClipboard();
		SetClipboardData(CF_UNICODETEXT, hMem);   // 소유권이 시스템으로 이전됨
		CloseClipboard();
	} else {
		GlobalFree(hMem);
	}
}

#elif CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
// ── Android: AppActivity.copyToClipboard(String) 호출 (JNI) ──
#include <jni.h>
#include "platform/android/jni/JniHelper.h"

void Clipboard::copy(const std::string& text)
{
	cocos2d::JniMethodInfo t;
	if (cocos2d::JniHelper::getStaticMethodInfo(
			t, "org/cocos2dx/cpp/AppActivity", "copyToClipboard", "(Ljava/lang/String;)V")) {
		jstring js = t.env->NewStringUTF(text.c_str());
		t.env->CallStaticVoidMethod(t.classID, t.methodID, js);
		t.env->DeleteLocalRef(js);
		t.env->DeleteLocalRef(t.classID);
	}
}

#elif (CC_TARGET_PLATFORM == CC_PLATFORM_IOS) || (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
// 정의는 Clipboard_apple.mm (Objective-C)

#else
// ── 기타(Linux 등): 미지원 ──
void Clipboard::copy(const std::string&) {}
#endif
