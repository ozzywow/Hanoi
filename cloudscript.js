// 상위 TOP_N 유지 + 비활성 플레이어 정리 통합 핸들러
// - L3~L4: 7일 이상 미접속 플레이어를 유효 후보에서 제외
// - L5~L9: 활성/비활성 구분 없이 상위 TOP_N만 유지
// - 나머지 전원 0으로 초기화 (EXPIRED_VAL 항목 포함)
// Scheduled Tasks: 0 * * * * (1시간마다)
handlers.maintainLeaderboards = function(args, context) {
    var TOP_N        = (args && args.topN) ? args.topN : 10;
    var EXPIRED_VAL  = 3599590;
    var EXPIRE_MS    = 7 * 24 * 60 * 60 * 1000;
    var EXPIRE_UNTIL = 5;  // 3~4레벨만 비활성 만료 적용
    var now          = Date.now();
    var kept         = 0;
    var cleared      = 0;

    for (var lv = 3; lv < 10; lv++) {
        var pad         = lv < 10 ? "0" : "";
        var statName    = "BestTime_L" + pad + lv;
        var checkExpiry = (lv < EXPIRE_UNTIL);

        // 전체 항목 수집 (0 제외, EXPIRED_VAL 포함)
        var allEntries = [];
        var startPos   = 0;
        while (true) {
            var lb = server.GetLeaderboard({
                StatisticName:      statName,
                StartPosition:      startPos,
                MaxResultsCount:    100,
                ProfileConstraints: { ShowLastLogin: true }
            });
            if (!lb || !lb.Leaderboard || lb.Leaderboard.length === 0) break;

            for (var i = 0; i < lb.Leaderboard.length; i++) {
                var e = lb.Leaderboard[i];
                if (e.StatValue !== 0) allEntries.push(e);
            }

            if (lb.Leaderboard.length < 100) break;
            startPos += 100;
        }

        // 유효 기록 필터: EXPIRED_VAL 제외, L3~L4는 비활성 플레이어도 제외
        var valid = [];
        for (var v = 0; v < allEntries.length; v++) {
            var entry = allEntries[v];
            if (entry.StatValue <= 0 || entry.StatValue >= EXPIRED_VAL) continue;

            if (checkExpiry) {
                var lastLoginMs = 0;
                if (entry.Profile && entry.Profile.LastLogin) {
                    lastLoginMs = new Date(entry.Profile.LastLogin).getTime();
                }
                if (lastLoginMs === 0 || (now - lastLoginMs) > EXPIRE_MS) continue;
            }

            valid.push(entry);
        }

        // 오름차순 정렬 (빠른 기록 우선)
        valid.sort(function(a, b) { return a.StatValue - b.StatValue; });

        // 상위 TOP_N PlayFabId 집합 구성
        var keepSet  = {};
        var topCount = valid.length < TOP_N ? valid.length : TOP_N;
        for (var j = 0; j < topCount; j++) {
            keepSet[valid[j].PlayFabId] = true;
            kept++;
        }

        // 상위 TOP_N 외 전원 0으로 초기화 (EXPIRED_VAL 항목 포함)
        for (var k = 0; k < allEntries.length; k++) {
            if (!keepSet[allEntries[k].PlayFabId]) {
                server.UpdatePlayerStatistics({
                    PlayFabId:  allEntries[k].PlayFabId,
                    Statistics: [{ StatisticName: statName, Value: 0 }]
                });
                cleared++;
            }
        }
    }

    return { kept: kept, cleared: cleared };
};

// ─────────────────────────────────────────────────────────────
//  랭킹 Top10 수상소감 (Award Comment)
//  클라: ExecuteCloudScript { FunctionName:"writeAwardComment",
//                             FunctionParameter:{ level:int, comment:string } }
//  저장: Shared Group "awards_L%02d", key = currentPlayerId,
//        value = JSON { t: 소감, ts: epochMs }, Permission=Public
//  ※ top-10 여부는 표시 시 playFabId 조인으로 자연 필터되므로 서버 순위검증 생략.
//    서버의 책임 = ① 본인 키로만 저장(사칭 차단) ② 길이/링크/욕설 재검증.
// ─────────────────────────────────────────────────────────────
var AWARD_MIN_LEVEL = 3;
var AWARD_MAX_LEVEL = 10;
var AWARD_MAX_CP    = 50;   // 최대 코드포인트 (클라 LeaderboardManager::AWARD_MAX_CP와 일치 필수)

function awardGroupId(level) {
    var pad = (level < 10 ? "0" : "");
    return "awards_L" + pad + level;
}

// 마스터 스위치: 공개 Title Data 키 "award_enabled".
// "0"/"false"/"off" → 비활성. 그 외/키없음/조회실패 → 활성(fail-open, 클라와 동일 정책).
function awardEnabled() {
    try {
        var td = server.GetTitleData({ Keys: ["award_enabled"] });
        var v = td && td.Data && td.Data.award_enabled;
        if (v === "0" || v === "false" || v === "off") return false;
    } catch (e) {}
    return true;
}

// UTF-16 문자열의 코드포인트 길이 (서로게이트 페어 = 1)
function codePointLength(s) {
    var n = 0;
    for (var i = 0; i < s.length; i++) {
        var c = s.charCodeAt(i);
        if (c >= 0xD800 && c <= 0xDBFF) i++;  // high surrogate → 다음 low 소비
        n++;
    }
    return n;
}

// 매칭용 정규화: 소문자 + 공백제거 + 흔한 leet 치환 (원본은 변형하지 않음)
function awardNormalize(s) {
    return s.toLowerCase()
            .replace(/\s+/g, "")
            .replace(/0/g, "o").replace(/1/g, "i").replace(/3/g, "e")
            .replace(/4/g, "a").replace(/5/g, "s").replace(/@/g, "a")
            .replace(/\$/g, "s");
}

// 한글 완성형 → 초성 추출 (초성체 욕설 'ㅅㅂ' 등 대응)
function awardChosung(s) {
    var CHO = "ㄱㄲㄴㄷㄸㄹㅁㅂㅃ"
            + "ㅅㅆㅇㅈㅉㅊㅋㅌㅍㅎ";
    var out = "";
    for (var i = 0; i < s.length; i++) {
        var code = s.charCodeAt(i);
        if (code >= 0xAC00 && code <= 0xD7A3) {
            out += CHO.charAt(Math.floor((code - 0xAC00) / 588));
        } else {
            out += s.charAt(i);
        }
    }
    return out;
}

function awardLoadBanned() {
    try {
        // Title INTERNAL Data — 서버만 읽음(클라가 금칙어 목록을 스크랩 못하게)
        var td = server.GetTitleInternalData({ Keys: ["banned_words"] });
        if (td && td.Data && td.Data.banned_words) {
            var arr = JSON.parse(td.Data.banned_words);
            if (arr && arr.length) return arr;
        }
    } catch (e) {}
    return [];
}

function awardHasLink(s) {
    return /(https?:\/\/|www\.|\.com|\.net|\.io|\.gg|\.kr|\.me|t\.me|kakao|\bhttp\b)/i.test(s);
}

function awardHasProfanity(s, banned) {
    var n   = awardNormalize(s);
    var cho = awardChosung(n);
    for (var i = 0; i < banned.length; i++) {
        var w = banned[i];
        if (!w) continue;
        if (n.indexOf(w) >= 0)   return true;  // 일반/leet 매칭
        if (cho.indexOf(w) >= 0) return true;  // 초성체 매칭
    }
    return false;
}

// 플레이어 이름(랭킹 표시명) 욕설 검증 — 서버 banned_words 최종 게이트.
//  클라: ExecuteCloudScript { FunctionName:"validateName", FunctionParameter:{ name:string } }
//  반환: { ok:true } 또는 { ok:false, reason:"profanity"/"link"/"empty"/"too_long" }
//  ※ 이름 설정 자체는 클라가 UpdateUserTitleDisplayName로 직접 수행. 여기선 검증만.
var NAME_MAX_CP = 12;   // 클라 EditBox maxLength(12)와 일치
handlers.validateName = function(args, context) {
    var name = (args && args.name != null ? String(args.name) : "")
                 .replace(/^\s+|\s+$/g, "");
    var len = codePointLength(name);
    if (len === 0)           return { ok: false, reason: "empty" };
    if (len > NAME_MAX_CP)   return { ok: false, reason: "too_long" };
    if (awardHasLink(name))  return { ok: false, reason: "link" };
    if (awardHasProfanity(name, awardLoadBanned()))
        return { ok: false, reason: "profanity" };
    return { ok: true };
};

// 수상소감 작성/수정
handlers.writeAwardComment = function(args, context) {
    var pid     = currentPlayerId;
    var level   = parseInt(args && args.level, 10);
    var comment = (args && args.comment != null ? String(args.comment) : "")
                    .replace(/^\s+|\s+$/g, "");

    if (!(level >= AWARD_MIN_LEVEL && level <= AWARD_MAX_LEVEL))
        return { ok: false, reason: "bad_level" };

    // 마스터 스위치 OFF → write 거부 (구버전/변조 클라 이중 방어)
    if (!awardEnabled())      return { ok: false, reason: "disabled" };

    var len = codePointLength(comment);
    if (len === 0)            return { ok: false, reason: "empty" };
    if (len > AWARD_MAX_CP)   return { ok: false, reason: "too_long" };
    if (awardHasLink(comment)) return { ok: false, reason: "link" };
    if (awardHasProfanity(comment, awardLoadBanned()))
        return { ok: false, reason: "profanity" };

    var data = {};
    data[pid] = JSON.stringify({ t: comment, ts: Date.now() });

    try {
        server.UpdateSharedGroupData({
            SharedGroupId: awardGroupId(level),
            Data:          data,
            Permission:    "Public"
        });
    } catch (e) {
        // 그룹 미생성 등 — 클라 부트스트랩(CreateSharedGroup) 후 재시도 유도
        return { ok: false, reason: "no_group", detail: String(e) };
    }
    return { ok: true, level: level, comment: comment };
};

// 수상소감 삭제 (본인)
handlers.deleteAwardComment = function(args, context) {
    var pid   = currentPlayerId;
    var level = parseInt(args && args.level, 10);
    if (!(level >= AWARD_MIN_LEVEL && level <= AWARD_MAX_LEVEL))
        return { ok: false, reason: "bad_level" };

    var data = {};
    data[pid] = null;  // null = 해당 키 삭제
    try {
        server.UpdateSharedGroupData({
            SharedGroupId: awardGroupId(level),
            Data:          data,
            Permission:    "Public"
        });
    } catch (e) {
        return { ok: false, reason: "no_group" };
    }
    return { ok: true };
};

// ─────────────────────────────────────────────────────────────
//  관리자 전용 — 신고된 소감 조회/삭제
//  adminKey = Title INTERNAL Data 키 "admin_key" 값과 일치해야 함.
//  (CloudScript 핸들러는 아무 클라이언트나 호출 가능하므로 키 가드 필수)
//  Game Manager → Automation → CloudScript → Run Revision 으로 실행.
// ─────────────────────────────────────────────────────────────
function awardAdminAuthorized(args) {
    try {
        var td = server.GetTitleInternalData({ Keys: ["admin_key"] });
        var key = td && td.Data && td.Data.admin_key;
        return !!key && args && args.adminKey === key;
    } catch (e) { return false; }
}

// 전 레벨(L03~L10) 소감 덤프 — 어떤 소감이 누구(playFabId) 것인지 확인용
handlers.dumpAwardComments = function(args, context) {
    if (!awardAdminAuthorized(args)) return { ok: false, reason: "denied" };
    var out = {};
    for (var lv = AWARD_MIN_LEVEL; lv <= AWARD_MAX_LEVEL; lv++) {
        try {
            var r = server.GetSharedGroupData({ SharedGroupId: awardGroupId(lv) });
            out["L" + lv] = (r && r.Data) ? r.Data : {};
        } catch (e) { out["L" + lv] = null; }
    }
    return { ok: true, data: out };
};

// 특정 유저의 소감 강제 삭제 (신고 대응)
handlers.adminDeleteAwardComment = function(args, context) {
    if (!awardAdminAuthorized(args)) return { ok: false, reason: "denied" };
    var level    = parseInt(args && args.level, 10);
    var targetId = String((args && args.targetId) || "");
    if (!(level >= AWARD_MIN_LEVEL && level <= AWARD_MAX_LEVEL) || !targetId)
        return { ok: false, reason: "bad_args" };

    var data = {}; data[targetId] = null;
    try {
        server.UpdateSharedGroupData({
            SharedGroupId: awardGroupId(level),
            Data:          data,
            Permission:    "Public"
        });
    } catch (e) {
        return { ok: false, reason: "no_group" };
    }
    return { ok: true, level: level, targetId: targetId };
};

// 전체 랭킹 초기화 - 모든 플레이어의 BestTime 통계를 0으로 리셋 (긴급용)
handlers.resetAllLeaderboards = function(args, context) {
    var fixed = 0;

    for (var lv = 3; lv < 10; lv++) {
        var pad      = lv < 10 ? "0" : "";
        var statName = "BestTime_L" + pad + lv;

        var startPos = 0;
        while (true) {
            var lb = server.GetLeaderboard({
                StatisticName:   statName,
                StartPosition:   startPos,
                MaxResultsCount: 100
            });
            if (!lb || !lb.Leaderboard || lb.Leaderboard.length === 0) break;

            for (var i = 0; i < lb.Leaderboard.length; i++) {
                var entry = lb.Leaderboard[i];
                if (entry.StatValue === 0) continue;

                server.UpdatePlayerStatistics({
                    PlayFabId:  entry.PlayFabId,
                    Statistics: [{ StatisticName: statName, Value: 0 }]
                });
                fixed++;
            }

            if (lb.Leaderboard.length < 100) break;
            startPos += 100;
        }
    }

    return { reset: fixed };
};
