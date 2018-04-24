#include <string>
#include "MKStoreManager_mm.h"
#include "MKStoreManager.h"
#include "MKStoreManagerDelegate.h"
#include "MKDelegateCPP.h"

USING_NS_CC;

static MKStoreManagerDelegate* _mkdelegate;
static InterfaceMKStoreKitDelegate* _interfaceDele;
static UIView*        _rootView;
static UIActivityIndicatorView* _activity;


UIViewController* getRootViewController();


bool iosLink_MKStoreManager::isFeaturePurchased(std::string featureId)
{
    NSString* obejctiveString =  [NSString stringWithUTF8String:featureId.c_str()];
    BOOL bRes = (bool)[MKStoreManager isFeaturePurchased:obejctiveString];
    return (bool)bRes;
}

void iosLink_MKStoreManager::buyFeature(std::string featureId)
{
    NSString* obejctiveString =  [NSString stringWithUTF8String:featureId.c_str()];
	[[MKStoreManager sharedManager] buyFeature:obejctiveString];
}

void iosLink_MKStoreManager::restorePreviousTransactions()
{
	[[MKStoreManager sharedManager] restorePreviousTransactions];
}

void iosLink_MKStoreManager::setDelegate(MKStoreManagerDelegate* delegate)
{
	_interfaceDele = [InterfaceMKStoreKitDelegate alloc];
    [_interfaceDele setdeletegate:delegate];
	_mkdelegate = delegate;    
    [MKStoreManager setDelegate:_interfaceDele];
}






iosUI::iosUI() 
{
	UIViewController* uiView = [UIApplication sharedApplication].keyWindow.rootViewController;
	_rootView = uiView.view;
}

iosUI::~iosUI() 
{

}



void iosUI::ToggleIndicator(bool lock)
{
	
	if (nil == _activity)
	{
		_activity
			= [[UIActivityIndicatorView alloc] initWithFrame:CGRectMake(320 / 2, 480 / 2, 40.0, 40.0)];

		[_rootView addSubview : _activity];
	}

	if (false == lock)
		[_activity stopAnimating];
	else
		[_activity startAnimating];

	return;
	
}


