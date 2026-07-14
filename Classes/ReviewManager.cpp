#include "ReviewManager.h"
#include "cocos2d.h"

#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
#include "platform/android/jni/JniHelper.h"
#endif

USING_NS_CC;

namespace {
    const char* KEY_PLAY_COUNT   = "review_play_count";    // 누적 플레이 카운트
    const char* KEY_LAST_VERSION  = "review_last_version";  // 마지막으로 리뷰 요청한 앱 버전
    const int   REVIEW_MIN_PLAYS  = 3;                      // 최소 누적 플레이 횟수
}

// iOS 네이티브 브리지 (ReviewBridge_ios.mm에서 구현)
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
extern "C" void ReviewBridge_requestReview_ios();
#endif

ReviewManager* ReviewManager::Instance()
{
    static ReviewManager s_instance;
    return &s_instance;
}

void ReviewManager::NotifyGamePlayed()
{
    auto ud = UserDefault::getInstance();
    int count = ud->getIntegerForKey(KEY_PLAY_COUNT, 0);
    if (count < 1000000) {           // 오버플로 방지용 상한
        ud->setIntegerForKey(KEY_PLAY_COUNT, count + 1);
        ud->flush();
    }
}

bool ReviewManager::passesGate()
{
    auto ud = UserDefault::getInstance();

    // ① 누적 플레이 3회 이상
    if (ud->getIntegerForKey(KEY_PLAY_COUNT, 0) < REVIEW_MIN_PLAYS)
        return false;

    // ② 이 앱 버전에서 아직 요청한 적 없어야 함 (버전당 1회)
    std::string cur = Application::getInstance()->getVersion();
    if (cur.empty()) cur = "unknown";
    std::string last = ud->getStringForKey(KEY_LAST_VERSION, "");
    if (last == cur)
        return false;

    // 통과 → 이번 버전 요청 기록 (실제 표출 여부와 무관하게 재요청 방지)
    ud->setStringForKey(KEY_LAST_VERSION, cur);
    ud->flush();
    return true;
}

void ReviewManager::MaybeRequestReview()
{
    if (!passesGate()) return;
    requestNative();
}

void ReviewManager::requestNative()
{
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
    // Google Play In-App Review API (AppActivity.requestReview → AppReviewManager)
    JniHelper::callStaticVoidMethod("org/cocos2dx/cpp/AppActivity", "requestReview");
#elif CC_TARGET_PLATFORM == CC_PLATFORM_IOS
    ReviewBridge_requestReview_ios();
#else
    // Windows/Mac: 인앱 리뷰 API 없음 → no-op (필요 시 스토어 URL 열기로 확장 가능)
    log("[review] in-app review not supported on this platform");
#endif
}
