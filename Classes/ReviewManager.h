#pragma once

#include <string>

// 인앱 리뷰(별점) 유도 관리자.
//  - 긍정적 순간(신기록)에만 요청
//  - 누적 플레이 REVIEW_MIN_PLAYS 회 이상
//  - 앱 버전당 1회만 요청 (요청한 버전을 UserDefault에 기록)
// 실제 팝업 표출은 OS가 최종 결정(iOS 연 3회, Android 쿼터) — 호출해도 안 뜰 수 있음(정상).
class ReviewManager
{
public:
    static ReviewManager* Instance();

    // 매 게임 클리어 시 호출 — 누적 플레이 카운트 증가.
    void NotifyGamePlayed();

    // 신기록 등 긍정적 순간 직후 호출 — 게이트 통과 시 네이티브 리뷰 요청.
    void MaybeRequestReview();

private:
    ReviewManager() = default;
    bool passesGate();       // 게이팅 조건 판정
    void requestNative();    // 플랫폼별 네이티브 리뷰 요청
};
