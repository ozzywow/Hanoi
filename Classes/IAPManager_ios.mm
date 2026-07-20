#include <string>
#include "IAPManager.h"
#include "IAPDelegate.h"
#include "MKStoreManager.h"
#include "MKDelegateCPP.h"

USING_NS_CC;

static IAPDelegate* _iapDelegate;
static InterfaceMKStoreKitDelegate* _interfaceDele;
static UIView*        _rootView;
static UIActivityIndicatorView* _activity;


UIViewController* getRootViewController();


// ── IAPPlatform (구 iosLink_MKStoreManager) — StoreKit 브리지 ──

bool IAPPlatform::isFeaturePurchased(std::string featureId)
{
    NSString* obejctiveString =  [NSString stringWithUTF8String:featureId.c_str()];
    BOOL bRes = (bool)[MKStoreManager isFeaturePurchased:obejctiveString];
    return (bool)bRes;
}

void IAPPlatform::buyFeature(std::string featureId)
{
    NSString* obejctiveString =  [NSString stringWithUTF8String:featureId.c_str()];
	[[MKStoreManager sharedManager] buyFeature:obejctiveString];
}

void IAPPlatform::restorePreviousTransactions()
{
	[[MKStoreManager sharedManager] restorePreviousTransactions];
}

void IAPPlatform::setDelegate(IAPDelegate* delegate)
{
	_interfaceDele = [InterfaceMKStoreKitDelegate alloc];
    [_interfaceDele setdeletegate:delegate];
	_iapDelegate = delegate;
    [MKStoreManager setDelegate:_interfaceDele];
}


// ── IAPIndicator (구 iosUI) — 로딩 인디케이터 ──

IAPIndicator::IAPIndicator()
{
	UIViewController* uiView = [UIApplication sharedApplication].keyWindow.rootViewController;
	_rootView = uiView.view;
}

IAPIndicator::~IAPIndicator()
{

}

void IAPIndicator::ToggleIndicator(bool lock)
{
	if (nil == _activity)
	{
		_activity
			= [[UIActivityIndicatorView alloc] initWithFrame:CGRectMake(480 / 2, 320 / 2, 40.0, 40.0)];

		[_rootView addSubview : _activity];
	}

	if (false == lock)
		[_activity stopAnimating];
	else
		[_activity startAnimating];

	return;
}
