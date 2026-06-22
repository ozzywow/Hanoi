package org.cocos2dx.cpp;

import android.app.Activity;
import android.util.Log;

import androidx.annotation.NonNull;

import com.android.billingclient.api.AcknowledgePurchaseParams;
import com.android.billingclient.api.BillingClient;
import com.android.billingclient.api.BillingClientStateListener;
import com.android.billingclient.api.BillingFlowParams;
import com.android.billingclient.api.BillingResult;
import com.android.billingclient.api.PendingPurchasesParams;
import com.android.billingclient.api.ProductDetails;
import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.PurchasesUpdatedListener;
import com.android.billingclient.api.QueryProductDetailsParams;
import com.android.billingclient.api.QueryPurchasesParams;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class BillingManager implements PurchasesUpdatedListener {

    private static final String TAG = "BillingManager";

    private static final String[] PRODUCT_IDS = {
        "com.ozzywow.nanoi.fullversion"
    };

    private final Activity mActivity;
    private BillingClient mBillingClient;
    private List<ProductDetails> mProductDetailsList = new ArrayList<>();

    public BillingManager(Activity activity) {
        mActivity = activity;
        mBillingClient = BillingClient.newBuilder(activity)
                .setListener(this)
                .enablePendingPurchases(
                    PendingPurchasesParams.newBuilder()
                        .enableOneTimeProducts()
                        .build()
                )
                .build();
        connect();
    }

    private void connect() {
        mBillingClient.startConnection(new BillingClientStateListener() {
            @Override
            public void onBillingSetupFinished(@NonNull BillingResult billingResult) {
                if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK) {
                    Log.d(TAG, "Billing connected");
                    queryProductDetails();
                    queryExistingPurchases(false);
                } else {
                    Log.e(TAG, "Billing setup failed: " + billingResult.getDebugMessage());
                }
            }

            @Override
            public void onBillingServiceDisconnected() {
                Log.w(TAG, "Billing disconnected — reconnecting");
                connect();
            }
        });
    }

    private void queryProductDetails() {
        List<QueryProductDetailsParams.Product> products = new ArrayList<>();
        for (String id : PRODUCT_IDS) {
            products.add(QueryProductDetailsParams.Product.newBuilder()
                    .setProductId(id)
                    .setProductType(BillingClient.ProductType.INAPP)
                    .build());
        }
        QueryProductDetailsParams params = QueryProductDetailsParams.newBuilder()
                .setProductList(products)
                .build();

        mBillingClient.queryProductDetailsAsync(params, (billingResult, queryResult) -> {
            if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK) {
                mProductDetailsList = queryResult.getProductDetailsList();
                Log.d(TAG, "queryProductDetails OK, count=" + mProductDetailsList.size());
            } else {
                Log.e(TAG, "queryProductDetails failed: " + billingResult.getDebugMessage());
            }
        });
    }

    public void purchaseProduct(String productId) {
        ProductDetails details = null;
        for (ProductDetails pd : mProductDetailsList) {
            if (pd.getProductId().equals(productId)) {
                details = pd;
                break;
            }
        }

        if (details == null) {
            Log.e(TAG, "Product not found: " + productId + " — querying details then canceling");
            queryProductDetails();
            AppActivity.nativeOnTransactionCanceled();
            return;
        }

        List<BillingFlowParams.ProductDetailsParams> productDetailsParamsList = new ArrayList<>();
        productDetailsParamsList.add(
                BillingFlowParams.ProductDetailsParams.newBuilder()
                        .setProductDetails(details)
                        .build()
        );

        BillingFlowParams flowParams = BillingFlowParams.newBuilder()
                .setProductDetailsParamsList(productDetailsParamsList)
                .build();

        BillingResult result = mBillingClient.launchBillingFlow(mActivity, flowParams);
        if (result.getResponseCode() != BillingClient.BillingResponseCode.OK) {
            Log.e(TAG, "launchBillingFlow failed: " + result.getDebugMessage());
            AppActivity.nativeOnTransactionCanceled();
        }
    }

    public void restorePurchases() {
        queryExistingPurchases(true);
    }

    private void queryExistingPurchases(final boolean notify) {
        mBillingClient.queryPurchasesAsync(
                QueryPurchasesParams.newBuilder()
                        .setProductType(BillingClient.ProductType.INAPP)
                        .build(),
                (billingResult, purchases) -> {
                    if (billingResult.getResponseCode() != BillingClient.BillingResponseCode.OK) {
                        Log.e(TAG, "queryPurchases failed: " + billingResult.getDebugMessage());
                        if (notify) AppActivity.nativeOnRestoreComplete(0);
                        return;
                    }
                    int count = 0;
                    for (Purchase purchase : purchases) {
                        if (purchase.getPurchaseState() == Purchase.PurchaseState.PURCHASED) {
                            acknowledgePurchase(purchase);
                            if (notify) {
                                for (String pid : purchase.getProducts()) {
                                    AppActivity.nativeOnPurchaseSuccess(pid);
                                }
                            }
                            count++;
                        }
                    }
                    if (notify) AppActivity.nativeOnRestoreComplete(count);
                }
        );
    }

    private void acknowledgePurchase(Purchase purchase) {
        if (purchase.isAcknowledged()) return;
        AcknowledgePurchaseParams params = AcknowledgePurchaseParams.newBuilder()
                .setPurchaseToken(purchase.getPurchaseToken())
                .build();
        mBillingClient.acknowledgePurchase(params, billingResult -> {
            if (billingResult.getResponseCode() != BillingClient.BillingResponseCode.OK) {
                Log.e(TAG, "acknowledgePurchase failed: " + billingResult.getDebugMessage());
            }
        });
    }

    @Override
    public void onPurchasesUpdated(@NonNull BillingResult billingResult, List<Purchase> purchases) {
        int code = billingResult.getResponseCode();
        if (code == BillingClient.BillingResponseCode.OK && purchases != null) {
            for (Purchase purchase : purchases) {
                if (purchase.getPurchaseState() == Purchase.PurchaseState.PURCHASED) {
                    acknowledgePurchase(purchase);
                    for (String pid : purchase.getProducts()) {
                        AppActivity.nativeOnPurchaseSuccess(pid);
                    }
                }
            }
        } else if (code == BillingClient.BillingResponseCode.USER_CANCELED) {
            Log.d(TAG, "Purchase canceled by user");
            AppActivity.nativeOnTransactionCanceled();
        } else if (code == BillingClient.BillingResponseCode.ITEM_ALREADY_OWNED) {
            Log.d(TAG, "Item already owned — restoring");
            queryExistingPurchases(true);
        } else {
            Log.e(TAG, "onPurchasesUpdated error: " + billingResult.getDebugMessage());
            AppActivity.nativeOnTransactionCanceled();
        }
    }
}
