#include "stdafx.h"
#include "LeaderboardManager.h"
#include "UserDataManager.h"
#include "GameConfig.h"
#include "common_define.h"
#include "network/HttpClient.h"
#include "json/document.h"

using namespace cocos2d;
using namespace cocos2d::network;

const std::string LeaderboardManager::BASE_URL = "https://" PLAYFAB_TITLE_ID ".playfabapi.com";

std::string LeaderboardManager::statName(int level)
{
    return StringUtils::format("BestTime_L%02d", level);
} 

static std::string getOrCreateDeviceId()
{
    const char* KEY = "playfab_custom_id";
    auto ud = UserDefault::getInstance();
    std::string id = ud->getStringForKey(KEY, "");
    if (id.empty()) {
        srand((unsigned int)time(nullptr));
        id = StringUtils::format("hanoi_%08x%08x", rand(), rand());
        ud->setStringForKey(KEY, id);
        ud->flush();
    }
    return id;
}

static std::string escapeJson(const std::string& s)
{
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '"')       { out += "\\\""; }
        else if (c == '\\') { out += "\\\\"; }
        else                { out += c; }
    }
    return out;
}

LeaderboardManager::LeaderboardManager() {}

void LeaderboardManager::httpPost(const std::string& url,
                                   const std::string& jsonBody,
                                   const std::string& authValue,
                                   std::function<void(bool, const std::string&)> callback)
{
    auto req = new HttpRequest();
    req->setUrl(url);
    req->setRequestType(HttpRequest::Type::POST);

    std::vector<std::string> headers = {
        "Content-Type: application/json",
        "Accept: application/json"
    };
    if (!authValue.empty())
        headers.push_back("X-Authorization: " + authValue);

    req->setHeaders(headers);
    req->setRequestData(jsonBody.c_str(), jsonBody.size());

    req->setResponseCallback([callback, url](HttpClient*, HttpResponse* response) {
        // This callback may fire on the HTTP worker thread.
        // Parse the response here, then dispatch to GL thread for all Cocos2d-x/UI work.
        bool success = false;
        std::string body;
        if (!response) {
            log("PlayFab HTTP error: url=%s no response", url.c_str());
        } else {
            long httpCode = response->getResponseCode();
            auto* data = response->getResponseData();
            if (data && !data->empty())
                body.assign(data->begin(), data->end());
            if (!response->isSucceed() || httpCode != 200) {
                const char* err = response->getErrorBuffer() ? response->getErrorBuffer() : "";
                log("PlayFab HTTP %ld: url=%s err=%s body=%.400s", httpCode, url.c_str(), err, body.c_str());
            } else {
                success = true;
            }
        }
        Director::getInstance()->getScheduler()->performFunctionInCocosThread(
            [callback, success, body]() {
                if (callback) callback(success, body);
            });
    });

    HttpClient::getInstance()->sendImmediate(req);
    req->release();
}

void LeaderboardManager::login(std::function<void(bool)> callback)
{
    if (isLoggedIn()) {
        if (callback) callback(true);
        return;
    }

    std::string customId = getOrCreateDeviceId();
    std::string displayName = UserDataManager::Instance()->GetUserName();

    std::string body = StringUtils::format(
        "{\"TitleId\":\"" PLAYFAB_TITLE_ID "\",\"CustomId\":\"%s\",\"CreateAccount\":true}",
        customId.c_str()
    );

    httpPost(BASE_URL + "/Client/LoginWithCustomID", body, "",
        [this, displayName, callback](bool ok, const std::string& resp) {
            if (!ok) {
                log("PlayFab login failed");
                if (callback) callback(false);
                return;
            }

            rapidjson::Document doc;
            doc.Parse(resp.c_str());
            if (doc.HasParseError() || !doc.HasMember("data")) {
                if (callback) callback(false);
                return;
            }

            const auto& data = doc["data"];
            if (data.HasMember("SessionTicket"))
                m_sessionTicket = data["SessionTicket"].GetString();
            if (data.HasMember("PlayFabId"))
                m_playFabId = data["PlayFabId"].GetString();

            log("PlayFab login OK: %s", m_playFabId.c_str());

            if (!displayName.empty()) {
                std::string nameBody = StringUtils::format(
                    "{\"DisplayName\":\"%s\"}", escapeJson(displayName).c_str()
                );
                httpPost(BASE_URL + "/Client/UpdateUserTitleDisplayName",
                         nameBody, m_sessionTicket, nullptr);
            }

            if (callback) callback(true);
        });
}

void LeaderboardManager::submitScore(int level, int scoreMs)
{
    if (!isLoggedIn()) {
        login([this, level, scoreMs](bool ok) {
            if (ok) submitScore(level, scoreMs);
        });
        return;
    }

    // Skip if scoreMs is worse than the stored best (best was already saved before this call)
    int best = UserDataManager::Instance()->GetBestRecord(level);
    if (best > 0 && scoreMs > best) return;

    std::string body = StringUtils::format(
        "{\"Statistics\":[{\"StatisticName\":\"%s\",\"Value\":%d}]}",
        statName(level).c_str(), scoreMs
    );

    httpPost(BASE_URL + "/Client/UpdatePlayerStatistics", body, m_sessionTicket,
        [level, scoreMs](bool ok, const std::string& resp) {
            log("PlayFab submitScore L%d %dms: %s | %s",
                level, scoreMs, ok ? "OK" : "FAIL", resp.c_str());
        });
}

void LeaderboardManager::updateDisplayName(const std::string& name)
{
    if (name.empty()) return;
    if (!isLoggedIn()) {
        login([this, name](bool ok) {
            if (ok) updateDisplayName(name);
        });
        return;
    }
    std::string body = StringUtils::format(
        "{\"DisplayName\":\"%s\"}", escapeJson(name).c_str()
    );
    httpPost(BASE_URL + "/Client/UpdateUserTitleDisplayName", body, m_sessionTicket,
        [name](bool ok, const std::string&) {
            log("PlayFab updateDisplayName '%s': %s", name.c_str(), ok ? "OK" : "FAIL");
        });
}

void LeaderboardManager::resetStats()
{
    if (!isLoggedIn()) {
        login([this](bool ok) {
            if (ok) resetStats();
        });
        return;
    }

    std::string statsJson = "[";
    for (int level = 3; level < MAX_PLAY_LEVEL; ++level) {
        if (level > 3) statsJson += ",";
        statsJson += StringUtils::format("{\"StatisticName\":\"%s\",\"Value\":0}",
            statName(level).c_str());
    }
    statsJson += "]";

    httpPost(BASE_URL + "/Client/UpdatePlayerStatistics",
        "{\"Statistics\":" + statsJson + "}", m_sessionTicket,
        [](bool ok, const std::string&) {
            log("PlayFab resetStats: %s", ok ? "OK" : "FAIL");
        });
}

void LeaderboardManager::fetchLeaderboard(int level, int maxCount,
    std::function<void(const std::vector<LeaderboardEntry>&)> callback)
{
    if (!isLoggedIn()) {
        login([this, level, maxCount, callback](bool ok) {
            if (ok) fetchLeaderboard(level, maxCount, callback);
            else if (callback) callback({});
        });
        return;
    }

    std::string body = StringUtils::format(
        "{\"StatisticName\":\"%s\",\"MaxResultsCount\":%d,\"StartPosition\":0"
        ",\"ProfileConstraints\":{\"ShowLocations\":true,\"ShowDisplayName\":true}}",
        statName(level).c_str(), maxCount
    );

    httpPost(BASE_URL + "/Client/GetLeaderboard", body, m_sessionTicket,
        [callback, level](bool ok, const std::string& resp) {
            log("PlayFab fetchLeaderboard L%d: %s | %s",
                level, ok ? "OK" : "FAIL", resp.substr(0, 300).c_str());
            std::vector<LeaderboardEntry> entries;
            if (!ok) {
                if (callback) callback(entries);
                return;
            }

            rapidjson::Document doc;
            doc.Parse(resp.c_str());
            if (doc.HasParseError() || !doc.HasMember("data")) {
                if (callback) callback(entries);
                return;
            }

            const auto& data = doc["data"];
            if (!data.HasMember("Leaderboard") || !data["Leaderboard"].IsArray()) {
                if (callback) callback(entries);
                return;
            }

            const auto& lb = data["Leaderboard"];
            for (rapidjson::SizeType i = 0; i < lb.Size(); i++) {
                LeaderboardEntry e;
                e.scoreMs = lb[i]["StatValue"].GetInt();
                if (e.scoreMs == 0) continue; // scoreMs == 0인 항목 스킵
                // top-level DisplayName 우선, 없으면 Profile.DisplayName
                if (lb[i].HasMember("DisplayName") && lb[i]["DisplayName"].IsString()) {
                    e.displayName = lb[i]["DisplayName"].GetString();
                } else if (lb[i].HasMember("Profile") && lb[i]["Profile"].IsObject()
                           && lb[i]["Profile"].HasMember("DisplayName")
                           && lb[i]["Profile"]["DisplayName"].IsString()) {
                    e.displayName = lb[i]["Profile"]["DisplayName"].GetString();
                } else {
                    e.displayName = "Player";
                }

                // Profile.Locations[0].CountryCode 추출 (IP 기반 PlayFab 자동 감지)
                if (lb[i].HasMember("Profile") && lb[i]["Profile"].IsObject()) {
                    const auto& profile = lb[i]["Profile"];
                    if (profile.HasMember("Locations") && profile["Locations"].IsArray()
                        && profile["Locations"].Size() > 0) {
                        const auto& loc = profile["Locations"][0];
                        if (loc.HasMember("CountryCode") && loc["CountryCode"].IsString()) {
                            std::string code = loc["CountryCode"].GetString();
                            for (char& c : code) c = (char)tolower((unsigned char)c);
                            e.countryCode = code;
                        }
                    }
                }

                entries.push_back(e);
            }
            // 타임어택: 시간이 짧을수록 상위 랭크
            std::sort(entries.begin(), entries.end(), [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
                return a.scoreMs < b.scoreMs;
            });
            for (size_t i = 0; i < entries.size(); ++i)
                entries[i].rank = (int)i + 1;
            if (callback) callback(entries);
        });
}
