#pragma once

#include "cocos2d.h"
#include "Singleton.h"
#include <string>
#include <vector>
#include <functional>
#include <map>
#include <ctime>

struct LeaderboardEntry {
    int         rank;
    std::string displayName;
    std::string countryCode;  // ISO 2자리 소문자 (e.g., "kr"), 없으면 빈 문자열
    int         scoreMs;
    std::string playFabId;
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

    // 리더보드 조회
    void fetchLeaderboard(int level, int maxCount,
                          std::function<void(const std::vector<LeaderboardEntry>&)> callback);

    bool isLoggedIn() const { return !m_sessionTicket.empty(); }
    const std::string& getPlayFabId() const { return m_playFabId; }

    static std::string statName(int level);

    // 캐시 TTL: 이 시간(시간 단위)이 지나면 서버 재질의
    static constexpr double CACHE_TTL_HOURS = 1.0;

private:
    std::string m_sessionTicket;
    std::string m_playFabId;

    struct CacheEntry {
        std::vector<LeaderboardEntry> entries;
        std::time_t cachedAt;
    };
    std::map<int, CacheEntry> m_leaderboardCache;

    void httpPost(const std::string& url,
                  const std::string& jsonBody,
                  const std::string& authValue,
                  std::function<void(bool, const std::string&)> callback);

    static const std::string BASE_URL;
};
