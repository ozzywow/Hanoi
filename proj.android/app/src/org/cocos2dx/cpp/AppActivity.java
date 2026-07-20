package org.cocos2dx.cpp;

import android.os.Bundle;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;

import org.cocos2dx.lib.Cocos2dxActivity;

public class AppActivity extends Cocos2dxActivity {

    private static AppActivity sInstance;
    private BillingManager mBillingManager;
    private AppReviewManager mReviewManager;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        sInstance = this;
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(
            WindowManager.LayoutParams.FLAG_FULLSCREEN,
            WindowManager.LayoutParams.FLAG_FULLSCREEN
        );
        getWindow().getDecorView().setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_LAYOUT_STABLE
            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_FULLSCREEN
            | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
        );

        super.onCreate(savedInstanceState);
        mBillingManager = new BillingManager(this);
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            getWindow().getDecorView().setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
            );
        }
    }

    // Called from C++ via JNI — 설치 링크를 클립보드에 복사
    public static void copyToClipboard(final String text) {
        if (sInstance == null) return;
        sInstance.runOnUiThread(() -> {
            android.content.ClipboardManager cm =
                (android.content.ClipboardManager) sInstance.getSystemService(
                    android.content.Context.CLIPBOARD_SERVICE);
            if (cm != null) {
                cm.setPrimaryClip(android.content.ClipData.newPlainText("install_link", text));
            }
        });
    }

    // Called from C++ via JNI
    public static void purchaseProduct(final String productId) {
        if (sInstance == null) return;
        sInstance.runOnUiThread(() -> {
            if (sInstance.mBillingManager != null) {
                sInstance.mBillingManager.purchaseProduct(productId);
            }
        });
    }

    // Called from C++ via JNI
    public static void restorePurchases() {
        if (sInstance == null) return;
        sInstance.runOnUiThread(() -> {
            if (sInstance.mBillingManager != null) {
                sInstance.mBillingManager.restorePurchases();
            }
        });
    }

    // Called from C++ via JNI (ReviewManager::requestNative)
    public static void requestReview() {
        if (sInstance == null) return;
        sInstance.runOnUiThread(() -> {
            if (sInstance.mReviewManager == null) {
                sInstance.mReviewManager = new AppReviewManager(sInstance);
            }
            sInstance.mReviewManager.requestReview();
        });
    }

    // Callbacks to C++ (called from BillingManager)
    public static native void nativeOnPurchaseSuccess(String productId);
    public static native void nativeOnTransactionCanceled();
    public static native void nativeOnRestoreComplete(int count);
}
