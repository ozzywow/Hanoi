#pragma once

#include "cocos2d.h"

USING_NS_CC;

// IAP 결제 콜백 인터페이스 — 결제 흐름을 관찰하는 씬이 구현한다. (구: MKStoreManagerDelegate)
// 로직 없는 순수 인터페이스이므로 헤더 전용(ctor/dtor는 default).
class IAPDelegate
{
public:
	IAPDelegate() = default;
	virtual ~IAPDelegate() = default;

	virtual void productFetchComplete() = 0;
	virtual void productPurchased(std::string productId) = 0;
	virtual void transactionCanceled() = 0;
	virtual void restorePreviousTransactions(int count) = 0;
};
