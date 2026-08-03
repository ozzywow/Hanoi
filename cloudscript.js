// 상위 TOP_N 유지 + 비활성 플레이어 정리 통합 핸들러
// - L3~L4: 7일 이상 미접속 플레이어를 유효 후보에서 제외
// - L5~L9: 활성/비활성 구분 없이 상위 TOP_N만 유지
// - 나머지 전원 0으로 초기화 (EXPIRED_VAL 항목 포함)
// Scheduled Tasks: 0 * * * * (1시간마다)
//
// ⚠️ CloudScript는 실행 1회당 PlayFab API 호출을 25회로 하드 제한한다
//    (CloudScriptAPIRequestCountExceeded). L3~L10 8레벨 × (리더보드 1 + 공유그룹 3)
//    = 정리 대상이 0이어도 최소 32회 → 단일 실행에 전 레벨 처리 불가.
//    해결: ① API 예산 가드로 한도 전에 중단(남은 작업은 다음 실행으로 이월 — 정리는
//    idempotent라 안전) ② 시간 기반 시작-레벨 로테이션으로 매 실행 다른 지점에서 시작
//    → 예산상 전 레벨을 못 돌려도 결국 모든 레벨이 순환 정리됨(상태 저장 불필요).
handlers.maintainLeaderboards = function(args, context) {
    var TOP_N        = (args && args.topN) ? args.topN : 10;
    var EXPIRED_VAL  = 3599590;
    var EXPIRE_MS    = 7 * 24 * 60 * 60 * 1000;
    var EXPIRE_UNTIL = 5;  // 3~4레벨만 비활성 만료 적용
    var now          = Date.now();
    var kept         = 0;
    var cleared      = 0;

    // ── API 호출 예산 가드 ──
    var API_BUDGET = 23;                       // 하드 리밋 25 - 여유 2
    var apiCalls   = 0;
    function canCall(n) { return (apiCalls + (n || 1)) <= API_BUDGET; }

    var LV_MIN = 3, LV_MAX = 10, LV_SPAN = (LV_MAX - LV_MIN + 1);  // 8
    // 시간(시각) 기반 시작 레벨 로테이션 — 매 실행 다른 레벨부터 시작
    var rotate    = Math.floor(now / (60 * 60 * 1000)) % LV_SPAN;
    var processed = 0;
    var budgetHit = false;

    for (var step = 0; step < LV_SPAN; step++) {
        // 레벨당 최소 리드(리더보드 1 + 공유그룹 3 = 4)를 확보 못 하면 중단
        if (!canCall(4)) { budgetHit = true; break; }

        var lv          = LV_MIN + ((rotate + step) % LV_SPAN);
        var pad         = lv < 10 ? "0" : "";
        var statName    = "BestTime_L" + pad + lv;
        var checkExpiry = (lv < EXPIRE_UNTIL);

        // 전체 항목 수집 (0 제외, EXPIRED_VAL 포함)
        var allEntries = [];
        var startPos   = 0;
        while (canCall(1)) {
            var lb = server.GetLeaderboard({
                StatisticName:      statName,
                StartPosition:      startPos,
                MaxResultsCount:    100,
                ProfileConstraints: { ShowLastLogin: true }
            });
            apiCalls++;
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
        // 예산 소진 시 남은 항목은 다음 실행/로테이션에서 정리 (idempotent)
        for (var k = 0; k < allEntries.length; k++) {
            if (!keepSet[allEntries[k].PlayFabId]) {
                if (!canCall(1)) { budgetHit = true; break; }
                server.UpdatePlayerStatistics({
                    PlayFabId:  allEntries[k].PlayFabId,
                    Statistics: [{ StatisticName: statName, Value: 0 }]
                });
                apiCalls++;
                cleared++;
            }
        }

        // 리플레이 정리(2차): Top10 밖으로 밀려난 랭커의 리플레이 blob 삭제 → 그룹을
        // 항상 현재 Top10(≤10개)로 유지. 없으면 고아 blob 무한 누적 → 조회 payload 폭증.
        if (lv <= REPLAY_MAX_LEVEL && canCall(1)) {
            try {
                var rg = server.GetSharedGroupData({ SharedGroupId: replayGroupId(lv) });
                apiCalls++;
                if (rg && rg.Data) {
                    var delR = {};
                    var nDel = 0;
                    for (var rkey in rg.Data) {
                        if (!keepSet[rkey]) { delR[rkey] = null; nDel++; }
                    }
                    if (nDel > 0 && canCall(1)) {
                        server.UpdateSharedGroupData({
                            SharedGroupId: replayGroupId(lv),
                            Data:          delR,
                            Permission:    "Public"
                        });
                        apiCalls++;
                    }
                }
            } catch (e) { /* 그룹 미생성 등 — 무시 */ }
        }

        // 소감 정리: Top10 밖 랭커 소감 삭제 (리플레이와 동일 — 고아 누적 방지). L10 포함.
        if (canCall(1)) {
            try {
                var cg = server.GetSharedGroupData({ SharedGroupId: awardGroupId(lv) });
                apiCalls++;
                if (cg && cg.Data) {
                    var delC = {};
                    var nDelC = 0;
                    for (var ckey in cg.Data) {
                        if (!keepSet[ckey]) { delC[ckey] = null; nDelC++; }
                    }
                    if (nDelC > 0 && canCall(1)) {
                        server.UpdateSharedGroupData({
                            SharedGroupId: awardGroupId(lv),
                            Data:          delC,
                            Permission:    "Public"
                        });
                        apiCalls++;
                    }
                }
            } catch (e) { /* 그룹 미생성 등 — 무시 */ }
        }

        // 격파 낙인 정리(battle_reward): 패자(key)/격파자(by)가 Top10 밖이거나 TTL 만료 시 삭제.
        if (canCall(1)) {
            try {
                var xg = server.GetSharedGroupData({ SharedGroupId: battleGroupId(lv) });
                apiCalls++;
                if (xg && xg.Data) {
                    var delX = {};
                    var nDelX = 0;
                    for (var xkey in xg.Data) {
                        var dropX = false;
                        if (!keepSet[xkey]) {
                            dropX = true;                       // 패자가 Top10 밖
                        } else {
                            try {
                                var rvX = JSON.parse(xg.Data[xkey].Value);
                                if (!rvX || !keepSet[rvX.by])              dropX = true;  // 격파자 Top10 밖
                                else if (rvX.ts && (now - rvX.ts) > BATTLE_TTL_MS) dropX = true;  // TTL 만료
                            } catch (pe) { dropX = true; }      // 파싱 실패 → 정리
                        }
                        if (dropX) { delX[xkey] = null; nDelX++; }
                    }
                    if (nDelX > 0 && canCall(1)) {
                        server.UpdateSharedGroupData({
                            SharedGroupId: battleGroupId(lv),
                            Data:          delX,
                            Permission:    "Public"
                        });
                        apiCalls++;
                    }
                }
            } catch (e) { /* 그룹 미생성 등 — 무시 */ }
        }

        // 좋아요 정리(replay_like): Top10 밖 대상의 좋아요 키 삭제 → 순위 밖 시 n·v 함께 0 초기화(D3-GC).
        if (canCall(1)) {
            try {
                var lg = server.GetSharedGroupData({ SharedGroupId: likeGroupId(lv) });
                apiCalls++;
                if (lg && lg.Data) {
                    var delL  = {};
                    var nDelL = 0;
                    for (var lkey in lg.Data) {
                        if (!keepSet[lkey]) { delL[lkey] = null; nDelL++; }
                    }
                    if (nDelL > 0 && canCall(1)) {
                        server.UpdateSharedGroupData({
                            SharedGroupId: likeGroupId(lv),
                            Data:          delL,
                            Permission:    "Public"
                        });
                        apiCalls++;
                    }
                }
            } catch (e) { /* 그룹 미생성 등 — 무시 */ }
        }

        processed++;
        if (budgetHit) break;   // 클리어 루프에서 예산 소진 → 다음 실행으로 이월
    }

    // 최근 접속 플레이어 정리(recent_players): ts 내림차순 상위 RECENT_MAX만 유지, 나머지 키 삭제.
    // 레벨 무관 전역 그룹 → 루프 밖에서 1회. 읽기 1 + (삭제 있을 때)쓰기 1.
    if (canCall(2)) {
        try {
            var rp = server.GetSharedGroupData({ SharedGroupId: RECENT_GROUP_ID });
            apiCalls++;
            if (rp && rp.Data) {
                var rArr = [];
                for (var pkey in rp.Data) {
                    var pts = 0;
                    try {
                        var pv = JSON.parse(rp.Data[pkey].Value);
                        if (pv && typeof pv.ts === "number") pts = pv.ts;
                    } catch (pe) { /* 파싱 실패 → ts=0 (가장 먼저 정리 대상) */ }
                    rArr.push({ id: pkey, ts: pts });
                }
                rArr.sort(function(a, b) { return b.ts - a.ts; });   // 최근 우선
                var delP = {};
                var nDelP = 0;
                for (var pi = RECENT_MAX; pi < rArr.length; pi++) { delP[rArr[pi].id] = null; nDelP++; }
                if (nDelP > 0) {
                    server.UpdateSharedGroupData({
                        SharedGroupId: RECENT_GROUP_ID,
                        Data:          delP,
                        Permission:    "Public"
                    });
                    apiCalls++;
                }
            }
        } catch (e) { /* 그룹 미생성 등 — 무시 */ }
    }

    return {
        kept:      kept,
        cleared:   cleared,
        processed: processed,   // 이번 실행에서 처리한 레벨 수
        startLevel: LV_MIN + rotate,
        apiCalls:  apiCalls,
        budgetHit: budgetHit
    };
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
//  리플레이 공유 (2차) — 랭커 터치노트 관전
//  클라: ExecuteCloudScript { FunctionName:"writeReplay",
//                             FunctionParameter:{ level:int, blob:base64 } }
//  저장: Shared Group "replays_L%02d", key = currentPlayerId,
//        value = JSON { r: base64blob, ts: epochMs }, Permission=Public
//  조회: 클라가 GetSharedGroupData(replays_L%02d) 직접 (CloudScript 불필요)
//  서버 책임: ① 본인 키로만 저장(사칭 차단) ② 레벨≤5 + Top10 게이트 ③ 크기 상한
// ─────────────────────────────────────────────────────────────
var REPLAY_MIN_LEVEL = 3;
var REPLAY_MAX_LEVEL = 10;   // 클라 LeaderboardManager::REPLAY_MAX_LEVEL과 일치 필수 (전 레벨)
var REPLAY_MAX_CHARS = 10000;

function replayGroupId(level) {
    var pad = (level < 10 ? "0" : "");
    return "replays_L" + pad + level;
}
function replayStatName(level) {
    var pad = (level < 10 ? "0" : "");
    return "BestTime_L" + pad + level;
}

// 리플레이 업로드 — 레벨≤5 + Top10 달성자만
handlers.writeReplay = function(args, context) {
    var pid   = currentPlayerId;
    var level = parseInt(args && args.level, 10);
    var blob  = (args && args.blob != null) ? String(args.blob) : "";

    if (!(level >= REPLAY_MIN_LEVEL && level <= REPLAY_MAX_LEVEL))
        return { ok: false, reason: "bad_level" };
    if (!blob)                        return { ok: false, reason: "empty" };
    if (blob.length > REPLAY_MAX_CHARS) return { ok: false, reason: "too_big" };

    // 서버측 Top10 검증 (변조 클라의 비랭커 업로드 차단)
    try {
        var lb = server.GetLeaderboardAroundUser({
            StatisticName:   replayStatName(level),
            PlayFabId:       pid,
            MaxResultsCount: 1
        });
        var pos = (lb && lb.Leaderboard && lb.Leaderboard.length > 0)
                    ? lb.Leaderboard[0].Position : -1;   // 0-indexed
        if (pos < 0 || pos > 9) return { ok: false, reason: "not_top10", pos: pos };
    } catch (e) {
        return { ok: false, reason: "rank_check_failed", detail: String(e) };
    }

    var data = {};
    data[pid] = JSON.stringify({ r: blob, ts: Date.now() });
    try {
        server.UpdateSharedGroupData({
            SharedGroupId: replayGroupId(level),
            Data:          data,
            Permission:    "Public"
        });
    } catch (e) {
        return { ok: false, reason: "no_group", detail: String(e) };
    }
    return { ok: true, level: level };
};

// 리플레이 삭제 (본인) — 이름/랭킹 초기화 시 함께 호출 권장
handlers.deleteReplay = function(args, context) {
    var pid   = currentPlayerId;
    var level = parseInt(args && args.level, 10);
    if (!(level >= REPLAY_MIN_LEVEL && level <= REPLAY_MAX_LEVEL))
        return { ok: false, reason: "bad_level" };
    var data = {}; data[pid] = null;
    try {
        server.UpdateSharedGroupData({
            SharedGroupId: replayGroupId(level),
            Data:          data,
            Permission:    "Public"
        });
    } catch (e) { return { ok: false, reason: "no_group" }; }
    return { ok: true };
};

// ─────────────────────────────────────────────────────────────
//  리플레이 좋아요 (👍) — docs/replay_like_plan.md
//  관전(WATCH)에서 본 리플레이에 투표자가 좋아요 → 대상(리플레이 주인)의 누적 좋아요 +1.
//  저장: Shared Group "likes_L%02d", key = 대상 playFabId,
//        value = JSON { n:정수, v:{ 투표자id:1, ... } }, Permission=Public
//        n=누적 좋아요 수(표시용), v=이미 누른 투표자 집합(중복 방지 — 대상 키에 동봉).
//  조회: 클라가 GetSharedGroupData(likes_L%02d) 직접 (n만 조인 표시, v는 무시)
//  서버 책임: ① 자기 좋아요 차단(D6) ② 중복 차단(v) ③ 마스터 스위치(D7)
//  ※ GC(maintainLeaderboards)가 Top10 밖 키 삭제 → 순위 밖 시 n·v 함께 0 초기화(D3-GC).
//    Top10 유지(신기록/순위변동) 대상은 keepSet 포함 → 안 지워짐 = 좋아요 유지.
// ─────────────────────────────────────────────────────────────
var LIKE_MIN_LEVEL = 3;
var LIKE_MAX_LEVEL = 10;   // 클라 LeaderboardManager::LIKE_MAX_LEVEL과 일치 필수

function likeGroupId(level) {
    var pad = (level < 10 ? "0" : "");
    return "likes_L" + pad + level;
}

// 마스터 스위치: 공개 Title Data "like_enabled" (award_enabled 패턴, fail-open)
function likeEnabled() {
    try {
        var td = server.GetTitleData({ Keys: ["like_enabled"] });
        var v = td && td.Data && td.Data.like_enabled;
        if (v === "0" || v === "false" || v === "off") return false;
    } catch (e) {}
    return true;
}

// 좋아요 누르기 — 투표자(currentPlayerId)가 targetId 리플레이에 +1 (중복/자기 차단)
//  클라: ExecuteCloudScript { FunctionName:"likeReplay",
//                             FunctionParameter:{ level:int, targetId:string } }
//  반환: { ok:true, n:카운트, already:bool } | { ok:false, reason:... }
handlers.likeReplay = function(args, context) {
    var voter  = currentPlayerId;
    var level  = parseInt(args && args.level, 10);
    var target = String((args && args.targetId) || "");

    if (!(level >= LIKE_MIN_LEVEL && level <= LIKE_MAX_LEVEL))
        return { ok: false, reason: "bad_level" };
    if (!target)          return { ok: false, reason: "empty_target" };
    if (target === voter) return { ok: false, reason: "self" };    // 자기 좋아요 차단
    if (!likeEnabled())   return { ok: false, reason: "disabled" };

    // 대상 키 읽기 → { n, v }. 그룹 미생성 시 no_group(클라 부트스트랩 후 재시도 유도).
    var obj = { n: 0, v: {} };
    try {
        var g = server.GetSharedGroupData({
            SharedGroupId: likeGroupId(level),
            Keys:          [target]
        });
        if (g && g.Data && g.Data[target] && g.Data[target].Value) {
            var parsed = JSON.parse(g.Data[target].Value);
            if (parsed) {
                if (typeof parsed.n === "number")            obj.n = parsed.n;
                if (parsed.v && typeof parsed.v === "object") obj.v = parsed.v;
            }
        }
    } catch (e) {
        return { ok: false, reason: "no_group", detail: String(e) };
    }

    // 중복: 이미 누른 투표자면 증가 없이 현재값 반환
    if (obj.v[voter]) return { ok: true, already: true, n: obj.n };

    obj.v[voter] = 1;
    obj.n        = obj.n + 1;

    var data = {};
    data[target] = JSON.stringify({ n: obj.n, v: obj.v });
    try {
        server.UpdateSharedGroupData({
            SharedGroupId: likeGroupId(level),
            Data:          data,
            Permission:    "Public"
        });
    } catch (e) {
        return { ok: false, reason: "no_group", detail: String(e) };
    }
    return { ok: true, already: false, n: obj.n };
};

// ─────────────────────────────────────────────────────────────
//  최근 접속 플레이어 (BottomInfoBar 티커) — Shared Group "recent_players"
//  키 = playFabId, value = JSON { name, cc, ts }, Permission=Public
//  조회: 클라가 GetSharedGroupData("recent_players") 직접 → ts 내림차순 정렬 후 상위 N 표시.
//  GC(maintainLeaderboards): ts 내림차순 상위 RECENT_MAX만 유지, 나머지 키 삭제 → payload 상한.
//  기록 조건: 이름이 설정된 플레이어만(무명 제외). 국가는 서버 프로필 Location(IP 기반, 조작불가).
//  ※ 자기 키만 갱신하므로 read-modify-write 레이스 없음(like/battle 패턴 미러).
// ─────────────────────────────────────────────────────────────
var RECENT_GROUP_ID = "recent_players";
var RECENT_MAX      = 20;   // 링버퍼 용량 (클라 표시 개수는 이 이하에서 자유 조절)

// 접속 기록 — 자기 키만 {name, cc, ts}로 덮어씀. name 힌트는 프로필 미반영(전파지연) 대비 폴백.
//  클라: ExecuteCloudScript { FunctionName:"touchRecentPlayer",
//                             FunctionParameter:{ name:string } }
//  반환: { ok:true, name, cc } | { ok:false, reason:"no_name"|"no_group" }
handlers.touchRecentPlayer = function(args, context) {
    var id   = currentPlayerId;
    var hint = String((args && args.name) || "");

    var name = "";
    var cc   = "";
    try {
        var p = server.GetPlayerProfile({
            PlayFabId: id,
            ProfileConstraints: { ShowDisplayName: true, ShowLocations: true }
        });
        if (p && p.PlayerProfile) {
            if (p.PlayerProfile.DisplayName) name = String(p.PlayerProfile.DisplayName);
            if (p.PlayerProfile.Locations && p.PlayerProfile.Locations.length > 0) {
                var lc = p.PlayerProfile.Locations[0].CountryCode;
                if (lc) cc = String(lc).toLowerCase();
            }
        }
    } catch (e) { /* 프로필 조회 실패 — 힌트로 폴백 */ }

    if (!name) name = hint;                                // 프로필 미반영 시 클라 힌트 사용
    if (!name) return { ok: false, reason: "no_name" };    // 무명 플레이어는 미기록

    var data = {};
    data[id] = JSON.stringify({ name: name, cc: cc, ts: Date.now() });
    try {
        server.UpdateSharedGroupData({
            SharedGroupId: RECENT_GROUP_ID,
            Data:          data,
            Permission:    "Public"
        });
    } catch (e) {
        return { ok: false, reason: "no_group", detail: String(e) };   // 클라 부트스트랩 후 재시도 유도
    }
    return { ok: true, name: name, cc: cc };
};

// 관리자 전용 — 최근 접속자 목록 강제 초기화 (테스트 데이터 정리용).
//  그룹은 유지하고 모든 키만 삭제(그룹 삭제 시 클라 재부트스트랩 안 됨 → 유지).
//  adminKey = Title INTERNAL Data "admin_key"와 일치 필수(소감 관리자 게이트 재사용).
//  Game Manager → Automation → CloudScript → Run Revision:
//    FunctionName: clearRecentPlayers, FunctionParameter: { "adminKey": "<키>" }
//  반환: { ok:true, cleared:N } | { ok:false, reason:"denied"|"no_group" }
handlers.clearRecentPlayers = function(args, context) {
    if (!awardAdminAuthorized(args)) return { ok: false, reason: "denied" };
    try {
        var g = server.GetSharedGroupData({ SharedGroupId: RECENT_GROUP_ID });
        if (!g || !g.Data) return { ok: true, cleared: 0 };
        var del = {};
        var n = 0;
        for (var k in g.Data) { del[k] = null; n++; }
        if (n > 0) {
            server.UpdateSharedGroupData({
                SharedGroupId: RECENT_GROUP_ID,
                Data:          del,
                Permission:    "Public"
            });
        }
        return { ok: true, cleared: n };
    } catch (e) {
        return { ok: false, reason: "no_group", detail: String(e) };
    }
};

// ─────────────────────────────────────────────────────────────
//  도전모드 격파/복수 (battle_reward) — docs/battle_reward_plan.md
//  고스트 대결에서 A(도전자)가 B(고스트)를 "상향 추월"하면 격파 낙인 기록.
//  저장: Shared Group "battles_L%02d", key = 패자(B) playFabId,
//        value = JSON { by:A id, aVal:A기록ms, ts }, Permission=Public
//  조회: 클라가 GetSharedGroupData(battles_L%02d) 직접 (렌더 시 id→name/country 맵으로 이름·깃발 해소)
//  성립 조건(모두 만족): ① A 신기록 제출됨 ② A.val<B.val(시간값 기준 A가 더 빠름) ③ 상향(aValWas>=B.val, 이전엔 느렸음) ④ A≠B
//  ※ 리더보드 Position은 원시 ms 내림차순(느릴수록 낮음)이라 Position 비교 금지 → 반드시 시간 값(val)으로 판정.
//    추월 게이트(A.val<B.val)는 서버 통계라 조작 불가. 상향 게이트는 클라 aValWas 기반(잔여 수용).
// ─────────────────────────────────────────────────────────────
var BATTLE_MIN_LEVEL = 3;
var BATTLE_MAX_LEVEL = 10;   // 클라 LeaderboardManager와 일치 필수
var BATTLE_TTL_MS    = 7 * 24 * 60 * 60 * 1000;  // 낙인 7일 후 페이드 (maintainLeaderboards GC)

function battleGroupId(level) {
    var pad = (level < 10 ? "0" : "");
    return "battles_L" + pad + level;
}

// 마스터 스위치: 공개 Title Data 키 "battle_enabled" (award_enabled 패턴, fail-open)
function battleEnabled() {
    try {
        var td = server.GetTitleData({ Keys: ["battle_enabled"] });
        var v = td && td.Data && td.Data.battle_enabled;
        if (v === "0" || v === "false" || v === "off") return false;
    } catch (e) {}
    return true;
}

// 해당 레벨에서 플레이어의 순위/기록 조회 → { pos:0-indexed, val:ms }. 없으면 { pos:-1, val:0 }.
function battleRankOf(level, playFabId) {
    try {
        var lb = server.GetLeaderboardAroundUser({
            StatisticName:   replayStatName(level),   // "BestTime_L%02d" (리플레이와 동일)
            PlayFabId:       playFabId,
            MaxResultsCount: 1
        });
        if (lb && lb.Leaderboard && lb.Leaderboard.length > 0) {
            var e0 = lb.Leaderboard[0];
            if (e0.PlayFabId === playFabId)
                return { pos: e0.Position, val: e0.StatValue };
        }
    } catch (e) {}
    return { pos: -1, val: 0 };
}

// 격파 기록 — 신기록 제출 직후 A가 호출
//  클라: ExecuteCloudScript { FunctionName:"recordBattle",
//                             FunctionParameter:{ level:int, loserId:string, aValWas:int } }
//  aValWas = A의 직전 기록(ms, 신기록 갱신 전 값). 상향 판정용(0=기록없음→상향 간주).
handlers.recordBattle = function(args, context) {
    var pid      = currentPlayerId;                              // A (격파자)
    var level    = parseInt(args && args.level, 10);
    var loserId  = String((args && args.loserId) || "");        // B (패자)
    var aValWas  = parseInt(args && args.aValWas, 10);          // A의 직전 기록(ms). 없으면 0.

    if (!(level >= BATTLE_MIN_LEVEL && level <= BATTLE_MAX_LEVEL))
        return { ok: false, reason: "bad_level" };
    if (!loserId)            return { ok: false, reason: "empty_loser" };
    if (loserId === pid)     return { ok: false, reason: "self" };       // 자기대결 차단
    if (isNaN(aValWas))      aValWas = 0;
    if (!battleEnabled())    return { ok: false, reason: "disabled" };

    var A = battleRankOf(level, pid);
    var B = battleRankOf(level, loserId);
    if (A.pos < 0 || A.val <= 0) return { ok: false, reason: "no_a_record" };
    if (B.pos < 0 || B.val <= 0) return { ok: false, reason: "no_b_record" };

    // 추월 게이트: 시간(값) 기준 — A가 B보다 빨라야(작아야) 함.
    // ※ 리더보드 Position은 원시 ms 내림차순(느릴수록 낮음)이라 Position 비교는 부정확 → 반드시 값으로 판정.
    if (!(A.val < B.val))
        return { ok: false, reason: "not_passed", aVal: A.val, bVal: B.val, aPos: A.pos, bPos: B.pos };

    // 상향 게이트: A가 예전엔 B보다 느렸어야(= B 아래였어야) 함. 이미 B보다 빨랐으면 하향 도전 → 무보상.
    // aValWas = A의 직전 기록(ms). 0/미제공이면 기록 없음 → 상향으로 간주(허용).
    if (!(aValWas <= 0 || aValWas >= B.val))
        return { ok: false, reason: "not_upward", aValWas: aValWas, bVal: B.val };

    var rec = {};
    rec[loserId] = JSON.stringify({ by: pid, aVal: A.val, ts: Date.now() });

    // 넥서시스 플립: 방금 격파한 loserId에게 예전에 "당했던" 내(pid) 낙인이 있으면 함께 삭제.
    // (복수 성공 → 내 피격 낙인 제거). 다른 사람(C)에게 당한 낙인은 유지. 같은 write에 원자 반영.
    var flipped = false;
    try {
        var mine = server.GetSharedGroupData({ SharedGroupId: battleGroupId(level), Keys: [pid] });
        if (mine && mine.Data && mine.Data[pid] && mine.Data[pid].Value) {
            var mv = JSON.parse(mine.Data[pid].Value);
            if (mv && mv.by === loserId) { rec[pid] = null; flipped = true; }
        }
    } catch (e) { /* 조회 실패 — 플립 생략, 기록은 진행 */ }

    try {
        server.UpdateSharedGroupData({
            SharedGroupId: battleGroupId(level),
            Data:          rec,
            Permission:    "Public"
        });
    } catch (e) {
        return { ok: false, reason: "no_group", detail: String(e) };
    }
    return { ok: true, level: level, loserId: loserId, aVal: A.val, bVal: B.val, flipped: flipped };
};

// 격파 낙인 삭제 (본인=패자) — 복수 성공/이름·랭킹 초기화 시 호출. key=본인 id.
handlers.deleteBattle = function(args, context) {
    var pid   = currentPlayerId;
    var level = parseInt(args && args.level, 10);
    if (!(level >= BATTLE_MIN_LEVEL && level <= BATTLE_MAX_LEVEL))
        return { ok: false, reason: "bad_level" };
    var data = {}; data[pid] = null;
    try {
        server.UpdateSharedGroupData({
            SharedGroupId: battleGroupId(level),
            Data:          data,
            Permission:    "Public"
        });
    } catch (e) { return { ok: false, reason: "no_group" }; }
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
//  Game Manager → Automation → CloudScript → Run Revision
//    FunctionName: resetAllLeaderboards, FunctionParameter: { "adminKey": "<키>" }
//  ⚠ 파괴적 + 되돌릴 수 없음. 게이트가 없으면 로그인만 한 아무 클라이언트나 전 랭킹을 밀 수 있다.
handlers.resetAllLeaderboards = function(args, context) {
    if (!awardAdminAuthorized(args)) return { ok: false, reason: "denied" };

    var fixed = 0;

    // L3~L10 전 레벨 (다른 기능들의 *_MAX_LEVEL 과 동일 범위).
    // 안쪽 `lv < 10` 은 한 자리 레벨의 0 패딩용이라 경계가 다르다 — 혼동 주의.
    for (var lv = 3; lv <= 10; lv++) {
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

// ─────────────────────────────────────────────────────────────
// 정크 플레이어 정리 — 이름도 점수도 없는 익명 계정 삭제.
// 세그먼트 기반 Scheduled Task("Actions on each player in segment")에서 플레이어별로 호출됨.
//   보존 조건: DisplayName 있음  OR  BestTime_* 통계(랭킹 점수) 있음.
//   그 외 → server.DeletePlayer 로 삭제.
// "오래됨/비활성" 필터는 세그먼트(예: Lapsed Players = 30일+ 미접속)가 담당하므로,
//   이 함수는 활동 여부와 무관하게 "이름/점수" 만으로 판별(세그먼트가 이미 오래된 것만 넘김).
// ⚠️ 사전 요구: Settings → API Features → "Allow server to delete player accounts" 활성화.
// 테스트: ExecuteCloudScript 로 { PlayFabId:"...", dryRun:true } 넘기면 삭제 없이 판정만 반환.
handlers.cleanupJunkPlayer = function (args, context) {
    var id = (args && args.PlayFabId) ? args.PlayFabId : currentPlayerId;
    if (!id) return { deleted: false, reason: "no-id" };

    // 1) 이름 있으면 보존
    var name = "";
    try {
        var prof = server.GetPlayerProfile({
            PlayFabId: id,
            ProfileConstraints: { ShowDisplayName: true }
        });
        if (prof && prof.PlayerProfile && prof.PlayerProfile.DisplayName)
            name = prof.PlayerProfile.DisplayName;
    } catch (e) {}
    if (name && name.length > 0) return { deleted: false, reason: "name", id: id };

    // 2) BestTime 점수(통계) 있으면 보존
    var hasScore = false;
    try {
        var st = server.GetPlayerStatistics({ PlayFabId: id });
        if (st && st.Statistics) {
            for (var i = 0; i < st.Statistics.length; i++) {
                if (st.Statistics[i].StatisticName.indexOf("BestTime_") === 0) { hasScore = true; break; }
            }
        }
    } catch (e) {}
    if (hasScore) return { deleted: false, reason: "score", id: id };

    // 3) 정크 → 삭제 (dryRun 이면 판정만)
    if (args && args.dryRun) return { deleted: false, reason: "junk-dryrun", id: id };
    // 삭제 에러를 삼키지 않고 결과에 담아 진단 가능하게.
    if (typeof server.DeletePlayer !== "function")
        return { deleted: false, reason: "junk", error: "server.DeletePlayer-not-available", id: id };
    try {
        server.DeletePlayer({ PlayFabId: id });
        return { deleted: true, reason: "junk", id: id };
    } catch (e) {
        return { deleted: false, reason: "junk", error: (e && e.message) ? e.message : String(e), id: id };
    }
};

// ─────────────────────────────────────────────────────────────
//  웹 게시판 (Community Board) — 랜딩 https://ozzywow.github.io/Hanoi/
//
//  ■ 신원 이전 (앱 → 브라우저 원탭 링크)
//    브라우저는 게임 계정을 알 방법이 없다(디바이스ID = 사실상 계정 비밀번호이므로 절대 전달 금지).
//    대신 앱이 일회용 토큰을 발급받아 URL로 넘기고, 웹이 그것을 세션키로 교환한다.
//      1) [앱] issueWebToken           → 토큰(5분, 1회용). Shared Group "web_link" 에 { p:pid, e:만료 }
//      2) [앱] openURL(SHARE_URL + "?wt=" + 토큰)
//      3) [웹] redeemWebToken(token)   → 토큰 소각 + 세션키 발급 + 이름/국가 반환
//      4) [웹] localStorage 에 세션키 저장, 주소창의 ?wt= 는 즉시 제거(history.replaceState)
//
//  ■ 세션키
//    형식 "<playFabId>.<32자 랜덤>". 소유자 판별에 전역 인덱스가 필요 없도록 앞부분에 pid를 박았다.
//    → 소유자 Internal Data "web_session" = { s:시크릿, e:만료 } 만 대조하면 되고 GC가 불필요하다.
//    30일 슬라이딩(글 쓸 때만 갱신 — 읽기는 세션이 필요 없으므로 추가 비용 0).
//    플레이어당 1개. 새로 발급되면 이전 세션은 덮어써져 즉시 무효(= 앱에서 다시 원탭 = 원격 로그아웃).
//    ※ 랜덤은 Math.random 기반(CloudScript에 crypto 없음). 유출 시 피해가 "그 이름으로 글쓰기"로
//      한정되므로 수용 가능한 수준.
//
//  ■ 쓰기 권한
//    writeBoardPost 는 세션키(웹) 또는 인증된 호출자(앱)를 작성자로 삼는다.
//    공유 링크로 들어온 방문자는 세션키가 없고 공용 뷰어 계정에는 이름이 없어 자동으로 읽기 전용.
//
//  ■ 저장: Shared Group "board", key = "<epochMs><랜덤6>", Permission=Public
//    value = JSON { p:작성자pid, n:이름, cc:국가, t:본문, ts:작성시각 }
//    이름을 작성 시점에 박아둬서(비정규화) 웹은 GetSharedGroupData 한 번으로 렌더 가능(조인 불필요).
//    링버퍼: BOARD_MAX 초과 시 오래된 키부터 삭제(writeBoardPost 인라인 — 별도 GC 태스크 불필요).
// ─────────────────────────────────────────────────────────────
var WEB_LINK_GROUP_ID  = "web_link";
var WEB_TOKEN_TTL_MS   = 5 * 60 * 1000;               // 원탭 토큰 5분
var WEB_SESSION_TTL_MS = 30 * 24 * 60 * 60 * 1000;    // 세션 30일(슬라이딩)
var BOARD_GROUP_ID     = "board";
var BOARD_MAX          = 50;    // 링버퍼 용량
var BOARD_MAX_CP       = 140;   // 본문 최대 코드포인트 (랜딩 textarea maxlength와 일치 필수)
var BOARD_AUTHOR_MAX   = 5;     // 작성자당 슬롯 상한 — 한 명이 보드를 밀어내지 못하게
var BOARD_COOLDOWN_MS  = 60 * 1000;   // 연속 작성 쿨다운

var WEB_ALPHABET = "abcdefghijklmnopqrstuvwxyz0123456789";
function webRandom(n) {
    var s = "";
    for (var i = 0; i < n; i++)
        s += WEB_ALPHABET.charAt(Math.floor(Math.random() * WEB_ALPHABET.length));
    return s;
}

// Shared Group 쓰기.
// classic CloudScript에는 CreateSharedGroup이 없으므로(Client API 전용) 그룹 생성은
// 게임의 bootstrapWebGroups()가 담당한다. 아래 create 시도는 혹시 제공되는 환경을 위한
// best-effort이며, 실패해도 조용히 no_group으로 떨어진다.
function webEnsureGroup(gid, data) {
    try {
        server.UpdateSharedGroupData({ SharedGroupId: gid, Data: data, Permission: "Public" });
        return true;
    } catch (e) {
        try { server.CreateSharedGroup({ SharedGroupId: gid }); } catch (e2) { return false; }
        try {
            server.UpdateSharedGroupData({ SharedGroupId: gid, Data: data, Permission: "Public" });
            return true;
        } catch (e3) { return false; }
    }
}

// 마스터 스위치: 공개 Title Data 키 "board_enabled" (award_enabled 패턴, fail-open)
function boardEnabled() {
    try {
        var td = server.GetTitleData({ Keys: ["board_enabled"] });
        var v = td && td.Data && td.Data.board_enabled;
        if (v === "0" || v === "false" || v === "off") return false;
    } catch (e) {}
    return true;
}

// 표시 이름 + 국가(서버 프로필 Location, IP 기반이라 조작 불가)
function webProfile(pid) {
    var out = { name: "", cc: "" };
    try {
        var p = server.GetPlayerProfile({
            PlayFabId: pid,
            ProfileConstraints: { ShowDisplayName: true, ShowLocations: true }
        });
        if (p && p.PlayerProfile) {
            if (p.PlayerProfile.DisplayName) out.name = String(p.PlayerProfile.DisplayName);
            if (p.PlayerProfile.Locations && p.PlayerProfile.Locations.length > 0) {
                var lc = p.PlayerProfile.Locations[0].CountryCode;
                if (lc) out.cc = String(lc).toLowerCase();
            }
        }
    } catch (e) {}
    return out;
}

// 랜딩의 공용 뷰어 계정(CustomId "web_public_viewer")은 누구나 로그인 가능 → 작성 금지.
// Title INTERNAL Data "web_viewer_id" 에 그 PlayFabId를 넣어두면 확실히 차단된다.
// 미설정이어도 이름 없는 계정은 no_name에서 걸리지만, 누군가 그 계정에 이름을 붙이는 순간
// 구멍이 되므로 배포 시 설정 권장(server/PHASE0_DEPLOY.md 참조).
function webIsPublicViewer(pid) {
    try {
        var td = server.GetTitleInternalData({ Keys: ["web_viewer_id"] });
        var v = td && td.Data && td.Data.web_viewer_id;
        if (v && String(v) === String(pid)) return true;
    } catch (e) {}
    return false;
}

// 세션 발급 — 플레이어당 1개(덮어쓰기 = 이전 세션 즉시 무효). 실패 시 "".
function webMintSession(pid) {
    var secret = webRandom(32);
    var rec    = { s: secret, e: Date.now() + WEB_SESSION_TTL_MS };
    try {
        server.UpdateUserInternalData({ PlayFabId: pid, Data: { web_session: JSON.stringify(rec) } });
    } catch (e) { return ""; }
    return pid + "." + secret;
}

// 세션키 → 소유자 playFabId ("" = 무효/만료). slide=true면 만료를 30일 뒤로 연장.
// ※ PlayFabId는 16진 문자열이라 "."을 포함하지 않으므로 첫 "."로 안전하게 분리된다.
function webLookupSession(sk, slide) {
    var s   = String(sk || "");
    var dot = s.indexOf(".");
    if (dot <= 0) return "";
    var pid    = s.substring(0, dot);
    var secret = s.substring(dot + 1);
    if (!secret) return "";

    var rec = null;
    try {
        var d = server.GetUserInternalData({ PlayFabId: pid, Keys: ["web_session"] });
        if (d && d.Data && d.Data.web_session && d.Data.web_session.Value)
            rec = JSON.parse(d.Data.web_session.Value);
    } catch (e) { return ""; }

    if (!rec || rec.s !== secret) return "";
    if (Date.now() > rec.e)       return "";

    if (slide) {
        rec.e = Date.now() + WEB_SESSION_TTL_MS;
        try {
            server.UpdateUserInternalData({ PlayFabId: pid, Data: { web_session: JSON.stringify(rec) } });
        } catch (e) { /* 연장 실패는 치명적이지 않음 — 이번 요청은 통과시킨다 */ }
    }
    return pid;
}

// 원탭 링크용 일회용 토큰 발급 (앱에서 호출, 인증된 호출자 기준).
//  클라: ExecuteCloudScript { FunctionName:"issueWebToken" }
//  반환: { ok:true, token, ttl } | { ok:false, reason:"no_name"|"no_group" }
handlers.issueWebToken = function(args, context) {
    var pid = currentPlayerId;
    if (!pid) return { ok: false, reason: "no_login" };

    // 이름 없는 플레이어는 게시판에 쓸 수 없으므로 토큰도 발급하지 않는다(호출측은 읽기 전용으로 연다).
    var prof = webProfile(pid);
    if (!prof.name) return { ok: false, reason: "no_name" };

    var token = webRandom(24);
    var data  = {};
    data[token] = JSON.stringify({ p: pid, e: Date.now() + WEB_TOKEN_TTL_MS });

    // 만료 토큰 기회적 청소 — 수명이 5분이라 그룹이 커지지 않는다(별도 GC 태스크 불필요).
    try {
        var g = server.GetSharedGroupData({ SharedGroupId: WEB_LINK_GROUP_ID });
        if (g && g.Data) {
            var now = Date.now();
            for (var k in g.Data) {
                var v = null;
                try { v = JSON.parse(g.Data[k].Value); } catch (e2) {}
                if (!v || !v.e || now > v.e) data[k] = null;
            }
        }
    } catch (e) { /* 그룹 미생성 — webEnsureGroup이 생성 */ }

    if (!webEnsureGroup(WEB_LINK_GROUP_ID, data)) return { ok: false, reason: "no_group" };
    return { ok: true, token: token, ttl: WEB_TOKEN_TTL_MS };
};

// 토큰 → 세션키 교환 (웹에서 호출).
//  반환: { ok:true, sessionKey, name, cc, ttl } | { ok:false, reason:"invalid"|"expired"|... }
handlers.redeemWebToken = function(args, context) {
    var token = String((args && args.token) || "");
    if (!token) return { ok: false, reason: "empty" };

    var rec = null;
    try {
        var g = server.GetSharedGroupData({ SharedGroupId: WEB_LINK_GROUP_ID, Keys: [token] });
        if (g && g.Data && g.Data[token] && g.Data[token].Value)
            rec = JSON.parse(g.Data[token].Value);
    } catch (e) { return { ok: false, reason: "no_group" }; }
    if (!rec || !rec.p) return { ok: false, reason: "invalid" };

    // 일회용 — 만료 여부와 무관하게 먼저 소각(재사용 차단)
    var del = {}; del[token] = null;
    try {
        server.UpdateSharedGroupData({ SharedGroupId: WEB_LINK_GROUP_ID, Data: del, Permission: "Public" });
    } catch (e) {}

    if (Date.now() > rec.e) return { ok: false, reason: "expired" };

    var prof = webProfile(rec.p);
    if (!prof.name) return { ok: false, reason: "no_name" };

    var sk = webMintSession(rec.p);
    if (!sk) return { ok: false, reason: "mint_failed" };
    return { ok: true, sessionKey: sk, name: prof.name, cc: prof.cc, ttl: WEB_SESSION_TTL_MS };
};

// 작성 쿨다운 — 마지막 작성 시각을 작성자 Internal Data("board_last")에 보관.
// 반환: 남은 대기 ms (0이면 작성 가능). 조회 실패는 통과시킨다(fail-open, 하우스 스타일).
function boardCooldownLeft(pid) {
    try {
        var d = server.GetUserInternalData({ PlayFabId: pid, Keys: ["board_last"] });
        if (d && d.Data && d.Data.board_last && d.Data.board_last.Value) {
            var last = parseInt(d.Data.board_last.Value, 10);
            if (last > 0) {
                var left = last + BOARD_COOLDOWN_MS - Date.now();
                if (left > 0) return left;
            }
        }
    } catch (e) {}
    return 0;
}

// 게시판 글 작성.
//  웹: FunctionParameter { sessionKey, text } / 앱: { text } (인증된 호출자가 작성자)
//  반환: { ok:true, id, name, cc } | { ok:false, reason:"read_only"|"expired"|"no_name"|검증사유 }
handlers.writeBoardPost = function(args, context) {
    if (!boardEnabled()) return { ok: false, reason: "disabled" };

    var text = (args && args.text != null ? String(args.text) : "").replace(/^\s+|\s+$/g, "");
    var len  = codePointLength(text);
    if (len === 0)          return { ok: false, reason: "empty" };
    if (len > BOARD_MAX_CP) return { ok: false, reason: "too_long" };
    if (awardHasLink(text)) return { ok: false, reason: "link" };
    if (awardHasProfanity(text, awardLoadBanned())) return { ok: false, reason: "profanity" };

    // 작성자 확정 — 세션키(웹)가 있으면 그 소유자, 없으면 인증된 호출자(앱).
    // 세션키는 클라가 보낸 이름을 절대 신뢰하지 않기 위한 장치다: 이름은 아래에서 서버가 직접 조회한다.
    var sk  = (args && args.sessionKey) ? String(args.sessionKey) : "";
    var pid = sk ? webLookupSession(sk, true) : currentPlayerId;
    if (sk && !pid)             return { ok: false, reason: "expired" };    // 세션 만료 → 앱에서 재연결
    if (!pid)                   return { ok: false, reason: "read_only" };
    if (webIsPublicViewer(pid)) return { ok: false, reason: "read_only" };  // 공유 링크 방문자

    // 연속 작성 차단 — 프로필/보드 조회보다 먼저 걸러 불필요한 API 호출을 줄인다.
    var wait = boardCooldownLeft(pid);
    if (wait > 0) return { ok: false, reason: "cooldown", retryMs: wait };

    var prof = webProfile(pid);
    if (!prof.name) return { ok: false, reason: "no_name" };

    var now  = Date.now();
    var id   = String(now) + webRandom(6);
    var data = {};
    data[id] = JSON.stringify({ p: pid, n: prof.name, cc: prof.cc, t: text, ts: now });

    // 정리 대상 수집 — 한 번의 보드 조회로 ①작성자 슬롯 상한 ②전체 링버퍼를 함께 처리한다.
    // id 앞 13자리가 epochMs라 사전순 정렬 == 시간순(연도 2286까지 자릿수 불변).
    var i, k;
    var del = {};
    try {
        var g = server.GetSharedGroupData({ SharedGroupId: BOARD_GROUP_ID });
        if (g && g.Data) {
            var keys = [], mineKeys = [];
            for (k in g.Data) {
                keys.push(k);
                var v = null;
                try { v = JSON.parse(g.Data[k].Value); } catch (e2) {}
                if (v && v.p === pid) mineKeys.push(k);
            }
            keys.sort();                                   // 오래된 것이 앞
            mineKeys.sort();

            // ① 작성자당 슬롯 상한 — 초과분은 "본인" 글 중 오래된 것부터 밀어낸다.
            //    한 명이 연속 작성으로 남의 글을 전부 밀어내는 것을 막는 핵심 장치.
            var mineOver = mineKeys.length + 1 - BOARD_AUTHOR_MAX;
            for (i = 0; i < mineOver; i++) del[mineKeys[i]] = 1;

            // ② 전체 링버퍼 — ①에서 이미 빠질 몫을 뺀 나머지 기준으로 오래된 것부터.
            var remaining = keys.length + 1;               // 이번 글 포함
            for (k in del) remaining--;
            var over = remaining - BOARD_MAX;
            for (i = 0; i < keys.length && over > 0; i++) {
                if (del[keys[i]]) continue;                // 이미 삭제 예정
                del[keys[i]] = 1;
                over--;
            }
        }
    } catch (e) { /* 그룹 미생성 — webEnsureGroup이 생성 */ }

    var dropped = 0;
    for (k in del) { data[k] = null; dropped++; }

    if (!webEnsureGroup(BOARD_GROUP_ID, data)) return { ok: false, reason: "no_group" };

    // 성공 후에만 쿨다운 시작 — 거부된 시도로 대기가 걸리지 않게.
    try {
        server.UpdateUserInternalData({ PlayFabId: pid, Data: { board_last: String(now) } });
    } catch (e) { /* 실패해도 글은 이미 올라갔다 */ }

    return { ok: true, id: id, name: prof.name, cc: prof.cc, dropped: dropped };
};

// 게시판 글 삭제 (본인 글만). 웹: { sessionKey, id } / 앱: { id }
handlers.deleteBoardPost = function(args, context) {
    var id = String((args && args.id) || "");
    if (!id) return { ok: false, reason: "empty" };

    var sk  = (args && args.sessionKey) ? String(args.sessionKey) : "";
    var pid = sk ? webLookupSession(sk, false) : currentPlayerId;
    if (!pid) return { ok: false, reason: sk ? "expired" : "read_only" };

    var rec = null;
    try {
        var g = server.GetSharedGroupData({ SharedGroupId: BOARD_GROUP_ID, Keys: [id] });
        if (g && g.Data && g.Data[id] && g.Data[id].Value) rec = JSON.parse(g.Data[id].Value);
    } catch (e) { return { ok: false, reason: "no_group" }; }
    if (!rec)          return { ok: true, already: true };   // 이미 없음(링버퍼에서 밀렸을 수 있음)
    if (rec.p !== pid) return { ok: false, reason: "not_owner" };

    var del = {}; del[id] = null;
    try {
        server.UpdateSharedGroupData({ SharedGroupId: BOARD_GROUP_ID, Data: del, Permission: "Public" });
    } catch (e) { return { ok: false, reason: "no_group" }; }
    return { ok: true };
};

// 관리자 전용 — 신고된 글 삭제. adminKey = Title INTERNAL Data "admin_key" (소감 관리자 게이트 재사용).
//  Game Manager → Automation → CloudScript → Run Revision
//    FunctionName: adminDeleteBoardPost, FunctionParameter: { "adminKey":"<키>", "id":"<글id>" }
//  id 생략 시 전체 삭제(그룹은 유지).
handlers.adminDeleteBoardPost = function(args, context) {
    if (!awardAdminAuthorized(args)) return { ok: false, reason: "denied" };
    var id = String((args && args.id) || "");

    var del = {};
    var n   = 0;
    if (id) {
        del[id] = null; n = 1;
    } else {
        try {
            var g = server.GetSharedGroupData({ SharedGroupId: BOARD_GROUP_ID });
            if (!g || !g.Data) return { ok: true, deleted: 0 };
            for (var k in g.Data) { del[k] = null; n++; }
        } catch (e) { return { ok: false, reason: "no_group" }; }
    }
    if (n === 0) return { ok: true, deleted: 0 };

    try {
        server.UpdateSharedGroupData({ SharedGroupId: BOARD_GROUP_ID, Data: del, Permission: "Public" });
    } catch (e) { return { ok: false, reason: "no_group" }; }
    return { ok: true, deleted: n };
};

// 관리자 전용 — 특정 플레이어의 웹 세션 강제 무효화 (계정 도용 신고 대응).
//  FunctionParameter: { "adminKey":"<키>", "playFabId":"<pid>" }
handlers.revokeWebSession = function(args, context) {
    if (!awardAdminAuthorized(args)) return { ok: false, reason: "denied" };
    var pid = String((args && args.playFabId) || "");
    if (!pid) return { ok: false, reason: "empty" };
    try {
        server.UpdateUserInternalData({ PlayFabId: pid, KeysToRemove: ["web_session"] });
    } catch (e) { return { ok: false, reason: "failed", detail: String(e) }; }
    return { ok: true };
};
