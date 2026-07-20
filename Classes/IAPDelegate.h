#pragma once

#include "cocos2d.h"

// 헤더에서 전역 using namespace를 열면 이 헤더를 먼저 include하는 .mm(Objective-C++)에서
// Foundation의 Point/Rect(MacTypes.h)와 cocos2d의 Point/Rect가 충돌한다. 여기선 cocos 타입을
// 직접 쓰지 않으므로 USING_NS_CC를 두지 않는다.

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
