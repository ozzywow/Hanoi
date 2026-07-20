#pragma once

#include "platform/CCPlatformConfig.h"

#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID

#include <string>

class IAPDelegate;

void AndroidBilling_setDelegate(IAPDelegate* delegate);
void AndroidBilling_buyFeature(const std::string& featureId);
bool AndroidBilling_isFeaturePurchased(const std::string& featureId);
void AndroidBilling_restorePreviousTransactions();

#endif // CC_PLATFORM_ANDROID
