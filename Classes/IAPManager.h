#pragma once

#include <string>
#include "Singleton.h"
#include "platform/CCPlatformConfig.h"

#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
#include "AndroidBilling.h"
#endif

class IAPDelegate;

// ──────────────────────────────────────────────────────────────
// IAP 계층 (구 MKStoreManager 포팅 정리)
//   IAPManager   : 씬이 사용하는 크로스플랫폼 파사드 (싱글턴)   ← 구 CMKStoreManager
//   IAPPlatform  : 플랫폼별 결제 브리지 (내부용)               ← 구 iosLink_MKStoreManager
//   IAPIndicator : 로딩 인디케이터                            ← 구 iosUI
//   IAPDelegate  : 결제 콜백 인터페이스 (IAPDelegate.h)         ← 구 MKStoreManagerDelegate
// 비iOS(Win/Mac/Linux/Android)는 여기 인라인 스텁/JNI로 처리하고,
// iOS 구현은 IAPManager_ios.mm(StoreKit)에 있다.
// ──────────────────────────────────────────────────────────────

// 플랫폼 결제 브리지 — IAPManager 내부에서만 호출.
class IAPPlatform
{
public:
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_MAC) || (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX)
	static bool isFeaturePurchased(std::string featureId) { return false; }
	static void buyFeature(std::string featureId) {}
	static void setDelegate(IAPDelegate* delegate) {}
	static void restorePreviousTransactions() {}
#elif CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
	static bool isFeaturePurchased(std::string featureId) { return AndroidBilling_isFeaturePurchased(featureId); }
	static void buyFeature(std::string featureId) { AndroidBilling_buyFeature(featureId); }
	static void setDelegate(IAPDelegate* delegate) { AndroidBilling_setDelegate(delegate); }
	static void restorePreviousTransactions() { AndroidBilling_restorePreviousTransactions(); }
#else // iOS — 구현은 IAPManager_ios.mm
	static bool isFeaturePurchased(std::string featureId);
	static void buyFeature(std::string featureId);
	static void setDelegate(IAPDelegate* delegate);
	static void restorePreviousTransactions();
#endif
};

// 로딩 인디케이터 (iOS만 실제 UI, 그 외 no-op).
class IAPIndicator : public Singleton<IAPIndicator>
{
public:
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_MAC) || (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX) || (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	IAPIndicator() {}
	~IAPIndicator() {}
	void ToggleIndicator(bool lock) {}
#else // iOS — 구현은 IAPManager_ios.mm
	IAPIndicator();
	~IAPIndicator();
	void ToggleIndicator(bool lock);
#endif
};

// 씬이 사용하는 크로스플랫폼 IAP 파사드.
class IAPManager : public Singleton<IAPManager>
{
public:
	IAPManager();
	~IAPManager();

	// 결제 완료/취소/복원 콜백을 받을 델리게이트 등록 (해제는 nullptr).
	void SetDelegate(IAPDelegate* delegate);

	bool isFeaturePurchased(std::string featureId);
	void buyFeature(std::string featureId);
	void restorePreviousTransactions();

	void ToggleIndicator(bool lock);

	IAPDelegate* _delegate = nullptr;
};
