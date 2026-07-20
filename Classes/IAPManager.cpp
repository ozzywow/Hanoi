#include "IAPManager.h"
#include "IAPDelegate.h"

IAPManager::IAPManager()
{
	_delegate = nullptr;
}

IAPManager::~IAPManager()
{
}

bool IAPManager::isFeaturePurchased(std::string featureId)
{
	return IAPPlatform::isFeaturePurchased(featureId);
}

void IAPManager::buyFeature(std::string featureId)
{
	IAPPlatform::buyFeature(featureId);
}

void IAPManager::SetDelegate(IAPDelegate* delegate)
{
	_delegate = delegate;
	IAPPlatform::setDelegate(delegate);
}

void IAPManager::restorePreviousTransactions()
{
	IAPPlatform::restorePreviousTransactions();
}

void IAPManager::ToggleIndicator(bool lock)
{
	IAPIndicator::Instance()->ToggleIndicator(lock);
}
