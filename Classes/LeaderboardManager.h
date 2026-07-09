#pragma once

#include "cocos2d.h"
#include "common_define.h"   // ENABLE_AWARD_COMMENT 등 전역 매크로
#include "Singleton.h"
#include <string>
#include <vector>
#include <functional>
#include <map>
#include <set>
#include <ctime>

struct LeaderboardEntry {
    int         rank;
    std::string displayName;
    std::string countryCode;  // ISO 2자리 소문자 (e.g., "kr"), 없으면 빈 문자열
    int         scoreMs;
    std::string playFabId;
#ifdef ENABLE_AWARD_COMMENT
    std::string comment;      // 수상소감 (없으면 빈 문자열) — Phase 2에서 조인
#endif
};

class LeaderboardManager : public Singleton<LeaderboardManager>
{
public:
    LeaderboardManager();

    // 익명 로그인 (CustomID = 기기 고유ID, 자동 생성)
    void login(std::function<void(bool)> callback = nullptr);

    // 스테이지 완료 시 기록 제출 (로그인 없으면 자동 로그인 후 제출)
    void submitScore(int level, int scoreMs, std::function<void(bool)> callback = nullptr);

    void resetStats();
    void updateDisplayName(const std::string& name, std::function<void(bool)> callback = nullptr);

    // 플레이어 이름 욕설 필터 (소감 기능과 독립 — 항상 동작).
    // clientFilterName: 즉시 피드백용 경량 필터 (통과=true, 거부 시 reasonOut="empty"/"profanity").
    // validateName: 클라 필터 + 서버 banned_words(CloudScript) 최종 검증. 콜백(통과, reason).
    static bool clientFilterName(const std::string& name, std::string& reasonOut);
    void        validateName(const std::string& name,
                             std::function<void(bool, const std::string&)> callback);

    // 리더보드 조회 — submitScore 진행 중인 레벨은 완료 후 자동 서빙
    void fetchLeaderboard(int level, int maxCount,
                          std::function<void(const std::vector<LeaderboardEntry>&)> callback);

    // 해당 레벨 캐시를 즉시 무효화
    void invalidateCache(int level);

    bool isLoggedIn() const { return !m_sessionTicket.empty(); }
    const std::string& getPlayFabId() const { return m_playFabId; }

    // ── 공개 Title Data 설정(award_enabled + notice) 단일 조회 ──
    // 로그인 시 1회 + 랭킹보드/타이틀 진입 시 재조회. 결과는 멤버에 캐시.
    // fail-open/safe: 조회 실패 시 기존값 유지.
    void fetchTitleConfig(std::function<void()> callback = nullptr);
    // 수상소감 마스터 스위치: "0"/"false"/"off" → 비활성, 그 외/키없음/실패 → 활성.
    bool isAwardEnabled() const { return m_awardEnabled; }
    // 상단 티커 공지(공개 Title Data "notice"). 비면 기존 랭킹 스크롤로 폴백.
    const std::string& getNotice() const { return m_notice; }

    // ── 앱 버전 게이트 (공개 Title Data 단일 키 latest_version) ──
    // 2.2.0부터 iOS/Android 버전 체계 통합 → 플랫폼 구분 없는 단일 키.
    // 버전 규칙 a.b.c: a=대규모, b=중요, c=버그픽스.
    //   a 변경 → 강제 업데이트 / b 변경 → 권장 업데이트 / c 변경 → 알림 없음(무음).
    // 안전을 위해 UserDefault에 캐시하지 않음 → 서버 조회 성공 시에만 판정(오프라인 오차단 방지).
    // dotted numeric 비교. a<b → -1, a==b → 0, a>b → 1. 구분자/자릿수 차이에 견고.
    // maxComponents>0 이면 앞쪽 그 개수만 비교(1 → major만, 2 → major.minor).
    static int compareVersion(const std::string& a, const std::string& b, int maxComponents = 0);
    // 강제 업데이트: major(a)가 latest보다 뒤처짐. latest 미설정/로컬버전 불명 → false(fail-open).
    bool needsForceUpdate() const;
    // 권장 업데이트: a는 같고 minor(b)만 뒤처짐. c만 다르면 false.
    bool hasRecommendedUpdate() const;
    // 패치 업데이트: a·b는 같고 patch(c)만 뒤처짐. (소극적 안내 — 앱 최초 실행시에만 표시)
    bool hasPatchUpdate() const;
    const std::string& getLatestVersion() const { return m_latestVersion; }

    // 선택적(b/c) 업데이트 "다시 묻지 않기" — 현재 latest_version 기준으로 억제 저장/조회.
    // 저장값 == 현재 latest 일 때만 억제(새 버전이 나오면 저장값≠latest → 다시 안내). 강제(a)엔 미적용.
    bool isOptionalUpdateSuppressed() const;
    void suppressOptionalUpdate();

    static std::string statName(int level);

#ifdef ENABLE_AWARD_COMMENT
    // ── 랭킹 Top10 수상소감 (Award Comments) ──
    // Shared Group awards_L03~L10 부트스트랩. 로그인 성공 시 자동 호출.
    // force=true면 UserDefault 캐시 무시하고 재생성 시도 (no_group 복구용).
    void bootstrapAwardGroups(bool force = false);

    // 레벨별 수상소감 조회 → (playFabId -> 소감 텍스트). CACHE_TTL_HOURS 캐시.
    void fetchComments(int level,
                       std::function<void(const std::map<std::string, std::string>&)> callback);

    // 수상소감 작성/수정 — 클라 경량 필터 후 CloudScript writeAwardComment 경유.
    // 콜백: (성공, reason). 성공 시 reason="", 실패 시 "empty"/"too_long"/"link"/"profanity"/"network" 등.
    void writeAwardComment(int level, const std::string& text,
                           std::function<void(bool, const std::string&)> callback);

    void invalidateComments(int level);

    // 내 수상소감 전체 삭제 (이름/랭킹 초기화 시). L03~L10 deleteAwardComment 호출 + 캐시 클리어.
    void deleteAllAwardComments();

    // 클라 경량 필터 (즉시 피드백용, 서버가 최종 게이트). 통과=true, 거부 시 reasonOut 설정.
    static bool clientFilterComment(const std::string& text, std::string& reasonOut);
    // UTF-8 코드포인트 수 (iOS/Android 공용)
    static int  utf8Length(const std::string& s);
    // Shared Group ID: "awards_L%02d"
    static std::string awardGroupId(int level);

    static constexpr int AWARD_MAX_CP = 50;  // 수상소감 최대 코드포인트 (서버 cloudscript.js와 일치 필수)
#endif // ENABLE_AWARD_COMMENT

    // 캐시 TTL: 이 시간(시간 단위)이 지나면 서버 재질의
    static constexpr double CACHE_TTL_HOURS = 1.0;

private:
    using FetchCallback = std::function<void(const std::vector<LeaderboardEntry>&)>;

    std::string m_sessionTicket;
    std::string m_playFabId;

    // 공개 Title Data 캐시 (fetchTitleConfig). ENABLE_AWARD_COMMENT와 무관하게 항상 존재.
    bool        m_awardEnabled = true;   // fail-open 기본값 = 활성
    std::string m_notice;                // 상단 티커 공지 (없으면 빈 문자열)
    std::string m_latestVersion;         // 최신 배포 버전 (공개 Title Data latest_version, 미설정=빈 문자열)

    struct CacheEntry {
        std::vector<LeaderboardEntry> entries;
        std::time_t cachedAt;
    };
    std::map<int, CacheEntry>              m_leaderboardCache;

    // submitScore가 in-flight인 레벨 추적
    std::set<int>                          m_pendingSubmitLevels;
    // submitScore 완료 후 실행할 fetchLeaderboard 콜백
    std::map<int, std::vector<FetchCallback>> m_deferredFetches;

#ifdef ENABLE_AWARD_COMMENT
    // 수상소감 캐시: level -> (playFabId -> 소감)
    struct CommentCacheEntry {
        std::map<std::string, std::string> byId;
        std::time_t cachedAt;
    };
    std::map<int, CommentCacheEntry>       m_commentCache;
    bool                                   m_awardGroupsBootstrapped = false;

    // writeAwardComment 내부 (로그인 완료 후). allowRetry: no_group 시 부트스트랩 후 1회 재시도.
    void doWriteAwardComment(int level, const std::string& text, bool allowRetry,
                             std::function<void(bool, const std::string&)> callback);

    // 리더보드 엔트리에 수상소감을 playFabId로 조인 후 서빙 (fetchLeaderboard 최종 단계)
    void joinCommentsAndServe(int level, std::vector<LeaderboardEntry> entries,
                              std::function<void(const std::vector<LeaderboardEntry>&)> callback);
#endif // ENABLE_AWARD_COMMENT

    // submitScore 내부 구현 (로그인 완료 후 호출)
    void doSubmitScore(int level, int scoreMs, std::function<void(bool)> callback);
    // submit 완료 후 pending 해제 + deferred 콜백 처리
    void releasePendingSubmit(int level, bool ok);

    void httpPost(const std::string& url,
                  const std::string& jsonBody,
                  const std::string& authValue,
                  std::function<void(bool, const std::string&)> callback);

    static const std::string BASE_URL;
};
