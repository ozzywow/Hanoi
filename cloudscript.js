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
