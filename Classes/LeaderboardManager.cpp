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

void LeaderboardManager::submitScore(int level, int scoreMs, std::function<void(bool)> callback)
{
    // 점수가 더 나쁘면 제출 스킵 (pending 마킹 전에 처리)
    int best = UserDataManager::Instance()->GetBestRecord(level);
    if (best > 0 && scoreMs > best) {
        if (callback) callback(true);
        return;
    }

    // fetchLeaderboard 가 이 레벨로 오면 submit 완료 후까지 defer
    m_pendingSubmitLevels.insert(level);

    if (!isLoggedIn()) {
        login([this, level, scoreMs, callback](bool ok) {
            if (ok) {
                doSubmitScore(level, scoreMs, callback);
            } else {
                releasePendingSubmit(level, false);
                if (callback) callback(false);
            }
        });
        return;
    }

    doSubmitScore(level, scoreMs, callback);
}

void LeaderboardManager::doSubmitScore(int level, int scoreMs, std::function<void(bool)> callback)
{
    std::string body = StringUtils::format(
        "{\"Statistics\":[{\"StatisticName\":\"%s\",\"Value\":%d}]}",
        statName(level).c_str(), scoreMs
    );

    httpPost(BASE_URL + "/Client/UpdatePlayerStatistics", body, m_sessionTicket,
        [this, level, scoreMs, callback](bool ok, const std::string& resp) {
            log("PlayFab submitScore L%d %dms: %s | %s",
                level, scoreMs, ok ? "OK" : "FAIL", resp.c_str());
            releasePendingSubmit(level, ok);
            if (callback) callback(ok);
        });
}

void LeaderboardManager::releasePendingSubmit(int level, bool ok)
{
    m_pendingSubmitLevels.erase(level);
    auto deferred = std::move(m_deferredFetches[level]);
    m_deferredFetches.erase(level);

    if (ok) {
        m_leaderboardCache.erase(level);
        if (!deferred.empty()) {
            // deferred 콜백들을 한 번의 fresh fetch로 모두 서빙
            auto cbList = std::make_shared<std::vector<FetchCallback>>(std::move(deferred));
            fetchLeaderboard(level, 10, [cbList](const std::vector<LeaderboardEntry>& e) {
                for (auto& cb : *cbList) if (cb) cb(e);
            });
        } else {
            // PlayFab 전파 지연 보상 — 2초 후 백그라운드 캐시 워밍
            // (즉시 fetch 시 submit 전 데이터가 캐시에 저장돼 TTL 만료까지 구 데이터 서빙)
            Director::getInstance()->getScheduler()->schedule(
                [this, level](float) { fetchLeaderboard(level, 10, nullptr); },
                (void*)this, 0.0f, 0, 2.0f, false,
                "lbm_warm_" + std::to_string(level));
        }
    } else {
        // 제출 실패 시 deferred 콜백에 빈 결과 반환
        for (auto& cb : deferred) if (cb) cb({});
    }
}

void LeaderboardManager::updateDisplayName(const std::string& name, std::function<void(bool)> callback)
{
    if (name.empty()) {
        if (callback) callback(false);
        return;
    }
    if (!isLoggedIn()) {
        login([this, name, callback](bool ok) {
            if (ok) updateDisplayName(name, callback);
            else if (callback) callback(false);
        });
        return;
    }
    std::string body = StringUtils::format(
        "{\"DisplayName\":\"%s\"}", escapeJson(name).c_str()
    );
    httpPost(BASE_URL + "/Client/UpdateUserTitleDisplayName", body, m_sessionTicket,
        [name, callback](bool ok, const std::string&) {
            log("PlayFab updateDisplayName '%s': %s", name.c_str(), ok ? "OK" : "FAIL");
            if (callback) callback(ok);
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
    for (int level = 3; level <= MAX_PLAY_LEVEL; ++level) {
        if (level > 3) statsJson += ",";
        statsJson += StringUtils::format("{\"StatisticName\":\"%s\",\"Value\":0}",
            statName(level).c_str());
    }
    statsJson += "]";

    httpPost(BASE_URL + "/Client/UpdatePlayerStatistics",
        "{\"Statistics\":" + statsJson + "}", m_sessionTicket,
        [this](bool ok, const std::string&) {
            log("PlayFab resetStats: %s", ok ? "OK" : "FAIL");
            if (ok) m_leaderboardCache.clear();
        });
}

void LeaderboardManager::invalidateCache(int level)
{
    m_leaderboardCache.erase(level);
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

    // submitScore가 in-flight 중이면 완료 후 fresh 데이터로 자동 서빙
    if (m_pendingSubmitLevels.count(level)) {
        log("LeaderboardManager: submit pending L%d, deferring fetch", level);
        if (callback) m_deferredFetches[level].push_back(callback);
        return;
    }

    // 캐시 확인: CACHE_TTL_HOURS 이내면 캐시 반환
    auto it = m_leaderboardCache.find(level);
    if (it != m_leaderboardCache.end()) {
        double elapsedHours = difftime(time(nullptr), it->second.cachedAt) / 3600.0;
        if (elapsedHours < CACHE_TTL_HOURS) {
            log("LeaderboardManager: cache hit L%d (%.2fh old)", level, elapsedHours);
            if (callback) {
                // 캐시 히트도 항상 비동기 dispatch — initWithDiscusNum 내 순서 역전 방지
                auto entries = it->second.entries;
                // 캐시는 전체 항목을 보관 — 요청한 개수만큼 잘라 반환
                if ((int)entries.size() > maxCount)
                    entries.resize(maxCount);
                Director::getInstance()->getScheduler()->performFunctionInCocosThread(
                    [callback, entries]() { callback(entries); });
            }
            return;
        }
        log("LeaderboardManager: cache expired L%d (%.2fh old)", level, elapsedHours);
    }

    // PlayFab 리더보드는 내림차순(값이 큰 순)으로만 정렬되는데, 이 게임은 타임어택(ms가 작을수록 1위)이라
    // raw ms 저장 시 진짜 빠른 기록은 리더보드 맨 아래에 위치한다. StartPosition:0에서 maxCount만 받으면
    // non-zero 기록이 maxCount를 넘는 순간 빠른 기록(=실제 1위)이 조회 윈도우 밖으로 탈락한다.
    // → 넉넉히 받아온 뒤 오름차순 정렬 후 maxCount로 잘라 반환한다. (cloudscript가 TOP_N=10 유지하므로 20이면 충분)
    int fetchCount = std::max(maxCount, 20);

    std::string body = StringUtils::format(
        "{\"StatisticName\":\"%s\",\"MaxResultsCount\":%d,\"StartPosition\":0"
        ",\"ProfileConstraints\":{\"ShowLocations\":true,\"ShowDisplayName\":true}}",
        statName(level).c_str(), fetchCount
    );

    httpPost(BASE_URL + "/Client/GetLeaderboard", body, m_sessionTicket,
        [this, callback, level, maxCount](bool ok, const std::string& resp) {
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
                if (e.scoreMs == 0 || e.scoreMs >= 3599590) continue; // 0 또는 만료(cloudscript EXPIRED_VAL) 항목 스킵
                if (lb[i].HasMember("PlayFabId") && lb[i]["PlayFabId"].IsString())
                    e.playFabId = lb[i]["PlayFabId"].GetString();
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

                // PlayFab 전파 지연으로 자신의 displayName이 아직 미반영된 경우
                // 로컬 저장 이름으로 대체 (네임플레이트와 동일하게 표시)
                if (e.playFabId == m_playFabId &&
                    (e.displayName.empty() || e.displayName == "Player")) {
                    std::string localName = UserDataManager::Instance()->GetUserName();
                    if (!localName.empty())
                        e.displayName = localName;
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
            // 내 항목의 displayName이 "Player"이면 로컬 저장 이름으로 교체
            // (updateDisplayName과 fetchLeaderboard의 타이밍 차이로 간헐적으로 발생)
            std::string localName = UserDataManager::Instance()->GetUserName();
            if (!localName.empty()) {
                for (auto& e : entries) {
                    if (e.playFabId == m_playFabId && e.displayName == "Player")
                        e.displayName = localName;
                }
            }

            // 타임어택: 시간이 짧을수록 상위 랭크
            std::sort(entries.begin(), entries.end(), [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
                return a.scoreMs < b.scoreMs;
            });
            for (size_t i = 0; i < entries.size(); ++i)
                entries[i].rank = (int)i + 1;

            // 캐시는 전체 항목 저장 (truncate 전) — 이후 maxCount=1 호출이 캐시를 오염시키지 않도록
            m_leaderboardCache[level] = CacheEntry{ entries, time(nullptr) };
            log("LeaderboardManager: cached L%d (%d entries)", level, (int)entries.size());

            // 콜백에는 요청한 maxCount 만큼만 전달
            if ((int)entries.size() > maxCount)
                entries.resize(maxCount);

            if (callback) callback(entries);
        });
}
