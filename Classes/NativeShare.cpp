#include "NativeShare.h"
#include "platform/CCPlatformConfig.h"

#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
// ── Android: AppActivity.shareText(String) → Intent.ACTION_SEND (JNI) ──
#include <jni.h>
#include "platform/android/jni/JniHelper.h"

bool NativeShare::isSupported() { return true; }

void NativeShare::share(const std::string& text)
{
	cocos2d::JniMethodInfo t;
	if (cocos2d::JniHelper::getStaticMethodInfo(
			t, "org/cocos2dx/cpp/AppActivity", "shareText", "(Ljava/lang/String;)V")) {
		jstring js = t.env->NewStringUTF(text.c_str());
		t.env->CallStaticVoidMethod(t.classID, t.methodID, js);
		t.env->DeleteLocalRef(js);
		t.env->DeleteLocalRef(t.classID);
	}
}

#elif (CC_TARGET_PLATFORM == CC_PLATFORM_IOS) || (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
// 정의는 NativeShare_apple.mm

#else
// ── Windows/Linux 등: 공유 시트 없음 → 호출측이 클립보드 복사로 폴백 ──
bool NativeShare::isSupported() { return false; }
void NativeShare::share(const std::string&) {}
#endif
