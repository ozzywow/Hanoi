#include "cocos2d.h"
#include "platform/CCPlatformConfig.h"

#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID

#include "AndroidBilling.h"
#include "IAPDelegate.h"

#include <jni.h>
#include "platform/android/jni/JniHelper.h"

#define APPACTIVITY_CLASS "org/cocos2dx/cpp/AppActivity"

static IAPDelegate* s_delegate = nullptr;
static bool s_purchased = false;

void AndroidBilling_setDelegate(IAPDelegate* delegate)
{
    s_delegate = delegate;
}

void AndroidBilling_buyFeature(const std::string& featureId)
{
    cocos2d::JniMethodInfo t;
    if (cocos2d::JniHelper::getStaticMethodInfo(t, APPACTIVITY_CLASS, "purchaseProduct", "(Ljava/lang/String;)V"))
    {
        jstring jFeatureId = t.env->NewStringUTF(featureId.c_str());
        t.env->CallStaticVoidMethod(t.classID, t.methodID, jFeatureId);
        t.env->DeleteLocalRef(jFeatureId);
        t.env->DeleteLocalRef(t.classID);
    }
}

bool AndroidBilling_isFeaturePurchased(const std::string& featureId)
{
    return s_purchased;
}

void AndroidBilling_restorePreviousTransactions()
{
    cocos2d::JniMethodInfo t;
    if (cocos2d::JniHelper::getStaticMethodInfo(t, APPACTIVITY_CLASS, "restorePurchases", "()V"))
    {
        t.env->CallStaticVoidMethod(t.classID, t.methodID);
        t.env->DeleteLocalRef(t.classID);
    }
}

// ---- JNI callbacks from Java ----

extern "C" {

JNIEXPORT void JNICALL
Java_org_cocos2dx_cpp_AppActivity_nativeOnPurchaseSuccess(JNIEnv* env, jclass cls, jstring productId)
{
    const char* id = env->GetStringUTFChars(productId, nullptr);
    std::string pid(id);
    env->ReleaseStringUTFChars(productId, id);

    s_purchased = true;

    cocos2d::Director::getInstance()->getScheduler()->performFunctionInCocosThread([pid]() {
        if (s_delegate) {
            s_delegate->productPurchased(pid);
        }
    });
}

JNIEXPORT void JNICALL
Java_org_cocos2dx_cpp_AppActivity_nativeOnTransactionCanceled(JNIEnv* env, jclass cls)
{
    cocos2d::Director::getInstance()->getScheduler()->performFunctionInCocosThread([]() {
        if (s_delegate) {
            s_delegate->transactionCanceled();
        }
    });
}

JNIEXPORT void JNICALL
Java_org_cocos2dx_cpp_AppActivity_nativeOnRestoreComplete(JNIEnv* env, jclass cls, jint count)
{
    int restoredCount = (int)count;
    cocos2d::Director::getInstance()->getScheduler()->performFunctionInCocosThread([restoredCount]() {
        if (s_delegate) {
            s_delegate->restorePreviousTransactions(restoredCount);
        }
    });
}

} // extern "C"

#endif // CC_PLATFORM_ANDROID
