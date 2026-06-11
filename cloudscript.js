handlers.deleteExpiredEntries = function(args, context) {
    var EXPIRE_MS   = 14 * 24 * 60 * 60 * 1000;
    var EXPIRED_VAL = 3599590;
    var now         = Date.now();
    var deleted     = 0;

    for (var lv = 3; lv < 10; lv++) {
        var pad      = lv < 10 ? "0" : "";
        var statName = "BestTime_L" + pad + lv;

        var startPos = 0;
        while (true) {
            var lb = server.GetLeaderboard({
                StatisticName:      statName,
                StartPosition:      startPos,
                MaxResultsCount:    100,
                ProfileConstraints: { ShowLastLogin: true }  // 별도 API 호출 없이 LastLogin 포함
            });
            if (!lb || !lb.Leaderboard || lb.Leaderboard.length === 0) break;

            for (var i = 0; i < lb.Leaderboard.length; i++) {
                var entry = lb.Leaderboard[i];
                if (entry.StatValue <= 0 || entry.StatValue >= EXPIRED_VAL) continue;

                var lastLoginMs = 0;
                if (entry.Profile && entry.Profile.LastLogin) {
                    lastLoginMs = new Date(entry.Profile.LastLogin).getTime();
                }

                if (lastLoginMs === 0 || (now - lastLoginMs) > EXPIRE_MS) {
                    server.UpdatePlayerStatistics({
                        PlayFabId:  entry.PlayFabId,
                        Statistics: [{ StatisticName: statName, Value: EXPIRED_VAL }]
                    });
                    deleted++;
                }
            }

            if (lb.Leaderboard.length < 100) break;
            startPos += 100;
        }
    }

    return { deleted: deleted, checkedAt: now };
};

// 전체 랭킹 초기화 - 모든 플레이어의 BestTime 통계를 0으로 리셋
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
