package org.cocos2dx.cpp;

import android.os.Bundle;
import android.view.Window;

import androidx.core.splashscreen.SplashScreen;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.WindowInsetsControllerCompat;

import org.cocos2dx.lib.Cocos2dxActivity;

public class AppActivity extends Cocos2dxActivity {

    private static AppActivity sInstance;
    private BillingManager mBillingManager;
    private AppReviewManager mReviewManager;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // 시스템 스플래시 설치 (super.onCreate 이전 필수) — Theme.App.Splash → postSplashScreenTheme 전환
        SplashScreen.installSplashScreen(this);

        sInstance = this;
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        applyImmersiveMode();

        super.onCreate(savedInstanceState);
        mBillingManager = new BillingManager(this);
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            applyImmersiveMode();
        }
    }

    // Android 16(API 36) 타겟은 엣지투엣지를 opt-out할 수 없다.
    // deprecated된 FLAG_FULLSCREEN / setSystemUiVisibility 대신 인셋 컨트롤러로 몰입형 전체화면 구성.
    private void applyImmersiveMode() {
        WindowCompat.setDecorFitsSystemWindows(getWindow(), false);
        WindowInsetsControllerCompat controller =
            WindowCompat.getInsetsController(getWindow(), getWindow().getDecorView());
        controller.setSystemBarsBehavior(
            WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
        controller.hide(WindowInsetsCompat.Type.systemBars());
    }

    // Called from C++ via JNI — 네이티브 공유 시트 (친구 초대)
    public static void shareText(final String text) {
        if (sInstance == null) return;
        sInstance.runOnUiThread(() -> {
            android.content.Intent i = new android.content.Intent(android.content.Intent.ACTION_SEND);
            i.setType("text/plain");
            i.putExtra(android.content.Intent.EXTRA_TEXT, text);
            sInstance.startActivity(android.content.Intent.createChooser(i, null));
        });
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
