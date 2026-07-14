package org.cocos2dx.cpp;

import android.app.Activity;
import android.util.Log;

import com.google.android.gms.tasks.Task;
import com.google.android.play.core.review.ReviewInfo;
import com.google.android.play.core.review.ReviewManager;
import com.google.android.play.core.review.ReviewManagerFactory;

// Google Play In-App Review 래퍼.
// 게이팅(누적 플레이/버전당 1회)은 C++ ReviewManager가 이미 통과시킨 뒤 호출됨 →
// 여기서는 곧바로 리뷰 플로우를 요청한다. 실제 팝업 표출 여부는 Play가 쿼터로 최종 결정.
public class AppReviewManager {

    private static final String TAG = "AppReviewManager";

    private final Activity mActivity;
    private final ReviewManager mManager;

    public AppReviewManager(Activity activity) {
        mActivity = activity;
        mManager = ReviewManagerFactory.create(activity);
    }

    public void requestReview() {
        Task<ReviewInfo> request = mManager.requestReviewFlow();
        request.addOnCompleteListener(task -> {
            if (task.isSuccessful()) {
                ReviewInfo reviewInfo = task.getResult();
                Task<Void> flow = mManager.launchReviewFlow(mActivity, reviewInfo);
                flow.addOnCompleteListener(t ->
                    Log.d(TAG, "review flow finished"));   // 성공/취소 관계없이 완료
            } else {
                Log.w(TAG, "review flow request failed", task.getException());
            }
        });
    }
}
