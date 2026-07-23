# 리플레이 좋아요(👍) 기능 기획서

작성: 2026-07-23 · 상태: **기획(미착수)**

관전(WATCH/다시보기)에서 본 리플레이에 다른 플레이어가 👍 좋아요를 누르면,
해당 리플레이 주인의 누적 좋아요 수가 올라가고 랭킹보드에 `👍3` 형태로 표시된다.

---

## 1. 결론 — 구현 가능성

**전면 구현 가능.** 새로운 서버 개념이 전혀 필요 없다.
기존 **수상소감(award_comment)** · **격파낙인(battle_reward)** 이 이미 확립한 3중 패턴
— ① Shared Group `X_L%02d`(key=playFabId) 저장 ② CloudScript로 서버 검증 후 write
③ 랭킹보드에서 playFabId 조인 표시 + `maintainLeaderboards` GC —
을 거의 그대로 복제하면 된다.

한 가지 구조적 차이만 있다: **소감/낙인은 "내가 내 것을 쓴다"지만, 좋아요는 "내가 남의 카운터를 올린다".**
→ ⓐ 중복 방지(한 사람이 한 리플레이에 1회만) ⓑ 자기 리플레이 좋아요 금지 두 게이트가 서버에 추가로 필요.
둘 다 CloudScript에서 `currentPlayerId`(=투표자) 기준으로 처리 가능하므로 문제없다.

**재사용 자산**
- 저장/조회 인프라: `LeaderboardManager`의 Shared Group 부트스트랩·fetch·1h 캐시·조인 패턴 (`fetchBattles`/`fetchComments` 미러)
- 서버: `cloudscript.js`의 `battleRankOf`(Top10 검증), `awardEnabled` 마스터 스위치, `maintainLeaderboards` GC 루프
- 표시: `MainScene_Rank.cpp` drawOnlineRank의 낙인 마커 조인 렌더(💥/🗡 옆에 붙이던 자리)
- 진입점: 관전 종료 팝업 `showSpectateEndPopup`(PlayScene_Replay.cpp) — 여기에 버튼 한 개 추가

---

## 2. 핵심 설계 결정

| # | 결정 | 값(권장) | 근거 |
|---|------|----------|------|
| **D1** | 좋아요 집계 단위 | **레벨별** (`likes_L%02d`) | 리플레이·리더보드가 전부 레벨별. 조인 메커니즘이 레벨별 그룹 전제. "이 레벨 리플레이가 받은 좋아요"가 자연스러움 |
| **D2** | 좋아요 표시 자리 | **수상소감 우측**에 `👍N` (N≥1만; 소감 없으면 이름 우측). 공간 부족 시 **소감을 줄여** 좋아요 자리 확보 | 유저 확정(2026-07-23 실물 보고 변경). 라벨 폭 선측정→소감 truncate에 예약 |
| **D2c** | 큰 수 축약 표기 | N≥1000 → `X.YK` (소수 1자리, **반올림 없이 버림**). 예: 1267→`1.2K`, 1000→`1.0K` | 폭 절약. 정수 연산만 써 부동소수 반올림 배제 |
| **D2b** | 좋아요를 누르는 위치 | **관전 종료 팝업**(showSpectateEndPopup)에 `👍 LIKE` 버튼 | 다 본 뒤 누르는 게 맥락상 맞음. REPLAY/HOME 옆 3버튼. WATCH 카드(관전 전)는 부자연 |
| **D3** | 카운터 저장 | Shared Group `likes_L%02d`, key=대상 playFabId, value=JSON `{n:정수, v:{투표자id:1}}` | 낙인/소감과 동일 조인 표시 + **dedup 동봉**(v). GC가 키 삭제 시 카운터·투표자 함께 초기화 |
| **D4** | 중복 방지(1인 1리플레이 1회) | **대상 키 안 투표자 집합** `v`에 기록 (D3와 동일 키) | D3-GC(순위밖→0)와 정합: 카운터+투표자를 한 키에서 통째 리셋 → 재진입 시 과거 투표자 재투표 가능. 단일 키 read-modify-write라 구현도 단순 |
| **D5** | 좋아요 취소 | **1차 미지원(누르면 끝)** | 단순. 토글은 백로그. 취소 지원 시 `v`에서 제거 + `n`−1로 확장 가능 |
| **D6** | 자기 리플레이 | 좋아요 **불가** (targetId===currentPlayerId 거부) | 자화자찬 방지 |
| **D7** | 마스터 스위치 | 공개 Title Data `like_enabled` (fail-open) | award/battle 패턴 동일. 재배포 없이 on/off. 서버 게이트 + **클라 게이트(fetchTitleConfig→isLikeEnabled)로 UI(버튼/카운트) 숨김**(2026-07-23 추가) |
| **D8** | 빌드 토글 | `common_define.h` `#define ENABLE_REPLAY_LIKE` | award_comment처럼 한 줄로 전체 포함/제외 |

### 확정된 결정 (2026-07-23 유저)
- **D1 = 레벨별** (`likes_L%02d`). 글로벌 폐기.
- **D3-GC = 순위 밖으로 밀리면 0 초기화**(GC가 키 삭제). 근거: 재진입 시 옛 좋아요가 되살아나면 안 됨.
  - **신기록 갱신·Top10 내 순위 변동 → 좋아요 유지**(리플레이 내용은 바뀌어도 초기화는 손해 경험). 근거: 그 플레이어는 여전히 Top10=keepSet에 있으므로 **기존 GC(keepSet만 남김) 그대로 자동 충족** — 별도 로직 불필요.
  - ⚠️ **이 결정이 dedup 저장 위치를 D4로 확정**한다(아래): 카운터를 GC로 0 초기화하려면, 투표자 기록도 함께 지워져야 재진입 시 과거 투표자가 다시 누를 수 있다 → **dedup을 대상 키에 동봉**(투표자 개인 데이터에 두면 GC가 못 지워 재진입해도 재투표 불가 → 카운터가 0에 갇힘).

---

## 3. 데이터 모델

### 좋아요 카운터 + 중복방지 (단일 키)
```
Shared Group  likes_L%02d   (L03~L10, Permission=Public)
  key   = 대상 playFabId (리플레이 주인)
  value = JSON {
    "n": 12,                       // 누적 좋아요 수 (표시용)
    "v": { "투표자id": 1, ... }     // 이미 좋아요한 투표자 집합 (중복 방지)
  }
```
- **표시**: 클라가 `GetSharedGroupData(likes_L%02d)` 직접 조회(1h 캐시) → drawOnlineRank에서 대상 playFabId로 `n` 조인. 랭킹 이탈 시 조인 안 되어 자동 숨김(격파낙인·소감과 동일). (`v`는 표시에 안 씀, 무시)
- **중복 판정**: CloudScript `likeReplay`가 대상 키를 읽어 `v[voter]` 있으면 already(무증가), 없으면 `v[voter]=1` + `n++` 후 한 번에 write.
- **GC 정합(핵심)**: D3-GC로 순위 밖 대상 키가 삭제되면 `n`과 `v`가 **함께** 사라짐 → 재진입 시 카운터 0 + 과거 투표자도 다시 누를 수 있는 진짜 새 출발. (dedup을 투표자 데이터에 뒀다면 GC가 못 지워 재투표 불가 = 카운터 0에 갇힘 → 그래서 대상 키 동봉이 필수)

> **payload**: `v`가 투표자 수만큼 자람(id 16자 × 인원). 이 게임 규모(Top10 × 소수 좋아요)에선 수 KB 이내로 무시 가능. 특정 리플레이가 폭발적으로 받으면 커질 수 있음 → 필요 시 상한 도입(예: `v` 크기 N 초과하면 `n`만 증가시키고 신규 투표자 dedup 생략) 검토. 1차는 상한 없이 진행.
> **동시성**: 서로 다른 두 투표자가 같은 대상을 동시에 like → read-modify-write 경합으로 +1/dedup 유실 가능. 좋아요는 저위험 지표라 수용(격파낙인과 동일 철학).

---

## 4. 서버 (cloudscript.js) — 신규 핸들러

`writeReplay`/`recordBattle` 바로 아래에 좋아요 섹션 추가. 기존 헬퍼(`replayStatName`, `battleRankOf`, `codePointLength`) 재사용.

```js
var LIKE_MIN_LEVEL = 3, LIKE_MAX_LEVEL = 10;
function likeGroupId(level){ var p=(level<10?"0":""); return "likes_L"+p+level; }
function likeEnabled(){ /* Title Data "like_enabled", award_enabled 패턴 fail-open */ }

// 좋아요 누르기 — 투표자(currentPlayerId)가 targetId 리플레이에 +1
//  클라: ExecuteCloudScript { FunctionName:"likeReplay",
//                             FunctionParameter:{ level:int, targetId:string } }
//  반환: { ok:true, n:새카운트, already:false } | { ok:false, reason:... }
handlers.likeReplay = function(args, context){
    var voter  = currentPlayerId;
    var level  = parseInt(args && args.level, 10);
    var target = String((args && args.targetId) || "");
    // 게이트: 레벨 범위 / 빈 대상 / 자기자신(target===voter 거부, D6) / 마스터 스위치(D7)
    // (선택) 대상이 실제 Top10인지 battleRankOf로 재검증 — 조인이 자연필터하므로 생략 가능

    // 1) 대상 키 read → obj = { n, v } (없으면 {n:0, v:{}})
    // 2) v[voter] 있으면 → { ok:true, already:true, n:obj.n }  (증가 없음)
    // 3) 없으면 v[voter]=1; n++; 대상 키 하나만 UpdateSharedGroupData write
    // 4) 반환 { ok:true, already:false, n:obj.n }
};

// (백로그 D5) unlikeReplay — 취소 지원 시. 대상 키에서 v[voter] 삭제 + n−1(0 하한)
```

### GC (maintainLeaderboards) — 좋아요 정리 블록 **추가 필요** (D3-GC 확정)
소감/낙인 정리 블록을 복제해 `likes_L%02d`에서 `keepSet`(현재 Top10) 밖 키 삭제.
→ 순위 밖 대상의 `{n,v}` 통째 삭제 = "순위 밖→0 초기화". Top10 유지 대상은 keepSet 포함 → 안 지워짐 = "신기록/순위변동 시 유지" 자동 충족.
- **API 예산**: 현재 레벨당 리더보드1 + 그룹3(replay/award/battle) get(+조건부 update). 좋아요 추가로 그룹4 → get 1콜 더(삭제 있으면 +1). 한 실행에 8레벨 전부는 예산(23) 초과 가능하나 **시각 기반 로테이션으로 순환 정리 + idempotent**라 안전(기존 설계 그대로).

---

## 5. 클라이언트

### LeaderboardManager (.h/.cpp) — battle/award 미러
- `static constexpr int LIKE_MAX_LEVEL = 10;`  `static std::string likeGroupId(int level);`
- `void bootstrapLikeGroups(bool force=false);` — 로그인 시 CreateSharedGroup×8 (award/replay/battle 부트스트랩 옆에)
- `void likeReplay(int level, const std::string& targetId, std::function<void(bool ok, int newCount, bool already)> cb);`
  → ExecuteCloudScript `likeReplay`. no_group 시 부트스트랩+1회 재시도(doWriteAwardComment 패턴).
- `void fetchLikes(int level, std::function<void(const std::map<std::string,int>&)> cb);` — GetSharedGroupData+1h 캐시 (fetchBattles 미러). `void invalidateLikes(int level);`
- 캐시 구조체 `LikeCacheEntry{ std::map<std::string,int> byId; std::time_t cachedAt; }`
- `LeaderboardEntry`에 `int likes = 0;` 필드 추가(조인 결과 담기, `#ifdef ENABLE_REPLAY_LIKE`).
- RESET ALL(clearAllCaches)에 좋아요 캐시 클리어 추가.

### PlayScene (관전 씬) — 대상 식별자 전달
- `createSpectateScene`에 `const std::string& targetId=""` 파라미터 추가 → `m_spectatePlayFabId` 저장.
  (RACE의 createRaceScene은 이미 pid를 받음 — 동일 패턴)
- `showAwardCardDialog`의 WATCH 버튼 콜백에서 `pid` 넘기도록 한 줄 수정.

### PlayScene_Replay.cpp — 좋아요 버튼
- `showSpectateEndPopup`: 버튼을 **REPLAY / 👍 LIKE / HOME 3개**로. (또는 REPLAY/HOME 유지 + LIKE를 헤드라인 아래 별도 행)
  - 이미 좋아요했으면(응답 already=true) 하트 채워진 상태 `👍 12`로 비활성/체크 표시.
  - 누르면 `likeReplay(m_spectateLevel, m_spectatePlayFabId, cb)` → 성공 시 버튼 라벨 `👍 N`으로 갱신 + 효과음(efs_move_disc_ok 등) + 살짝 스케일 팝.
  - 자기 리플레이(m_spectatePlayFabId==내id)면 버튼 자체를 숨김.
- 관전 진입 시 현재 카운트를 보여주고 싶으면 `fetchLikes`로 프리로드(선택).

### MainScene_Rank.cpp — 랭킹보드 표시 `👍N` (이름 우측, D2)
- drawOnlineRank에서 `fetchBattles`처럼 렌더 전 `fetchLikes(level, ...)`로 조인해 `entries[i].likes` 채우거나, fetchReplays ▶마커처럼 프리워밍 콜백으로 사후 부착.
- **위치 = 이름 라벨 우측**: 이름 라벨 폭(`nm->getContentSize().width`)을 재서 `55 + width + gap` 지점에 `👍N` 배치.
  - **N≥1일 때만** 표시(0은 생략), 폰트 `rowFont-2`, 색 앰버(255,190,80).
  - **표기 = `formatLikeCount(N)`** (D2c). 공유 유틸로 `common_define.h`에 추가(countryToFlag 옆, 드로우 아님·순수 유틸) → drawOnlineRank·관전팝업·카드 공용. 정수 버림:
    ```cpp
    // N<1000 → "1267"이 아니라 그대로 "999"; N≥1000 → "X.YK"(버림). 반올림 없음.
    static inline std::string formatLikeCount(int n) {
        if (n < 1000) return std::to_string(n);
        int tenths = n / 100;                 // 1267 → 12  (버림)
        return cocos2d::StringUtils::format("%d.%dK", tenths / 10, tenths % 10);  // 12 → "1.2K"
    }
    ```
    검산: 1267→`1.2K`, 1000→`1.0K`, 12345→`12.3K`, 999→`999`.
  - ⚠️ 이름 우측~시간 사이 구간은 **수상소감**도 쓰는 자리(현재 `nameRight = 55+nm.width`부터 시작). 좋아요 라벨을 먼저 놓고 소감 시작 x를 `👍N` 폭만큼 밀어야 겹침 없음. (소감 fit-or-truncate의 `availL` 보정)
  - 결과물 보고 폰트/여백 조율(유저).
- 카드(showAwardCardDialog / showSpectateEndPopup)에도 `👍N` 요약 표시(선택).

---

## 6. 어뷰징 / 보안
- **유일 writer = CloudScript**(투표자 세션으로 실행). 클라가 그룹에 직접 write 불가(Public이라도 클라 UpdateSharedGroupData는 자기 인증 필요 — 카운터 조작 차단).
- **중복 방지**는 서버가 대상 키의 `v`(투표자 집합) 서버측 검사로 강제 → 스팸 무의미.
- **자기 좋아요 차단**(D6) 서버 게이트.
- **마스터 스위치**(D7) + 서버 이중 방어(likeEnabled OFF 시 write 거부).
- 좋아요는 점수/랭킹에 영향 없음(순수 표시 지표) → 조작돼도 리더보드 무결성 훼손 없음. 위험도 낮음.

## 7. 빌드
- 전부 기존 파일에만 추가 → **신규 .cpp 없음 = Android/iOS 빌드설정·Xcode 수동추가 불필요**(award_comment와 동일).
- `#define ENABLE_REPLAY_LIKE` 가드로 전체 on/off. OFF 시 조인·버튼·필드 전부 컴파일 제외.
- 서버 게이트는 cloudscript.js 재배포 후 활성(미배포 시 클라 `CloudScriptNotFound` → 버튼 무동작, 크래시 없음).

---

## 8. 페이즈 로드맵
1. **1차 — 서버**: ✅ **코드 완료(2026-07-23)** — cloudscript.js `likeReplay` + `likeGroupId`/`likeEnabled` + maintainLeaderboards 좋아요 GC 블록. ⬜ **PlayFab 재배포 + `like_enabled` Title Data 키 생성 + ExecuteCloudScript 실기 테스트(중복/자기좋아요/카운트/no_group)** — 유저.
   - ⚠️ 테스트 전제: `likes_L%02d` Shared Group이 존재해야 함(없으면 no_group). 2차 클라 `bootstrapLikeGroups`가 로그인 시 생성. 서버 단독 테스트는 Game Manager에서 그룹 수동 생성 또는 2차 이후 진행.
2. **2차 — 클라 코어**: ✅ **코드 완료(2026-07-23)** — common_define(`ENABLE_REPLAY_LIKE`+`formatLikeCount`), LeaderboardManager(likeGroupId/bootstrapLikeGroups/likeReplay+doLikeReplay/fetchLikes/invalidateLikes/LikeCacheEntry/`LeaderboardEntry.likes`/login부트스트랩/clearAllCaches), PlayScene.h(createSpectateScene `targetId`+`m_spectatePlayFabId`), MainScene_Dialog(WATCH가 pid 전달), PlayScene_Replay(showSpectateEndPopup `👍 LIKE` 버튼 — 3열, 프리로드 카운트, 자기리플레이 숨김, 재탭 잠금). ⬜ **유저 빌드 검증.**
3. **3차 — 표시**: ✅ **완료·실물 검증(2026-07-23)** — MainScene_Rank buildRows 렌더 루프에 `👍N`(앰버, rowFont-2, N≥1만)을 **수상소감 우측**(소감 없으면 이름 우측)에 배치, 공간 부족 시 소감 축소(lkReserve 예약). 두 경로(재진입[A]/첫진입[B]) fetchBattles 콜백 안에 fetchLikes 중첩 조인(`entriesCopy[i].likes` 채움 후 buildRows). 실기 확인 완료. (카드 표시는 선택, 미구현)

> **전체 완료(2026-07-23).** 1차 서버 배포 + 2·3차 클라 빌드·실물 검증 완료. 미커밋.

### 후속 추가 (2026-07-23, 유저 요청)
- **like_enabled 클라 게이트**: `fetchTitleConfig`에 `like_enabled` 키 추가 → `m_likeEnabled`/`isLikeEnabled()`(award_enabled 미러, stale-while-revalidate `cfg_like_enabled`). OFF 시 클라 UI 숨김 3곳: 관전 LIKE 버튼(canLike), 보드 `👍N`(lkLabel 생성 게이트), 카드 `👍N`. 서버(likeReplay disabled)+클라 이중.
- **카드 `👍N`**: `showAwardCardDialog`(관전=수상 통합 카드) 헤더 이름 우측에 비동기 `fetchLikes`로 `👍N` 부착(N≥1, 활성 시). ⬜ 빌드 검증.
4. **백로그**: 좋아요 취소 토글(D5), `v` payload 상한(폭발적 인기 대비), "가장 사랑받은 리플레이" 정렬/뱃지.

## 9. 관련 문서·메모리
- `docs/replay_ghost_plan.md` (리플레이/관전 인프라), `docs/battle_reward_plan.md` (Shared Group+CloudScript 미러 원형)
- 메모리: `project_replay_ghost`, `project_award_comment`, `project_battle_reward`, `project_leaderboard_position_inversion`
