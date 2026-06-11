#ifndef _MKSTOREMANAGER_MM_H
#define _MKSTOREMANAGER_MM_H

#include "platform/CCPlatformConfig.h"
#include "Singleton.h"

#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
#include "AndroidBilling.h"
#endif

class MKStoreManagerDelegate;
class iosLink_MKStoreManager
{
public:
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_MAC) || (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX)
	static bool isFeaturePurchased(std::string featureId) { return false; };
	static void buyFeature(std::string featureId) {};
	static void setDelegate(MKStoreManagerDelegate* delegate) {};
	static void restorePreviousTransactions(){};
#elif CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
	static bool isFeaturePurchased(std::string featureId) { return AndroidBilling_isFeaturePurchased(featureId); };
	static void buyFeature(std::string featureId) { AndroidBilling_buyFeature(featureId); };
	static void setDelegate(MKStoreManagerDelegate* delegate) { AndroidBilling_setDelegate(delegate); };
	static void restorePreviousTransactions() { AndroidBilling_restorePreviousTransactions(); };
#else
	static bool isFeaturePurchased(std::string featureId);
	static void buyFeature(std::string featureId);
	static void setDelegate(MKStoreManagerDelegate* delegate);
	static MKStoreManagerDelegate* getDelegate();
	static void restorePreviousTransactions();
#endif
};


class iosUI : public Singleton<iosUI>
{
public:
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_MAC) || (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX) || (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	iosUI() {};
	~iosUI() {};
	void	ToggleIndicator(bool lock) {};
#else
	iosUI();
	~iosUI();
	void	ToggleIndicator(bool lock);
#endif
};


#endif //_MKSTOREMANAGER_MM_H
