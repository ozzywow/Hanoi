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

// 명백한 욕설/링크 즉시 차단용 경량 스캐너 (전체 목록은 서버 banned_words가 최종 검증).
// 소감/이름 공용. 입력은 소문자 사본을 기대(ASCII만 영향, 멀티바이트 한글은 그대로라 매칭 정상).
static std::string toLowerAscii(const std::string& s)
{
    std::string low = s;
    for (char& c : low) c = (char)tolower((unsigned char)c);
    return low;
}
static bool textHasProfanity(const std::string& low)
{
    static const char* bad[] = { "씨발", "시발", "개새끼",
                                 "병신", "fuck", "shit", "asshole", "bitch" };
    for (const char* w : bad)
        if (low.find(w) != std::string::npos) return true;
    return false;
}
static bool textHasLink(const std::string& low)
{
    static const char* needles[] = { "http", "www.", "://", ".com", ".net",
                                     ".gg", ".io", ".kr", "t.me", "kakao" };
    for (const char* n : needles)
        if (low.find(n) != std::string::npos) return true;
    return false;
}

// 플레이어 이름 경량 필터 (즉시 피드백용, 서버 validateName이 최종 게이트).
// 통과=true. 거부 시 reasonOut = "empty" / "profanity".
// ※ ENABLE_AWARD_COMMENT와 무관하게 항상 동작 (이름 필터는 소감 기능과 독립).
bool LeaderboardManager::clientFilterName(const std::string& name, std::string& reasonOut)
{
    size_t a = name.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) { reasonOut = "empty"; return false; }
    size_t b = name.find_last_not_of(" \t\r\n");
    std::string t = name.substr(a, b - a + 1);

    std::string low = toLowerAscii(t);
    if (textHasProfanity(low)) { reasonOut = "profanity"; return false; }

    reasonOut.clear();
    return true;
}

// 이름 검증 — 클라 경량 필터 후 CloudScript validateName(서버 banned_words) 경유.
// 콜백: (통과, reason). 네트워크/로그인 실패 시 클라 필터 통과분은 허용(fail-open).
void LeaderboardManager::validateName(const std::string& name,
    std::function<void(bool, const std::string&)> callback)
{
    std::string reason;
    if (!clientFilterName(name, reason)) {
        if (callback) callback(false, reason);
        return;
    }
    if (!isLoggedIn()) {
        login([this, name, callback](bool ok) {
            if (ok) validateName(name, callback);
            else if (callback) callback(true, "");   // 로그인 실패 → 클라 필터 통과분 허용
        });
        return;
    }

    std::string body = StringUtils::format(
        "{\"FunctionName\":\"validateName\","
        "\"FunctionParameter\":{\"name\":\"%s\"},"
        "\"GeneratePlayStreamEvent\":false}",
        escapeJson(name).c_str());

    httpPost(BASE_URL + "/Client/ExecuteCloudScript", body, m_sessionTicket,
        [callback](bool ok, const std::string& resp) {
            if (!ok) { if (callback) callback(true, ""); return; }   // 네트워크 실패 → 허용
            rapidjson::Document doc;
            doc.Parse(resp.c_str());
            if (doc.HasParseError() || !doc.HasMember("data")) {
                if (callback) callback(true, ""); return;
            }
            const auto& data = doc["data"];
            if (!data.HasMember("FunctionResult") || !data["FunctionResult"].IsObject()) {
                if (callback) callback(true, ""); return;            // 스크립트 오류 등 → 허용
            }
            const auto& fr = data["FunctionResult"];
            bool okFlag = fr.HasMember("ok") && fr["ok"].IsBool() && fr["ok"].GetBool();
            std::string reason = (fr.HasMember("reason") && fr["reason"].IsString())
                                 ? fr["reason"].GetString() : "";
            if (okFlag) { if (callback) callback(true, ""); }
            else        { if (callback) callback(false, reason.empty() ? "profanity" : reason); }
        });
}

LeaderboardManager::LeaderboardManager()
{
    // 직전 실행 시 저장한 Title Data 값을 즉시 로드(stale-while-revalidate) —
    // 네트워크 도착 전에도 마지막 공지/스위치를 바로 표시. 최초 실행만 기본값.
    m_awardEnabled = UserDefault::getInstance()->getBoolForKey("cfg_award_enabled", true);
    m_likeEnabled  = UserDefault::getInstance()->getBoolForKey("cfg_like_enabled", true);
    m_notice       = UserDefault::getInstance()->getStringForKey("cfg_notice", "");
}

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

    // 진행 중인 로그인이 있으면 콜백만 대기열에 추가하고 새 요청은 보내지 않는다
    // (로그인 스톰 방지 — 콜드 스타트에 다수 조회가 동시에 login()을 호출).
    if (callback) m_loginWaiters.push_back(callback);
    if (m_loginInFlight) return;
    m_loginInFlight = true;

    std::string customId = getOrCreateDeviceId();
    std::string displayName = UserDataManager::Instance()->GetUserName();

    std::string body = StringUtils::format(
        "{\"TitleId\":\"" PLAYFAB_TITLE_ID "\",\"CustomId\":\"%s\",\"CreateAccount\":true}",
        customId.c_str()
    );

    httpPost(BASE_URL + "/Client/LoginWithCustomID", body, "",
        [this, displayName](bool ok, const std::string& resp) {
            // 성공/실패 어느 경로로도 대기열을 반드시 일괄 소진 (누락 시 콜백 영구 대기).
            auto flushWaiters = [this](bool success) {
                m_loginInFlight = false;
                std::vector<std::function<void(bool)>> waiters;
                waiters.swap(m_loginWaiters);
                for (auto& cb : waiters) if (cb) cb(success);
            };

            if (!ok) {
                log("PlayFab login failed");
                flushWaiters(false);
                return;
            }

            rapidjson::Document doc;
            doc.Parse(resp.c_str());
            if (doc.HasParseError() || !doc.HasMember("data")) {
                flushWaiters(false);
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

#ifdef ENABLE_AWARD_COMMENT
            // 수상소감 Shared Group 최초 1회 생성 (기기당 1회, 이미 있으면 무시)
            bootstrapAwardGroups();
#endif
            // 리플레이 공유 Shared Group(replays_L03~L05) 최초 1회 생성 (2차)
            bootstrapReplayGroups();

            // 격파 낙인 Shared Group(battles_L03~L10) 최초 1회 생성 (3차, battle_reward)
            bootstrapBattleGroups();

#ifdef ENABLE_REPLAY_LIKE
            // 리플레이 좋아요 Shared Group(likes_L03~L10) 최초 1회 생성
            bootstrapLikeGroups();
#endif

            // 최근 접속 플레이어 Shared Group(recent_players) 최초 1회 생성
            bootstrapRecentGroup();

            // 공개 Title Data(마스터 스위치 + 공지) warm-up — 소감 기능 OFF여도 공지는 동작
            fetchTitleConfig();

            flushWaiters(true);
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

bool LeaderboardManager::isNameTakenByRanker(const std::string& name) const
{
    // trim + ASCII 소문자 정규화 (한글은 대소문자 없음 → 영향 없음)
    auto norm = [](const std::string& s) -> std::string {
        size_t a = s.find_first_not_of(" \t\r\n");
        if (a == std::string::npos) return std::string();
        size_t b = s.find_last_not_of(" \t\r\n");
        std::string t = s.substr(a, b - a + 1);
        for (char& c : t) c = (char)tolower((unsigned char)c);
        return t;
    };
    std::string target = norm(name);
    if (target.empty()) return false;

    // 캐시된 모든 레벨의 엔트리 이름과 대조 (본인 playFabId 제외)
    for (const auto& kv : m_leaderboardCache) {
        for (const auto& e : kv.second.entries) {
            if (!m_playFabId.empty() && e.playFabId == m_playFabId) continue;
            if (norm(e.displayName) == target) return true;
        }
    }
    return false;
}

void LeaderboardManager::isNameTakenByRankerAsync(const std::string& name,
    std::function<void(bool)> callback)
{
    // 전 레벨 리더보드를 조회(캐시 채움)한 뒤 동기 검사. fetchLeaderboard는 캐시 신선하면 즉시 반환.
    auto remaining = std::make_shared<int>(MAX_PLAY_LEVEL - 3 + 1);
    auto done      = std::make_shared<bool>(false);
    auto finish = [this, name, callback, done]() {
        if (*done) return;
        *done = true;
        if (callback) callback(isNameTakenByRanker(name));
    };
    for (int lv = 3; lv <= MAX_PLAY_LEVEL; ++lv) {
        fetchLeaderboard(lv, 10, [remaining, finish](const std::vector<LeaderboardEntry>&) {
            if (--(*remaining) <= 0) finish();
        });
    }
}

void LeaderboardManager::clearAllCaches()
{
    m_leaderboardCache.clear();
    m_replayCache.clear();
    m_battleCache.clear();
#ifdef ENABLE_REPLAY_LIKE
    m_likeCache.clear();
#endif
#ifdef ENABLE_AWARD_COMMENT
    m_commentCache.clear();
#endif
}

// 공개 Title Data 단일 조회 — award_enabled(마스터 스위치) + notice(상단 티커 공지).
// ENABLE_AWARD_COMMENT와 무관하게 항상 컴파일(공지는 소감 기능과 독립).
void LeaderboardManager::fetchTitleConfig(std::function<void()> callback)
{
    if (!isLoggedIn()) {
        login([this, callback](bool ok) {
            if (ok) fetchTitleConfig(callback);
            else if (callback) callback();  // 로그인 실패 — 기존값 유지(fail-open/safe)
        });
        return;
    }

    // 2.2.0부터 iOS/Android 버전 통합 → 플랫폼 구분 없는 단일 latest_version 키.
    std::string body = "{\"Keys\":[\"award_enabled\",\"notice\",\"latest_version\",\"like_enabled\"]}";
    httpPost(BASE_URL + "/Client/GetTitleData", body, m_sessionTicket,
        [this, callback](bool ok, const std::string& resp) {
            // 조회 실패 시 기존값 유지 (award_enabled=활성, notice는 직전값)
            if (ok) {
                rapidjson::Document doc;
                doc.Parse(resp.c_str());
                if (!doc.HasParseError() && doc.HasMember("data") &&
                    doc["data"].HasMember("Data") && doc["data"]["Data"].IsObject()) {
                    const auto& d = doc["data"]["Data"];
                    // award_enabled: "0"/"false"/"off" → 비활성, 그 외/키없음 → 활성
                    if (d.HasMember("award_enabled") && d["award_enabled"].IsString()) {
                        std::string v = d["award_enabled"].GetString();
                        m_awardEnabled = !(v == "0" || v == "false" || v == "off");
                    } else {
                        m_awardEnabled = true;
                    }
                    // like_enabled(replay_like): award_enabled와 동일 정책
                    if (d.HasMember("like_enabled") && d["like_enabled"].IsString()) {
                        std::string v = d["like_enabled"].GetString();
                        m_likeEnabled = !(v == "0" || v == "false" || v == "off");
                    } else {
                        m_likeEnabled = true;
                    }
                    // notice: 있으면 그대로, 없으면 빈 문자열(기존 랭킹 스크롤로 폴백)
                    if (d.HasMember("notice") && d["notice"].IsString())
                        m_notice = d["notice"].GetString();
                    else
                        m_notice.clear();
                    // 버전 게이트 (단일 latest_version). 없으면 빈 문자열 → 게이트 비활성.
                    // 캐시하지 않음(멤버만) — 오프라인 오차단 방지, 서버 조회 성공 시에만 판정.
                    m_latestVersion = (d.HasMember("latest_version") && d["latest_version"].IsString())
                        ? d["latest_version"].GetString() : "";
                    log("LeaderboardManager: award_enabled=%s notice=%s latest=%s",
                        m_awardEnabled ? "ON" : "OFF",
                        m_notice.empty() ? "(none)" : m_notice.c_str(),
                        m_latestVersion.empty() ? "(none)" : m_latestVersion.c_str());
                    // 다음 실행에서 즉시 쓰도록 캐시 저장(stale-while-revalidate)
                    UserDefault::getInstance()->setBoolForKey("cfg_award_enabled", m_awardEnabled);
                    UserDefault::getInstance()->setBoolForKey("cfg_like_enabled", m_likeEnabled);
                    UserDefault::getInstance()->setStringForKey("cfg_notice", m_notice);
                    UserDefault::getInstance()->flush();
                }
            }
            if (callback) callback();  // httpPost 콜백은 이미 cocos 스레드
        });
}

// dotted numeric 버전 비교. 숫자 그룹을 순서대로 비교하고, 짧은 쪽의 부족한 자리는 0으로 간주.
// 구분자 종류('.', '-' 등)나 자릿수 차이에 견고. "2.1.10" > "2.1.3" 정상 판정.
// maxComponents>0 이면 앞쪽 그 개수의 그룹만 비교(예: 2 → major.minor만, patch 무시).
int LeaderboardManager::compareVersion(const std::string& a, const std::string& b, int maxComponents)
{
    size_t ia = 0, ib = 0;
    int comp = 0;
    while (ia < a.size() || ib < b.size()) {
        if (maxComponents > 0 && comp >= maxComponents) break;
        long na = 0, nb = 0;
        while (ia < a.size() && a[ia] >= '0' && a[ia] <= '9') na = na * 10 + (a[ia++] - '0');
        while (ib < b.size() && b[ib] >= '0' && b[ib] <= '9') nb = nb * 10 + (b[ib++] - '0');
        if (na != nb) return na < nb ? -1 : 1;
        while (ia < a.size() && !(a[ia] >= '0' && a[ia] <= '9')) ia++;  // 다음 숫자 그룹으로
        while (ib < b.size() && !(b[ib] >= '0' && b[ib] <= '9')) ib++;
        comp++;
    }
    return 0;
}

// 강제: major(a)가 올라감. (a<latest.a) → 강제 업데이트.
bool LeaderboardManager::needsForceUpdate() const
{
    if (m_latestVersion.empty()) return false;
    std::string cur = Application::getInstance()->getVersion();
    if (cur.empty()) return false;  // 로컬 버전 불명(win32 등) → 차단 안 함 (fail-open)
    return compareVersion(cur, m_latestVersion, 1) < 0;  // major(a)만 비교
}

// 권장: a는 같고 minor(b)만 올라감. (a==latest.a && a.b<latest.a.b) → 권장 업데이트.
// c(patch)만 다른 경우는 어느 쪽도 아님 → 알림 없음(무음).
bool LeaderboardManager::hasRecommendedUpdate() const
{
    if (m_latestVersion.empty()) return false;
    std::string cur = Application::getInstance()->getVersion();
    if (cur.empty()) return false;
    return compareVersion(cur, m_latestVersion, 1) == 0    // major(a) 동일
        && compareVersion(cur, m_latestVersion, 2) < 0;    // minor(b)는 뒤처짐
}

// 패치: a·b는 같고 patch(c)만 뒤처짐. (소극적 선택적 안내)
bool LeaderboardManager::hasPatchUpdate() const
{
    if (m_latestVersion.empty()) return false;
    std::string cur = Application::getInstance()->getVersion();
    if (cur.empty()) return false;
    return compareVersion(cur, m_latestVersion, 2) == 0    // major.minor(a.b) 동일
        && compareVersion(cur, m_latestVersion) < 0;       // 전체로는 뒤처짐 → c만 다름
}

// "다시 묻지 않기" 억제 — 사용자가 옵트아웃한 버전을 UserDefault에 저장(버전별).
static const char* KEY_UPDATE_OPTOUT = "update_optout_version";

bool LeaderboardManager::isOptionalUpdateSuppressed() const
{
    if (m_latestVersion.empty()) return false;
    std::string v = UserDefault::getInstance()->getStringForKey(KEY_UPDATE_OPTOUT, "");
    return !v.empty() && v == m_latestVersion;
}

void LeaderboardManager::suppressOptionalUpdate()
{
    if (m_latestVersion.empty()) return;
    UserDefault::getInstance()->setStringForKey(KEY_UPDATE_OPTOUT, m_latestVersion);
    UserDefault::getInstance()->flush();
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
#ifdef ENABLE_AWARD_COMMENT
                // 소감 조인 후 서빙 (joinCommentsAndServe가 cocos 스레드로 dispatch)
                joinCommentsAndServe(level, entries, callback);
#else
                Director::getInstance()->getScheduler()->performFunctionInCocosThread(
                    [callback, entries]() { callback(entries); });
#endif
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

#ifdef ENABLE_AWARD_COMMENT
            // 소감을 playFabId로 조인 후 서빙
            joinCommentsAndServe(level, entries, callback);
#else
            if (callback) callback(entries);
#endif
        });
}

#ifdef ENABLE_AWARD_COMMENT
// ─────────────────────────────────────────────────────────────
//  랭킹 Top10 수상소감 (Award Comments)
// ─────────────────────────────────────────────────────────────

std::string LeaderboardManager::awardGroupId(int level)
{
    return StringUtils::format("awards_L%02d", level);
}

int LeaderboardManager::utf8Length(const std::string& s)
{
    return (int)StringUtils::getCharacterCountInUTF8String(s);
}

bool LeaderboardManager::clientFilterComment(const std::string& text, std::string& reasonOut)
{
    // trim
    size_t a = text.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) { reasonOut = "empty"; return false; }
    size_t b = text.find_last_not_of(" \t\r\n");
    std::string t = text.substr(a, b - a + 1);

    int len = utf8Length(t);
    if (len == 0)            { reasonOut = "empty";    return false; }
    if (len > AWARD_MAX_CP)  { reasonOut = "too_long"; return false; }

    // 소문자 사본 (ASCII만 영향, 멀티바이트는 그대로 유지되어 한글 매칭 정상)
    std::string low = toLowerAscii(t);
    if (textHasLink(low))      { reasonOut = "link";      return false; }
    if (textHasProfanity(low)) { reasonOut = "profanity"; return false; }

    reasonOut.clear();
    return true;
}

void LeaderboardManager::bootstrapAwardGroups(bool force)
{
    if (!force) {
        if (m_awardGroupsBootstrapped) return;
        if (UserDefault::getInstance()->getBoolForKey("award_groups_bootstrapped", false)) {
            m_awardGroupsBootstrapped = true;
            return;
        }
    }
    if (!isLoggedIn()) return;  // 로그인 성공 콜백에서 다시 호출됨

    for (int lv = 3; lv <= MAX_PLAY_LEVEL; ++lv) {
        std::string gid  = awardGroupId(lv);
        std::string body = StringUtils::format("{\"SharedGroupId\":\"%s\"}", gid.c_str());
        httpPost(BASE_URL + "/Client/CreateSharedGroup", body, m_sessionTicket,
            [gid](bool ok, const std::string&) {
                log("PlayFab CreateSharedGroup %s: %s",
                    gid.c_str(), ok ? "created" : "exists/err (ignored)");
            });
    }
    m_awardGroupsBootstrapped = true;
    UserDefault::getInstance()->setBoolForKey("award_groups_bootstrapped", true);
    UserDefault::getInstance()->flush();
}

void LeaderboardManager::invalidateComments(int level)
{
    m_commentCache.erase(level);
}

void LeaderboardManager::deleteAllAwardComments()
{
    if (!isLoggedIn()) {
        login([this](bool ok) { if (ok) deleteAllAwardComments(); });
        return;
    }
    for (int lv = 3; lv <= MAX_PLAY_LEVEL; ++lv) {
        std::string body = StringUtils::format(
            "{\"FunctionName\":\"deleteAwardComment\","
            "\"FunctionParameter\":{\"level\":%d},"
            "\"GeneratePlayStreamEvent\":false}", lv);
        httpPost(BASE_URL + "/Client/ExecuteCloudScript", body, m_sessionTicket,
            [lv](bool ok, const std::string&) {
                log("PlayFab deleteAwardComment L%d: %s", lv, ok ? "OK" : "FAIL");
            });
    }
    m_commentCache.clear();
}

void LeaderboardManager::fetchComments(int level,
    std::function<void(const std::map<std::string, std::string>&)> callback)
{
    if (!isLoggedIn()) {
        login([this, level, callback](bool ok) {
            if (ok) fetchComments(level, callback);
            else if (callback) callback({});
        });
        return;
    }

    // 캐시 확인
    auto it = m_commentCache.find(level);
    if (it != m_commentCache.end()) {
        double elapsedH = difftime(time(nullptr), it->second.cachedAt) / 3600.0;
        if (elapsedH < CACHE_TTL_HOURS) {
            if (callback) {
                auto byId = it->second.byId;
                Director::getInstance()->getScheduler()->performFunctionInCocosThread(
                    [callback, byId]() { callback(byId); });
            }
            return;
        }
    }

    std::string body = StringUtils::format(
        "{\"SharedGroupId\":\"%s\",\"GetMembers\":false}", awardGroupId(level).c_str());

    httpPost(BASE_URL + "/Client/GetSharedGroupData", body, m_sessionTicket,
        [this, level, callback](bool ok, const std::string& resp) {
            std::map<std::string, std::string> out;
            if (!ok) {
                // 그룹 없음/오류 — 빈 결과 (캐시하지 않음)
                if (callback) callback(out);
                return;
            }
            rapidjson::Document doc;
            doc.Parse(resp.c_str());
            if (!doc.HasParseError() && doc.HasMember("data")) {
                const auto& data = doc["data"];
                if (data.HasMember("Data") && data["Data"].IsObject()) {
                    for (auto m = data["Data"].MemberBegin(); m != data["Data"].MemberEnd(); ++m) {
                        if (!m->value.IsObject() ||
                            !m->value.HasMember("Value") || !m->value["Value"].IsString())
                            continue;
                        // Value = JSON 문자열 {"t":"소감","ts":epochMs}
                        rapidjson::Document inner;
                        inner.Parse(m->value["Value"].GetString());
                        if (!inner.HasParseError() &&
                            inner.HasMember("t") && inner["t"].IsString()) {
                            out[m->name.GetString()] = inner["t"].GetString();
                        }
                    }
                }
            }
            m_commentCache[level] = CommentCacheEntry{ out, time(nullptr) };
            log("LeaderboardManager: cached comments L%d (%d entries)", level, (int)out.size());
            if (callback) callback(out);
        });
}

void LeaderboardManager::writeAwardComment(int level, const std::string& text,
    std::function<void(bool, const std::string&)> callback)
{
    // 1) 클라 경량 필터 — 즉시 거부
    std::string reason;
    if (!clientFilterComment(text, reason)) {
        if (callback) callback(false, reason);
        return;
    }

    if (!isLoggedIn()) {
        login([this, level, text, callback](bool ok) {
            if (ok) doWriteAwardComment(level, text, true, callback);
            else if (callback) callback(false, "login");
        });
        return;
    }
    doWriteAwardComment(level, text, true, callback);
}

void LeaderboardManager::doWriteAwardComment(int level, const std::string& text,
    bool allowRetry, std::function<void(bool, const std::string&)> callback)
{
    std::string body = StringUtils::format(
        "{\"FunctionName\":\"writeAwardComment\","
        "\"FunctionParameter\":{\"level\":%d,\"comment\":\"%s\"},"
        "\"GeneratePlayStreamEvent\":false}",
        level, escapeJson(text).c_str());

    httpPost(BASE_URL + "/Client/ExecuteCloudScript", body, m_sessionTicket,
        [this, level, text, allowRetry, callback](bool ok, const std::string& resp) {
            if (!ok) { if (callback) callback(false, "network"); return; }

            rapidjson::Document doc;
            doc.Parse(resp.c_str());
            if (doc.HasParseError() || !doc.HasMember("data")) {
                if (callback) callback(false, "parse");
                return;
            }
            const auto& data = doc["data"];

            // CloudScript 실행 자체가 실패한 경우
            if (data.HasMember("Error") && data["Error"].IsObject()) {
                log("PlayFab writeAwardComment script error: %.300s", resp.c_str());
                if (callback) callback(false, "script_error");
                return;
            }
            if (!data.HasMember("FunctionResult") || !data["FunctionResult"].IsObject()) {
                if (callback) callback(false, "parse");
                return;
            }
            const auto& fr = data["FunctionResult"];
            bool okFlag = fr.HasMember("ok") && fr["ok"].IsBool() && fr["ok"].GetBool();
            std::string reason = (fr.HasMember("reason") && fr["reason"].IsString())
                                 ? fr["reason"].GetString() : "";

            if (okFlag) {
                // 작성 성공 — 소감/리더보드 캐시 무효화 (즉시 반영은 상위에서 재조회)
                m_commentCache.erase(level);
                m_leaderboardCache.erase(level);
                log("PlayFab writeAwardComment L%d: OK", level);
                if (callback) callback(true, "");
                return;
            }

            // 그룹 미생성 → 부트스트랩 후 1회 재시도
            if (reason == "no_group" && allowRetry) {
                log("PlayFab writeAwardComment L%d: no_group, bootstrapping + retry", level);
                bootstrapAwardGroups(true);
                Director::getInstance()->getScheduler()->schedule(
                    [this, level, text, callback](float) {
                        doWriteAwardComment(level, text, false, callback);
                    },
                    (void*)this, 0.0f, 0, 1.0f, false,
                    "award_write_retry_" + std::to_string(level));
                return;
            }

            log("PlayFab writeAwardComment L%d: FAIL reason=%s", level, reason.c_str());
            if (callback) callback(false, reason);
        });
}

void LeaderboardManager::joinCommentsAndServe(int level, std::vector<LeaderboardEntry> entries,
    std::function<void(const std::vector<LeaderboardEntry>&)> callback)
{
    if (!callback) return;
    // 마스터 스위치 OFF → 소감 조인 생략(숨김). 이름/시간 등 나머지는 그대로 서빙.
    if (!m_awardEnabled) {
        Director::getInstance()->getScheduler()->performFunctionInCocosThread(
            [entries, callback]() { callback(entries); });
        return;
    }
    // 소감은 별도 캐시(1h) — 조회 후 playFabId로 매칭해 각 엔트리에 채운다.
    fetchComments(level,
        [entries, callback](const std::map<std::string, std::string>& comments) mutable {
            for (auto& e : entries) {
                auto it = comments.find(e.playFabId);
                if (it != comments.end()) e.comment = it->second;
            }
            callback(entries);
        });
}
#endif // ENABLE_AWARD_COMMENT


// ─────────────────────────────────────────────────────────────────────────
//  리플레이 공유 (2차) — Shared Group replays_L%02d, award_comment 인프라 미러
// ─────────────────────────────────────────────────────────────────────────

std::string LeaderboardManager::replayGroupId(int level)
{
    return StringUtils::format("replays_L%02d", level);
}

void LeaderboardManager::bootstrapReplayGroups(bool force)
{
    if (!force) {
        if (m_replayGroupsBootstrapped) return;
        if (UserDefault::getInstance()->getBoolForKey("replay_groups_bootstrapped", false)) {
            m_replayGroupsBootstrapped = true;
            return;
        }
    }
    if (!isLoggedIn()) return;  // 로그인 성공 콜백에서 다시 호출됨

    for (int lv = 3; lv <= REPLAY_MAX_LEVEL; ++lv) {
        std::string gid  = replayGroupId(lv);
        std::string body = StringUtils::format("{\"SharedGroupId\":\"%s\"}", gid.c_str());
        httpPost(BASE_URL + "/Client/CreateSharedGroup", body, m_sessionTicket,
            [gid](bool ok, const std::string&) {
                log("PlayFab CreateSharedGroup %s: %s",
                    gid.c_str(), ok ? "created" : "exists/err (ignored)");
            });
    }
    m_replayGroupsBootstrapped = true;
    UserDefault::getInstance()->setBoolForKey("replay_groups_bootstrapped", true);
    UserDefault::getInstance()->flush();
}

void LeaderboardManager::invalidateReplays(int level)
{
    m_replayCache.erase(level);
}

void LeaderboardManager::fetchReplays(int level,
    std::function<void(const std::map<std::string, std::string>&)> callback)
{
    if (!isLoggedIn()) {
        login([this, level, callback](bool ok) {
            if (ok) fetchReplays(level, callback);
            else if (callback) callback({});
        });
        return;
    }

    // 캐시 확인
    auto it = m_replayCache.find(level);
    if (it != m_replayCache.end()) {
        double elapsedH = difftime(time(nullptr), it->second.cachedAt) / 3600.0;
        if (elapsedH < CACHE_TTL_HOURS) {
            if (callback) {
                auto byId = it->second.byId;
                Director::getInstance()->getScheduler()->performFunctionInCocosThread(
                    [callback, byId]() { callback(byId); });
            }
            return;
        }
    }

    std::string body = StringUtils::format(
        "{\"SharedGroupId\":\"%s\",\"GetMembers\":false}", replayGroupId(level).c_str());

    httpPost(BASE_URL + "/Client/GetSharedGroupData", body, m_sessionTicket,
        [this, level, callback](bool ok, const std::string& resp) {
            std::map<std::string, std::string> out;
            if (!ok) {
                if (callback) callback(out);   // 그룹 없음/오류 — 캐시하지 않음
                return;
            }
            rapidjson::Document doc;
            doc.Parse(resp.c_str());
            if (!doc.HasParseError() && doc.HasMember("data")) {
                const auto& data = doc["data"];
                if (data.HasMember("Data") && data["Data"].IsObject()) {
                    for (auto m = data["Data"].MemberBegin(); m != data["Data"].MemberEnd(); ++m) {
                        if (!m->value.IsObject() ||
                            !m->value.HasMember("Value") || !m->value["Value"].IsString())
                            continue;
                        // Value = JSON {"r":"<base64>","ms":finalMs}
                        rapidjson::Document inner;
                        inner.Parse(m->value["Value"].GetString());
                        if (!inner.HasParseError() &&
                            inner.HasMember("r") && inner["r"].IsString()) {
                            out[m->name.GetString()] = inner["r"].GetString();
                        }
                    }
                }
            }
            m_replayCache[level] = ReplayCacheEntry{ out, time(nullptr) };
            log("LeaderboardManager: cached replays L%d (%d entries)", level, (int)out.size());
            if (callback) callback(out);
        });
}

void LeaderboardManager::uploadReplay(int level, const std::string& blob,
    std::function<void(bool)> callback)
{
    if (blob.empty() || level > REPLAY_MAX_LEVEL) {
        if (callback) callback(false);
        return;
    }
    if (!isLoggedIn()) {
        login([this, level, blob, callback](bool ok) {
            if (ok) doUploadReplay(level, blob, true, callback);
            else if (callback) callback(false);
        });
        return;
    }
    doUploadReplay(level, blob, true, callback);
}

void LeaderboardManager::doUploadReplay(int level, const std::string& blob,
    bool allowRetry, std::function<void(bool)> callback)
{
    std::string body = StringUtils::format(
        "{\"FunctionName\":\"writeReplay\","
        "\"FunctionParameter\":{\"level\":%d,\"blob\":\"%s\"},"
        "\"GeneratePlayStreamEvent\":false}",
        level, escapeJson(blob).c_str());

    httpPost(BASE_URL + "/Client/ExecuteCloudScript", body, m_sessionTicket,
        [this, level, blob, allowRetry, callback](bool ok, const std::string& resp) {
            if (!ok) { if (callback) callback(false); return; }

            rapidjson::Document doc;
            doc.Parse(resp.c_str());
            if (doc.HasParseError() || !doc.HasMember("data")) { if (callback) callback(false); return; }
            const auto& data = doc["data"];

            if (data.HasMember("Error") && data["Error"].IsObject()) {
                log("PlayFab writeReplay script error: %.300s", resp.c_str());
                if (callback) callback(false);
                return;
            }
            if (!data.HasMember("FunctionResult") || !data["FunctionResult"].IsObject()) {
                if (callback) callback(false);
                return;
            }
            const auto& fr = data["FunctionResult"];
            bool okFlag = fr.HasMember("ok") && fr["ok"].IsBool() && fr["ok"].GetBool();
            std::string reason = (fr.HasMember("reason") && fr["reason"].IsString())
                                 ? fr["reason"].GetString() : "";

            if (okFlag) {
                m_replayCache.erase(level);   // 다음 조회 시 갱신
                log("PlayFab writeReplay L%d: OK", level);
                if (callback) callback(true);
                return;
            }

            // 그룹 미생성 → 부트스트랩 후 1회 재시도
            if (reason == "no_group" && allowRetry) {
                log("PlayFab writeReplay L%d: no_group, bootstrapping + retry", level);
                bootstrapReplayGroups(true);
                Director::getInstance()->getScheduler()->schedule(
                    [this, level, blob, callback](float) {
                        doUploadReplay(level, blob, false, callback);
                    },
                    (void*)this, 0.0f, 0, 1.0f, false,
                    "replay_write_retry_" + std::to_string(level));
                return;
            }

            log("PlayFab writeReplay L%d: FAIL reason=%s", level, reason.c_str());
            if (callback) callback(false);
        });
}


// ─────────────────────────────────────────────────────────────────────────
//  도전모드 격파/복수 (3차) — Shared Group battles_L%02d, replay 인프라 미러
// ─────────────────────────────────────────────────────────────────────────

std::string LeaderboardManager::battleGroupId(int level)
{
    return StringUtils::format("battles_L%02d", level);
}

void LeaderboardManager::bootstrapBattleGroups(bool force)
{
    if (!force) {
        if (m_battleGroupsBootstrapped) return;
        if (UserDefault::getInstance()->getBoolForKey("battle_groups_bootstrapped", false)) {
            m_battleGroupsBootstrapped = true;
            return;
        }
    }
    if (!isLoggedIn()) return;  // 로그인 성공 콜백에서 다시 호출됨

    for (int lv = 3; lv <= BATTLE_MAX_LEVEL; ++lv) {
        std::string gid  = battleGroupId(lv);
        std::string body = StringUtils::format("{\"SharedGroupId\":\"%s\"}", gid.c_str());
        httpPost(BASE_URL + "/Client/CreateSharedGroup", body, m_sessionTicket,
            [gid](bool ok, const std::string&) {
                log("PlayFab CreateSharedGroup %s: %s",
                    gid.c_str(), ok ? "created" : "exists/err (ignored)");
            });
    }
    m_battleGroupsBootstrapped = true;
    UserDefault::getInstance()->setBoolForKey("battle_groups_bootstrapped", true);
    UserDefault::getInstance()->flush();
}

void LeaderboardManager::recordBattle(int level, const std::string& loserId, int aValWas,
    std::function<void(bool, const std::string&)> callback)
{
    if (loserId.empty() || level > BATTLE_MAX_LEVEL) {
        if (callback) callback(false, "bad_args");
        return;
    }
    if (!isLoggedIn()) {
        login([this, level, loserId, aValWas, callback](bool ok) {
            if (ok) doRecordBattle(level, loserId, aValWas, 1, callback);
            else if (callback) callback(false, "network");
        });
        return;
    }
    doRecordBattle(level, loserId, aValWas, 1, callback);
}

void LeaderboardManager::doRecordBattle(int level, const std::string& loserId, int aValWas,
    int attempt, std::function<void(bool, const std::string&)> callback)
{
    std::string body = StringUtils::format(
        "{\"FunctionName\":\"recordBattle\","
        "\"FunctionParameter\":{\"level\":%d,\"loserId\":\"%s\",\"aValWas\":%d},"
        "\"GeneratePlayStreamEvent\":false}",
        level, escapeJson(loserId).c_str(), aValWas);

    httpPost(BASE_URL + "/Client/ExecuteCloudScript", body, m_sessionTicket,
        [this, level, loserId, aValWas, attempt, callback](bool ok, const std::string& resp) {
            if (!ok) { if (callback) callback(false, "network"); return; }

            rapidjson::Document doc;
            doc.Parse(resp.c_str());
            if (doc.HasParseError() || !doc.HasMember("data")) { if (callback) callback(false, "parse"); return; }
            const auto& data = doc["data"];

            if (data.HasMember("Error") && data["Error"].IsObject()) {
                log("PlayFab recordBattle script error: %.300s", resp.c_str());
                if (callback) callback(false, "script_error");
                return;
            }
            if (!data.HasMember("FunctionResult") || !data["FunctionResult"].IsObject()) {
                if (callback) callback(false, "parse");
                return;
            }
            const auto& fr = data["FunctionResult"];
            bool okFlag = fr.HasMember("ok") && fr["ok"].IsBool() && fr["ok"].GetBool();
            std::string reason = (fr.HasMember("reason") && fr["reason"].IsString())
                                 ? fr["reason"].GetString() : "";

            if (okFlag) {
                m_battleCache.erase(level);   // 다음 랭킹보드 조회 시 낙인 갱신
                log("PlayFab recordBattle L%d: OK (loser=%s, attempt=%d)", level, loserId.c_str(), attempt);
                if (callback) callback(true, "");
                return;
            }

            // 재시도 판단:
            //  - no_group : Shared Group 미생성 → 부트스트랩 후 재시도(1초 뒤)
            //  - not_passed / no_a_record / no_b_record : 신기록 리더보드 전파 지연으로 추정 → 재시도(3초 뒤)
            bool propDelay = (reason == "not_passed" || reason == "no_a_record" || reason == "no_b_record");
            bool noGroup   = (reason == "no_group");
            if ((noGroup || propDelay) && attempt < BATTLE_MAX_ATTEMPT) {
                if (noGroup) bootstrapBattleGroups(true);
                float delay = noGroup ? 1.0f : 3.0f;
                log("PlayFab recordBattle L%d: reason=%s → retry (attempt %d→%d, %.0fs)",
                    level, reason.c_str(), attempt, attempt + 1, delay);
                Director::getInstance()->getScheduler()->schedule(
                    [this, level, loserId, aValWas, attempt, callback](float) {
                        doRecordBattle(level, loserId, aValWas, attempt + 1, callback);
                    },
                    (void*)this, 0.0f, 0, delay, false,
                    StringUtils::format("battle_write_retry_%d_%d", level, attempt));
                return;
            }

            log("PlayFab recordBattle L%d: FAIL reason=%s (attempt=%d) raw=%.400s",
                level, reason.c_str(), attempt, resp.c_str());
            if (callback) callback(false, reason);
        });
}

void LeaderboardManager::deleteBattle(int level, std::function<void(bool)> callback)
{
    if (!isLoggedIn()) {
        login([this, level, callback](bool ok) {
            if (ok) deleteBattle(level, callback);
            else if (callback) callback(false);
        });
        return;
    }
    std::string body = StringUtils::format(
        "{\"FunctionName\":\"deleteBattle\","
        "\"FunctionParameter\":{\"level\":%d},"
        "\"GeneratePlayStreamEvent\":false}", level);
    httpPost(BASE_URL + "/Client/ExecuteCloudScript", body, m_sessionTicket,
        [this, level, callback](bool ok, const std::string&) {
            if (ok) m_battleCache.erase(level);
            log("PlayFab deleteBattle L%d: %s", level, ok ? "OK" : "FAIL");
            if (callback) callback(ok);
        });
}

void LeaderboardManager::invalidateBattles(int level)
{
    m_battleCache.erase(level);
}

void LeaderboardManager::fetchBattles(int level,
    std::function<void(const std::map<std::string, std::string>&)> callback)
{
    if (!isLoggedIn()) {
        login([this, level, callback](bool ok) {
            if (ok) fetchBattles(level, callback);
            else if (callback) callback({});
        });
        return;
    }

    // 캐시 확인
    auto it = m_battleCache.find(level);
    if (it != m_battleCache.end()) {
        double elapsedH = difftime(time(nullptr), it->second.cachedAt) / 3600.0;
        if (elapsedH < CACHE_TTL_HOURS) {
            if (callback) {
                auto byLoser = it->second.byLoser;
                Director::getInstance()->getScheduler()->performFunctionInCocosThread(
                    [callback, byLoser]() { callback(byLoser); });
            }
            return;
        }
    }

    std::string body = StringUtils::format(
        "{\"SharedGroupId\":\"%s\",\"GetMembers\":false}", battleGroupId(level).c_str());

    httpPost(BASE_URL + "/Client/GetSharedGroupData", body, m_sessionTicket,
        [this, level, callback](bool ok, const std::string& resp) {
            std::map<std::string, std::string> out;   // loserId -> winnerId(by)
            if (!ok) {
                if (callback) callback(out);   // 그룹 없음/오류 — 캐시하지 않음
                return;
            }
            rapidjson::Document doc;
            doc.Parse(resp.c_str());
            if (!doc.HasParseError() && doc.HasMember("data")) {
                const auto& data = doc["data"];
                if (data.HasMember("Data") && data["Data"].IsObject()) {
                    for (auto m = data["Data"].MemberBegin(); m != data["Data"].MemberEnd(); ++m) {
                        if (!m->value.IsObject() ||
                            !m->value.HasMember("Value") || !m->value["Value"].IsString())
                            continue;
                        // Value = JSON {"by":"<winnerId>","aVal":..,"ts":..}
                        rapidjson::Document inner;
                        inner.Parse(m->value["Value"].GetString());
                        if (!inner.HasParseError() &&
                            inner.HasMember("by") && inner["by"].IsString()) {
                            out[m->name.GetString()] = inner["by"].GetString();
                        }
                    }
                }
            }
            m_battleCache[level] = BattleCacheEntry{ out, time(nullptr) };
            log("LeaderboardManager: cached battles L%d (%d entries)", level, (int)out.size());
            if (callback) callback(out);
        });
}


#ifdef ENABLE_REPLAY_LIKE
// ─────────────────────────────────────────────────────────────────────────
//  리플레이 좋아요 (👍) — Shared Group likes_L%02d, battle/replay 인프라 미러
//  저장 value = JSON { n:카운트, v:{투표자id:1} }. 조회는 n만 뽑아 조인 표시.
// ─────────────────────────────────────────────────────────────────────────

std::string LeaderboardManager::likeGroupId(int level)
{
    return StringUtils::format("likes_L%02d", level);
}

void LeaderboardManager::bootstrapLikeGroups(bool force)
{
    if (!force) {
        if (m_likeGroupsBootstrapped) return;
        if (UserDefault::getInstance()->getBoolForKey("like_groups_bootstrapped", false)) {
            m_likeGroupsBootstrapped = true;
            return;
        }
    }
    if (!isLoggedIn()) return;  // 로그인 성공 콜백에서 다시 호출됨

    for (int lv = 3; lv <= LIKE_MAX_LEVEL; ++lv) {
        std::string gid  = likeGroupId(lv);
        std::string body = StringUtils::format("{\"SharedGroupId\":\"%s\"}", gid.c_str());
        httpPost(BASE_URL + "/Client/CreateSharedGroup", body, m_sessionTicket,
            [gid](bool ok, const std::string&) {
                log("PlayFab CreateSharedGroup %s: %s",
                    gid.c_str(), ok ? "created" : "exists/err (ignored)");
            });
    }
    m_likeGroupsBootstrapped = true;
    UserDefault::getInstance()->setBoolForKey("like_groups_bootstrapped", true);
    UserDefault::getInstance()->flush();
}

void LeaderboardManager::invalidateLikes(int level)
{
    m_likeCache.erase(level);
}

void LeaderboardManager::fetchLikes(int level,
    std::function<void(const std::map<std::string, int>&)> callback)
{
    if (!isLoggedIn()) {
        login([this, level, callback](bool ok) {
            if (ok) fetchLikes(level, callback);
            else if (callback) callback({});
        });
        return;
    }

    // 캐시 확인
    auto it = m_likeCache.find(level);
    if (it != m_likeCache.end()) {
        double elapsedH = difftime(time(nullptr), it->second.cachedAt) / 3600.0;
        if (elapsedH < CACHE_TTL_HOURS) {
            if (callback) {
                auto byId = it->second.byId;
                Director::getInstance()->getScheduler()->performFunctionInCocosThread(
                    [callback, byId]() { callback(byId); });
            }
            return;
        }
    }

    std::string body = StringUtils::format(
        "{\"SharedGroupId\":\"%s\",\"GetMembers\":false}", likeGroupId(level).c_str());

    httpPost(BASE_URL + "/Client/GetSharedGroupData", body, m_sessionTicket,
        [this, level, callback](bool ok, const std::string& resp) {
            std::map<std::string, int> out;   // playFabId -> count
            if (!ok) {
                if (callback) callback(out);   // 그룹 없음/오류 — 캐시하지 않음
                return;
            }
            rapidjson::Document doc;
            doc.Parse(resp.c_str());
            if (!doc.HasParseError() && doc.HasMember("data")) {
                const auto& data = doc["data"];
                if (data.HasMember("Data") && data["Data"].IsObject()) {
                    for (auto m = data["Data"].MemberBegin(); m != data["Data"].MemberEnd(); ++m) {
                        if (!m->value.IsObject() ||
                            !m->value.HasMember("Value") || !m->value["Value"].IsString())
                            continue;
                        // Value = JSON {"n":카운트,"v":{...}}  → n만 사용
                        rapidjson::Document inner;
                        inner.Parse(m->value["Value"].GetString());
                        if (!inner.HasParseError() &&
                            inner.HasMember("n") && inner["n"].IsInt()) {
                            int n = inner["n"].GetInt();
                            if (n > 0) out[m->name.GetString()] = n;
                        }
                    }
                }
            }
            m_likeCache[level] = LikeCacheEntry{ out, time(nullptr) };
            log("LeaderboardManager: cached likes L%d (%d entries)", level, (int)out.size());
            if (callback) callback(out);
        });
}

void LeaderboardManager::likeReplay(int level, const std::string& targetId,
    std::function<void(bool, int, bool)> callback)
{
    if (targetId.empty() || level > LIKE_MAX_LEVEL) {
        if (callback) callback(false, 0, false);
        return;
    }
    if (!isLoggedIn()) {
        login([this, level, targetId, callback](bool ok) {
            if (ok) doLikeReplay(level, targetId, true, callback);
            else if (callback) callback(false, 0, false);
        });
        return;
    }
    doLikeReplay(level, targetId, true, callback);
}

void LeaderboardManager::doLikeReplay(int level, const std::string& targetId,
    bool allowRetry, std::function<void(bool, int, bool)> callback)
{
    std::string body = StringUtils::format(
        "{\"FunctionName\":\"likeReplay\","
        "\"FunctionParameter\":{\"level\":%d,\"targetId\":\"%s\"},"
        "\"GeneratePlayStreamEvent\":false}",
        level, escapeJson(targetId).c_str());

    httpPost(BASE_URL + "/Client/ExecuteCloudScript", body, m_sessionTicket,
        [this, level, targetId, allowRetry, callback](bool ok, const std::string& resp) {
            if (!ok) { if (callback) callback(false, 0, false); return; }

            rapidjson::Document doc;
            doc.Parse(resp.c_str());
            if (doc.HasParseError() || !doc.HasMember("data")) { if (callback) callback(false, 0, false); return; }
            const auto& data = doc["data"];

            if (data.HasMember("Error") && data["Error"].IsObject()) {
                log("PlayFab likeReplay script error: %.300s", resp.c_str());
                if (callback) callback(false, 0, false);
                return;
            }
            if (!data.HasMember("FunctionResult") || !data["FunctionResult"].IsObject()) {
                if (callback) callback(false, 0, false);
                return;
            }
            const auto& fr = data["FunctionResult"];
            bool okFlag = fr.HasMember("ok") && fr["ok"].IsBool() && fr["ok"].GetBool();
            std::string reason = (fr.HasMember("reason") && fr["reason"].IsString())
                                 ? fr["reason"].GetString() : "";
            int  n       = (fr.HasMember("n") && fr["n"].IsInt()) ? fr["n"].GetInt() : 0;
            bool already = fr.HasMember("already") && fr["already"].IsBool() && fr["already"].GetBool();

            if (okFlag) {
                m_likeCache.erase(level);   // 다음 랭킹보드 조회 시 갱신
                log("PlayFab likeReplay L%d target=%s: OK (n=%d, already=%d)",
                    level, targetId.c_str(), n, already ? 1 : 0);
                if (callback) callback(true, n, already);
                return;
            }

            // 그룹 미생성 → 부트스트랩 후 1회 재시도
            if (reason == "no_group" && allowRetry) {
                log("PlayFab likeReplay L%d: no_group, bootstrapping + retry", level);
                bootstrapLikeGroups(true);
                Director::getInstance()->getScheduler()->schedule(
                    [this, level, targetId, callback](float) {
                        doLikeReplay(level, targetId, false, callback);
                    },
                    (void*)this, 0.0f, 0, 1.0f, false,
                    "like_write_retry_" + std::to_string(level));
                return;
            }

            log("PlayFab likeReplay L%d: FAIL reason=%s", level, reason.c_str());
            if (callback) callback(false, 0, false);
        });
}
#endif // ENABLE_REPLAY_LIKE

// ─────────────────────────────────────────────────────────────
//  최근 접속 플레이어 (BottomInfoBar 티커) — Shared Group "recent_players"
// ─────────────────────────────────────────────────────────────
void LeaderboardManager::bootstrapRecentGroup(bool force)
{
    if (!force) {
        if (m_recentGroupBootstrapped) return;
        if (UserDefault::getInstance()->getBoolForKey("recent_group_bootstrapped", false)) {
            m_recentGroupBootstrapped = true;
            return;
        }
    }
    if (!isLoggedIn()) return;  // 로그인 성공 콜백에서 다시 호출됨

    std::string body = StringUtils::format(
        "{\"SharedGroupId\":\"%s\"}", recentGroupId().c_str());
    httpPost(BASE_URL + "/Client/CreateSharedGroup", body, m_sessionTicket,
        [](bool ok, const std::string&) {
            log("PlayFab CreateSharedGroup recent_players: %s",
                ok ? "created" : "exists/err (ignored)");
        });
    m_recentGroupBootstrapped = true;
    UserDefault::getInstance()->setBoolForKey("recent_group_bootstrapped", true);
    UserDefault::getInstance()->flush();
}

void LeaderboardManager::touchRecentPlayer(const std::string& nameHint,
    std::function<void(bool)> callback)
{
    if (!isLoggedIn()) {
        login([this, nameHint, callback](bool ok) {
            if (ok) doTouchRecentPlayer(nameHint, true, callback);
            else if (callback) callback(false);
        });
        return;
    }
    doTouchRecentPlayer(nameHint, true, callback);
}

void LeaderboardManager::doTouchRecentPlayer(const std::string& nameHint,
    bool allowRetry, std::function<void(bool)> callback)
{
    std::string body = StringUtils::format(
        "{\"FunctionName\":\"touchRecentPlayer\","
        "\"FunctionParameter\":{\"name\":\"%s\"},"
        "\"GeneratePlayStreamEvent\":false}",
        escapeJson(nameHint).c_str());

    httpPost(BASE_URL + "/Client/ExecuteCloudScript", body, m_sessionTicket,
        [this, nameHint, allowRetry, callback](bool ok, const std::string& resp) {
            if (!ok) { if (callback) callback(false); return; }

            rapidjson::Document doc;
            doc.Parse(resp.c_str());
            if (doc.HasParseError() || !doc.HasMember("data")) { if (callback) callback(false); return; }
            const auto& data = doc["data"];

            if (data.HasMember("Error") && data["Error"].IsObject()) {
                log("PlayFab touchRecentPlayer script error: %.300s", resp.c_str());
                if (callback) callback(false);
                return;
            }
            if (!data.HasMember("FunctionResult") || !data["FunctionResult"].IsObject()) {
                if (callback) callback(false);
                return;
            }
            const auto& fr = data["FunctionResult"];
            bool okFlag = fr.HasMember("ok") && fr["ok"].IsBool() && fr["ok"].GetBool();
            std::string reason = (fr.HasMember("reason") && fr["reason"].IsString())
                                 ? fr["reason"].GetString() : "";

            if (okFlag) {
                m_recentCache.valid = false;   // 다음 티커 조회 시 갱신
                log("PlayFab touchRecentPlayer: OK");
                if (callback) callback(true);
                return;
            }

            // 그룹 미생성 → 부트스트랩 후 1회 재시도
            if (reason == "no_group" && allowRetry) {
                log("PlayFab touchRecentPlayer: no_group, bootstrapping + retry");
                bootstrapRecentGroup(true);
                Director::getInstance()->getScheduler()->schedule(
                    [this, nameHint, callback](float) {
                        doTouchRecentPlayer(nameHint, false, callback);
                    },
                    (void*)this, 0.0f, 0, 1.0f, false, "recent_touch_retry");
                return;
            }

            log("PlayFab touchRecentPlayer: FAIL reason=%s", reason.c_str());
            if (callback) callback(false);
        });
}

void LeaderboardManager::fetchRecentPlayers(
    std::function<void(const std::vector<RecentPlayerEntry>&)> callback)
{
    if (!isLoggedIn()) {
        login([this, callback](bool ok) {
            if (ok) fetchRecentPlayers(callback);
            else if (callback) callback({});
        });
        return;
    }

    // 캐시 확인 (전역, TTL 1h)
    if (m_recentCache.valid) {
        double elapsedH = difftime(time(nullptr), m_recentCache.cachedAt) / 3600.0;
        if (elapsedH < CACHE_TTL_HOURS) {
            if (callback) {
                auto list = m_recentCache.list;
                Director::getInstance()->getScheduler()->performFunctionInCocosThread(
                    [callback, list]() { callback(list); });
            }
            return;
        }
    }

    std::string body = StringUtils::format(
        "{\"SharedGroupId\":\"%s\",\"GetMembers\":false}", recentGroupId().c_str());

    httpPost(BASE_URL + "/Client/GetSharedGroupData", body, m_sessionTicket,
        [this, callback](bool ok, const std::string& resp) {
            std::vector<RecentPlayerEntry> out;
            if (!ok) {
                if (callback) callback(out);   // 그룹 없음/오류 — 캐시하지 않음
                return;
            }
            // {name, cc, ts} 파싱 후 ts 내림차순 정렬 (최근 우선)
            struct Row { RecentPlayerEntry e; double ts; };
            std::vector<Row> rows;
            rapidjson::Document doc;
            doc.Parse(resp.c_str());
            if (!doc.HasParseError() && doc.HasMember("data")) {
                const auto& data = doc["data"];
                if (data.HasMember("Data") && data["Data"].IsObject()) {
                    for (auto m = data["Data"].MemberBegin(); m != data["Data"].MemberEnd(); ++m) {
                        if (!m->value.IsObject() ||
                            !m->value.HasMember("Value") || !m->value["Value"].IsString())
                            continue;
                        rapidjson::Document inner;
                        inner.Parse(m->value["Value"].GetString());
                        if (inner.HasParseError()) continue;
                        Row r;
                        r.ts = (inner.HasMember("ts") && inner["ts"].IsNumber())
                               ? inner["ts"].GetDouble() : 0.0;
                        if (inner.HasMember("name") && inner["name"].IsString())
                            r.e.name = inner["name"].GetString();
                        if (inner.HasMember("cc") && inner["cc"].IsString())
                            r.e.countryCode = inner["cc"].GetString();
                        if (!r.e.name.empty()) rows.push_back(r);
                    }
                }
            }
            std::sort(rows.begin(), rows.end(),
                      [](const Row& a, const Row& b) { return a.ts > b.ts; });
            for (auto& r : rows) out.push_back(r.e);

            // 빈 결과는 캐시하지 않는다(전파 지연/신규 그룹으로 잠깐 비었을 때, 빈 캐시가
            // 1h 동안 목록을 가려 PlayScene 복귀 후에도 빈 목록으로 남는 문제 방지).
            if (!out.empty()) {
                m_recentCache.list     = out;
                m_recentCache.cachedAt = time(nullptr);
                m_recentCache.valid    = true;

                // 디스크 영속화(콜드 스타트 즉시 표시용). 표시에 쓰는 name/cc만 JSON 배열로.
                std::string js = "[";
                for (size_t i = 0; i < out.size(); ++i) {
                    if (i) js += ",";
                    js += "{\"name\":\"" + escapeJson(out[i].name) +
                          "\",\"cc\":\"" + escapeJson(out[i].countryCode) + "\"}";
                }
                js += "]";
                UserDefault::getInstance()->setStringForKey("recent_cache_json", js);
                UserDefault::getInstance()->flush();
            }
            log("LeaderboardManager: fetched recent players (%d entries)", (int)out.size());
            if (callback) callback(out);
        });
}

void LeaderboardManager::invalidateRecent()
{
    m_recentCache.valid = false;
}

std::vector<RecentPlayerEntry> LeaderboardManager::peekPersistedRecent() const
{
    std::vector<RecentPlayerEntry> out;
    std::string js = UserDefault::getInstance()->getStringForKey("recent_cache_json", "");
    if (js.empty()) return out;

    rapidjson::Document doc;
    doc.Parse(js.c_str());
    if (doc.HasParseError() || !doc.IsArray()) return out;
    for (auto it = doc.Begin(); it != doc.End(); ++it) {
        if (!it->IsObject()) continue;
        RecentPlayerEntry e;
        if (it->HasMember("name") && (*it)["name"].IsString()) e.name = (*it)["name"].GetString();
        if (it->HasMember("cc")   && (*it)["cc"].IsString())   e.countryCode = (*it)["cc"].GetString();
        if (!e.name.empty()) out.push_back(e);
    }
    return out;
}
