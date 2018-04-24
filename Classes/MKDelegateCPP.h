#ifndef MKDELEGATE_CPP_H
#define MKDELEGATE_CPP_H

#include "MKStoreManager.h"

#if __cplusplus
#include "MKStoreManagerDelegate.h"



@interface InterfaceMKStoreKitDelegate: NSObject<MKStoreKitDelegate>
{
    MKStoreManagerDelegate* cppDelegate;
}
-(void)setdeletegate:(MKStoreManagerDelegate*) d;
-(void)productFetchComplete;
-(void)productPurchased:(NSString *)productId;
-(void)transactionCanceled;
-(void)restorePreviousTransactions:(int)count;
@end

#endif //__cplusplus
#endif //MKDELEGATE_CPP_H
