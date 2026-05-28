#pragma once

#include "cocos2d.h"
#include "Singleton.h"
#include <string>
#include <vector>
#include <functional>

struct LeaderboardEntry {
    int         rank;
    std::string displayName;
    std::string countryCode;  // ISO 2자리 소문자 (e.g., "kr"), 없으면 빈 문자열
    int         scoreMs;
};

class LeaderboardManager : public Singleton<LeaderboardManager>
{
public:
    LeaderboardManager();

    // ?듬챸 濡쒓렇??(CustomID = 湲곌린 怨좎쑀ID, ?먮룞 ?앹꽦)
    void login(std::function<void(bool)> callback = nullptr);

    // 寃뚯엫 ?꾨즺 ??湲곕줉 ?쒖텧 (媛쒖씤 理쒓퀬 湲곕줉???뚮쭔 ?꾩넚)
    void submitScore(int level, int scoreMs);

    void resetStats();
    void updateDisplayName(const std::string& name);

    // ??궧 議고쉶
    void fetchLeaderboard(int level, int maxCount,
                          std::function<void(const std::vector<LeaderboardEntry>&)> callback);

    bool isLoggedIn() const { return !m_sessionTicket.empty(); }

    static std::string statName(int level);

private:
    std::string m_sessionTicket;
    std::string m_playFabId;

    void httpPost(const std::string& url,
                  const std::string& jsonBody,
                  const std::string& authValue,
                  std::function<void(bool, const std::string&)> callback);

    static const std::string BASE_URL;
};
