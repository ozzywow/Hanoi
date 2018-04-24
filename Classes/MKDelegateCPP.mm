#import "MKDelegateCPP.h"

@implementation InterfaceMKStoreKitDelegate
-(void)setdeletegate:(MKStoreManagerDelegate*) d
{
	self->cppDelegate = d;
}
-(void)productFetchComplete
{
	self->cppDelegate->productFetchComplete();
}

-(void)productPurchased:(NSString *)productId
{
    std::string cppString = [productId UTF8String];
	self->cppDelegate->productPurchased(cppString);
}

-(void)transactionCanceled
{
	self->cppDelegate->transactionCanceled();
}
-(void)restorePreviousTransactions:(int)count
{
	self->cppDelegate->restorePreviousTransactions(count);
}
@end
