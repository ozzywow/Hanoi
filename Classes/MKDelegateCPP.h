#ifndef MKDELEGATE_CPP_H
#define MKDELEGATE_CPP_H

#include "MKStoreManager.h"

#if __cplusplus
#include "IAPDelegate.h"



@interface InterfaceMKStoreKitDelegate: NSObject<MKStoreKitDelegate>
{
    IAPDelegate* cppDelegate;
}
-(void)setdeletegate:(IAPDelegate*) d;
-(void)productFetchComplete;
-(void)productPurchased:(NSString *)productId;
-(void)transactionCanceled;
-(void)restorePreviousTransactions:(int)count;
@end

#endif //__cplusplus
#endif //MKDELEGATE_CPP_H
