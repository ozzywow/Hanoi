// iOS 인앱 리뷰 브리지 — SKStoreReviewController.
// ReviewManager.cpp의 requestNative()에서 호출.
// ⚠️ 이 파일은 .xcodeproj에 수동 추가 필요 (CLAUDE.md 규칙).

#include "platform/CCPlatformConfig.h"

#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS

#import <StoreKit/StoreKit.h>
#import <UIKit/UIKit.h>

extern "C" void ReviewBridge_requestReview_ios()
{
    // iOS 14+ : 활성 window scene을 찾아 scene 기반 API 사용
    if (@available(iOS 14.0, *)) {
        for (UIScene* scene in [UIApplication sharedApplication].connectedScenes) {
            if ([scene isKindOfClass:[UIWindowScene class]] &&
                scene.activationState == UISceneActivationStateForegroundActive) {
                [SKStoreReviewController requestReviewInScene:(UIWindowScene*)scene];
                return;
            }
        }
    }
    // iOS 10.3 ~ 13 : 구형 API (iOS 14에서 deprecated지만 동작)
    if (@available(iOS 10.3, *)) {
        [SKStoreReviewController requestReview];
    }
}

#endif // CC_PLATFORM_IOS
