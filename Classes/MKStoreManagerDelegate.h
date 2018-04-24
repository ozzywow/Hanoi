#pragma once

#include "cocos2d.h"

USING_NS_CC;


class MKStoreManagerDelegate
{
public:
	MKStoreManagerDelegate();
	~MKStoreManagerDelegate();

	virtual void productFetchComplete() = 0;
	virtual void productPurchased(std::string productId) = 0;
	virtual void transactionCanceled() = 0;
	virtual void restorePreviousTransactions(int count) = 0;
};

