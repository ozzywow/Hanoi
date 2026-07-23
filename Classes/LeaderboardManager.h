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
#ifdef ENABLE_REPLAY_LIKE
    int         likes = 0;    // 받은 좋아요 수 (없으면 0) — fetchLikes 조인
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

    // 랭커 이름 중복 소프트 필터: 캐시된 리더보드 엔트리 이름과 대소문자 무시(+trim) 비교, 본인 제외.
    // 캐시된 범위 한정 best-effort (PlayFab은 비유니크 허용 상태 — 명백한 랭커 중복만 차단).
    bool isNameTakenByRanker(const std::string& name) const;

    // 위 검사의 비동기판: 전 레벨 리더보드를 fresh 조회(캐시 채움)한 뒤 대조.
    // ResetAll이 캐시를 비운 직후 같은 콜드 캐시에서도 정상 동작. 캐시 워밍 상태면 네트워크 없이 즉시.
    void isNameTakenByRankerAsync(const std::string& name, std::function<void(bool taken)> callback);

    // 리더보드 조회 — submitScore 진행 중인 레벨은 완료 후 자동 서빙
    void fetchLeaderboard(int level, int maxCount,
                          std::function<void(const std::vector<LeaderboardEntry>&)> callback);

    // 해당 레벨 캐시를 즉시 무효화
    void invalidateCache(int level);

    // 랭킹보드 관련 전 캐시(리더보드/리플레이/수상소감)를 즉시 비운다. RESET ALL 시 호출.
    void clearAllCaches();

    bool isLoggedIn() const { return !m_sessionTicket.empty(); }
    const std::string& getPlayFabId() const { return m_playFabId; }

    // ── 공개 Title Data 설정(award_enabled + notice) 단일 조회 ──
    // 로그인 시 1회 + 랭킹보드/타이틀 진입 시 재조회. 결과는 멤버에 캐시.
    // fail-open/safe: 조회 실패 시 기존값 유지.
    void fetchTitleConfig(std::function<void()> callback = nullptr);
    // 수상소감 마스터 스위치: "0"/"false"/"off" → 비활성, 그 외/키없음/실패 → 활성.
    bool isAwardEnabled() const { return m_awardEnabled; }
    // 좋아요 마스터 스위치(replay_like): award_enabled와 동일 정책. OFF면 클라 UI(버튼/카운트) 숨김.
    bool isLikeEnabled() const { return m_likeEnabled; }
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

    // ── 리플레이 공유 (2차) — Shared Group replays_L%02d, award_comment 인프라 미러 ──
    // 업로드 조건: 레벨 ≤ REPLAY_MAX_LEVEL AND Top10 (게이트는 호출측 MainScene에서 판정).
    static constexpr int REPLAY_MAX_LEVEL = 10;   // 전 레벨(3~10) 저장
    static std::string replayGroupId(int level);   // "replays_L%02d"

    // 로그인 성공 시 replays_L03~L05 Shared Group 부트스트랩.
    void bootstrapReplayGroups(bool force = false);

    // 로컬 최고기록 리플레이 blob을 PlayFab에 업로드 (CloudScript writeReplay). 콜백(성공).
    void uploadReplay(int level, const std::string& blob,
                      std::function<void(bool)> callback = nullptr);

    // 레벨별 리플레이 조회 → (playFabId -> Base64 blob). CACHE_TTL_HOURS 캐시.
    void fetchReplays(int level,
                      std::function<void(const std::map<std::string, std::string>&)> callback);
    void invalidateReplays(int level);

    // ── 도전모드 격파/복수 (3차, battle_reward) — Shared Group battles_L%02d ──
    // 고스트 대결에서 상향 추월 시 격파 낙인. 표시명·깃발은 렌더 시 id 맵으로 해소(미저장).
    static constexpr int BATTLE_MAX_LEVEL = 10;    // 서버 cloudscript BATTLE_MAX_LEVEL과 일치 필수
    static std::string battleGroupId(int level);   // "battles_L%02d"

    // 로그인 성공 시 battles_L03~L10 Shared Group 부트스트랩 (award/replay 미러).
    void bootstrapBattleGroups(bool force = false);

    // 격파 기록 — 신기록 제출 직후 호출. loserId=고스트(B) id, aValWas=A의 직전 기록(ms, 상향 판정용).
    // 콜백: (성공, reason). 성립 실패 시 reason="not_passed"/"not_upward"/"self"/"disabled" 등.
    void recordBattle(int level, const std::string& loserId, int aValWas,
                      std::function<void(bool, const std::string&)> callback = nullptr);

    // 내 격파 낙인 삭제 (복수 성공/이름·랭킹 초기화 시). 서버는 key=본인 id만 삭제.
    void deleteBattle(int level, std::function<void(bool)> callback = nullptr);

    // 레벨별 격파 낙인 조회 → (패자 loserId -> 격파자 winnerId). CACHE_TTL_HOURS 캐시.
    // 표시명·깃발은 호출측이 리더보드 id 맵으로 해소(레코드엔 미저장).
    void fetchBattles(int level,
                      std::function<void(const std::map<std::string, std::string>&)> callback);
    void invalidateBattles(int level);

#ifdef ENABLE_REPLAY_LIKE
    // ── 리플레이 좋아요 (👍) — Shared Group likes_L%02d, docs/replay_like_plan.md ──
    // 관전에서 좋아요 → 대상 누적 +1 → 랭킹보드 이름 우측 👍N 표시. battle/replay 인프라 미러.
    static constexpr int LIKE_MAX_LEVEL = 10;   // 서버 cloudscript LIKE_MAX_LEVEL과 일치 필수
    static std::string likeGroupId(int level);  // "likes_L%02d"

    // 로그인 성공 시 likes_L03~L10 Shared Group 부트스트랩 (battle/replay 미러).
    void bootstrapLikeGroups(bool force = false);

    // 좋아요 누르기 — 관전 대상(targetId) 리플레이에 +1. CloudScript likeReplay 경유.
    // 콜백: (ok, newCount, already). already=true면 이미 누른 상태(카운트 무변).
    void likeReplay(int level, const std::string& targetId,
                    std::function<void(bool ok, int newCount, bool already)> callback = nullptr);

    // 레벨별 좋아요 수 조회 → (playFabId -> count). CACHE_TTL_HOURS 캐시.
    void fetchLikes(int level,
                    std::function<void(const std::map<std::string, int>&)> callback);
    void invalidateLikes(int level);
#endif // ENABLE_REPLAY_LIKE

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
    bool        m_likeEnabled  = true;   // 좋아요 마스터 스위치(replay_like), fail-open
    std::string m_notice;                // 상단 티커 공지 (없으면 빈 문자열)
    std::string m_latestVersion;         // 최신 배포 버전 (공개 Title Data latest_version, 미설정=빈 문자열)

    struct CacheEntry {
        std::vector<LeaderboardEntry> entries;
        std::time_t cachedAt;
    };
    std::map<int, CacheEntry>              m_leaderboardCache;

    // 리플레이 캐시: level -> (playFabId -> Base64 blob)
    struct ReplayCacheEntry {
        std::map<std::string, std::string> byId;
        std::time_t cachedAt;
    };
    std::map<int, ReplayCacheEntry>        m_replayCache;
    bool                                   m_replayGroupsBootstrapped = false;
    void doUploadReplay(int level, const std::string& blob, bool allowRetry,
                        std::function<void(bool)> callback);

    // 격파 낙인 (3차, battle_reward)
    bool                                   m_battleGroupsBootstrapped = false;
    // attempt: 1부터. no_group→부트스트랩 후 재시도, not_passed/no_record→전파 지연으로 보고 재시도(최대 BATTLE_MAX_ATTEMPT).
    void doRecordBattle(int level, const std::string& loserId, int aValWas, int attempt,
                        std::function<void(bool, const std::string&)> callback);
    static constexpr int BATTLE_MAX_ATTEMPT = 4;   // 초기 1 + 전파 재시도 3
    // 격파 낙인 캐시: level -> (패자 loserId -> 격파자 winnerId)
    struct BattleCacheEntry {
        std::map<std::string, std::string> byLoser;
        std::time_t cachedAt;
    };
    std::map<int, BattleCacheEntry>        m_battleCache;

#ifdef ENABLE_REPLAY_LIKE
    // 좋아요 캐시: level -> (playFabId -> count)
    struct LikeCacheEntry {
        std::map<std::string, int> byId;
        std::time_t cachedAt;
    };
    std::map<int, LikeCacheEntry>          m_likeCache;
    bool                                   m_likeGroupsBootstrapped = false;
    // allowRetry: no_group 시 부트스트랩 후 1회 재시도(doUploadReplay 패턴).
    void doLikeReplay(int level, const std::string& targetId, bool allowRetry,
                      std::function<void(bool, int, bool)> callback);
#endif // ENABLE_REPLAY_LIKE

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
