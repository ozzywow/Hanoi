#ifndef _MKSTOREMANAGER_MM_H
#define _MKSTOREMANAGER_MM_H


//#include "MKStoreManager.h"
#include "Singleton.h"


class MKStoreManagerDelegate;
class iosLink_MKStoreManager
{
public:
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_MAC) || (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX)
	static bool isFeaturePurchased(std::string featureId) { return false; };
	static void buyFeature(std::string featureId) {};
	static void setDelegate(MKStoreManagerDelegate* delegate) {};
	static void restorePreviousTransactions(){};
#else
	static bool isFeaturePurchased(std::string featureId);
	static void buyFeature(std::string featureId);
	static void setDelegate(MKStoreManagerDelegate* delegate);
	static MKStoreManagerDelegate* getDelegate();
	static void restorePreviousTransactions();
#endif 
};


//class UIView;
//class UIActivityIndicatorView;
//class UIViewController;
class iosUI : public Singleton<iosUI>
{
public:
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_MAC) || (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX)
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
