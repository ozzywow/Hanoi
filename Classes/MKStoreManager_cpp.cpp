#include "MKStoreManager_cpp.h"
#include "MKStoreManager_mm.h"


CMKStoreManager::CMKStoreManager()
{
	_mkdelegate = NULL;
}

CMKStoreManager::~CMKStoreManager()
{

}

bool CMKStoreManager::isFeaturePurchased(std::string featureId)
{
    return iosLink_MKStoreManager::isFeaturePurchased(featureId);
}

void CMKStoreManager::buyFeature(std::string featureId)
{
    iosLink_MKStoreManager::buyFeature(featureId);
}


void CMKStoreManager::SetDelegate(MKStoreManagerDelegate* delegate)
{
    _mkdelegate = delegate;
    iosLink_MKStoreManager::setDelegate(delegate);
}


void CMKStoreManager::ToggleIndicator(bool lock)
{
	iosUI::Instance()->ToggleIndicator(lock);
}


void CMKStoreManager::restorePreviousTransactions()
{
	iosLink_MKStoreManager::restorePreviousTransactions();
}