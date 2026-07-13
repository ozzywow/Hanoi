# 도전모드 격파/복수 기획안

> Tower of Hanoi Olympic — 고스트 대결 승패에 낙인·뱃지·복수 루프를 얹어 경쟁 극대화
> 작성일: 2026-07-10 · 상태: 기획 (구현 전)
> 선행: [replay_ghost_plan.md](replay_ghost_plan.md) (고스트 대결/리플레이 인프라)

---

## 1. 개요

현재 도전모드(고스트 대결)는 승패 팝업(YOU WIN/LOSE)만 띄우고 보상이 일반모드와 동일(랭킹 반영뿐)하다. 경쟁 동기가 약하다.

**재화 시스템은 도입하지 않는다** (소비처가 없어 무용). 대신 **프레스티지·낙인·연출**로 승자에겐 자랑거리를, 패자에겐 복수 동기를 준다.

세 축:
1. **격파 마크(뱃지)** — 마커는 **상대 국가 깃발**(`countryToFlag()` 재활용). 승자 A row엔 "🗡️ {꺾은 상대 깃발}" 전리품, 패자 B row엔 "💥 {이긴 상대 깃발}" 낙인.
2. **복수 훅(풀 방식)** — B가 다음 진입 시 "A에게 추월당함 → 복수" 원탭 재도전.
3. **K.O. 연출** — 승리 순간 고스트가 무너지는 극적 연출.

### 설계 대원칙
- **비파괴**: B의 소감(`t`)·기록을 절대 덮어쓰지 않는다. 격파는 완전히 분리된 데이터 레이어.
- **랭킹 연동 진실성**: 격파 마크는 "실제로 리더보드에서 B를 추월"했을 때만 부여. 레이스 승리만으로는 안 준다.
- **서버 검증**: 클라의 "내가 이겼다" 주장이 아니라, 신뢰 가능한 통계 상태를 서버가 재검증한 뒤 발동.
- **회복 가능**: 낙인은 영구가 아니다. 복수 성공 시 반전, 미활동 시 TTL 페이드.

---

## 2. 격파 성립 조건 (랭킹 연동)

레이스 승리(`m_mastTime < m_ghostScoreMs`, [PlayScene.cpp:710](../Classes/PlayScene.cpp#L710))는 **"이번 판에서 B의 옛 고스트를 이겼다"**일 뿐, **랭킹 추월과 다르다.** 격파는 아래를 **모두** 만족할 때만 성립한다:

1. **상향 도전**이어야 함 — 도전 시작 시 **B가 A보다 위 순위**(A 순위 > B 순위, 즉 `m_ghostRank` < A의 현재 순위).
2. A의 이번 레이스 기록이 **A의 신기록**이고 (기존 신기록 제출 흐름을 탐),
3. 제출 후 라이브 리더보드에서 **A의 순위 < B의 순위** (A가 실제로 위로 추월),
4. A ≠ B (자기 자신 대결 제외).

### 하향 도전은 무보상 (핵심 정책)
상위 랭커가 자기보다 **아래** 랭커에게 도전하는 것은 **허용**하지만, 승리해도 **아무 일도 일어나지 않는다** — 격파 마크 없음, 낙인 없음, 연출 없음(일반 WIN 팝업만). "빼앗기"는 위를 뒤집을 때만 성립하기 때문. 이래야 1위가 하위권을 사냥해 낙인을 남발하는 것이 원천 차단된다.

→ 조건 1·3을 **서버가 재검증**한다. 격파 기록은 클라가 임의로 호출하는 게 아니라, **신기록 제출 직후** 클라가 `recordBattle`를 호출하고 서버가 통계 상태로 진위를 판정한다.

### 왜 신기록 제출에 훅을 거나
레이스는 클라 로컬 재생이라 "이겼다"는 조작 가능하다. 그러나 `BestTime_Lxx`는 서버 Min 집계라 조작이 어렵다. 격파를 **신뢰 가능한 통계 상태**에 묶으면 anti-cheat가 공짜로 따라온다.

---

## 3. 데이터 모델

기존 소감/리플레이와 **동일 패턴**의 별도 Shared Group. 오염 격리.

```
Shared Group: "battles_L%02d"   (레벨별, awards_/replays_ 와 동형)
key   = 패자(B) playFabId
value = JSON {
  by:       "A_playFabId",   // 격파자 (이름은 저장 안 함 — 렌더 시 id→name 맵 해소)
  aRank:    3,               // 격파 시점 A 순위 (0-indexed)
  bRankWas: 2,               // 격파당한 B의 직전 순위 (= A가 도전한 m_ghostRank)
  ts:       1720598400000    // epochMs (TTL·최신성 판정)
}
Permission: Public
```

> **이름은 저장하지 않는다.** A/B 모두 Top10이므로 리더보드 렌더 시 만든 `id→name` 맵으로 양쪽 이름을 해소한다. 캐시(byName)를 두지 않아 stale 이름 문제도 없음. (트레이드오프: 대상이 그 순간 로드 셋 밖이면 이름 공백 — Top10 한정이라 실질 영향 없음.)

### 조회 (클라, CloudScript 불필요)
- **B의 피격 마크**: `battles_L%02d`에서 B키 직접 조회.
- **A의 격파 마크**: 그룹 전체(Top10이라 ≤10개)를 스캔해 `by == A`인 항목 검색. 역인덱스 불필요.
- 리더보드 그리는 시점에 `GetSharedGroupData(battles_L%02d)` 1회로 전체 로드 → 각 row 매칭.

### 정합성
- **1인 1피격**: B키는 1개. 새 격파자가 덮어씀(가장 최근 격파자만 낙인). 파밍 스택 없음.
- B의 소감(`awards_L%02d[B].t`)과 **완전 분리** — 서로 건드리지 않음.

---

## 4. CloudScript 핸들러

기존 `writeReplay`/`writeAwardComment` 컨벤션 준수 (currentPlayerId, Public, no_group 반환 등).

### 4.1 `recordBattle` — 격파 기록 (신규)

```
클라 호출: ExecuteCloudScript {
  FunctionName: "recordBattle",
  FunctionParameter: { level:int, loserId:string, bRankWas:int }
}
반환: { ok:true, level } | { ok:false, reason:"..." }
```

서버 검증 순서:
1. `level` 범위 (3~10), `loserId` 비어있지 않음, `loserId !== currentPlayerId` (자기대결 차단).
2. A(currentPlayerId) 순위 조회: `GetLeaderboardAroundUser(BestTime_Lxx, A, 1)` → `aPos`, `aVal`.
3. B(loserId) 순위 조회: 동일 → `bPos`, `bVal`.
4. **추월 게이트**: `aPos >= 0 && bPos >= 0 && aPos < bPos` (A가 실제로 B 위). 실패 시 `reason:"not_passed"`.
5. **상향 게이트(하향 도전 차단)**: `bPos > bRankWas` — A가 B를 뚫고 올라오면 B는 한 칸 이상 밀려난다(`bPos > 도전 시점 B 순위`). 하향 도전(A가 이미 B 위)이면 A가 개선해도 B 순위는 그대로라 `bPos == bRankWas` → **거부** `reason:"not_upward"`. `bRankWas`는 클라가 도전한 `m_ghostRank`.
6. `battles_L%02d`에 key=`loserId` 로 `{by:A, aRank:aPos, bRankWas, ts}` 저장.

> **클라 1차 차단**: 정상 클라는 애초에 **상향 도전(도전 시작 시 A 순위 > `m_ghostRank`)일 때만** `recordBattle`를 호출한다. 하향 도전은 호출 자체를 안 함 → 정상 유저 전원 정확. 서버 게이트(5)는 변조 클라 방어용.

#### 잔여 악용 & 수용 (문서화)
- `bRankWas`는 클라 값이라, 변조 클라가 하향 승리를 "상향"으로 위장할 여지가 있음(bRankWas를 낮게 조작). 그러나:
  - **추월 게이트(4)는 서버 통계라 우회 불가** — A는 반드시 실제로 B 위여야 함.
  - 마크는 **비파괴·회복 가능·TTL(7일)·1인1피격** → 최악 피해가 작음.
  - 랭킹 히스토리를 서버에 영속하지 않는 한 100% 차단은 불가 → 과투자 대신 위 조건으로 **수용**.
- **레이트 리밋**: 동일 A→B 반복은 최신 1건만 남으므로 스팸해도 낙인 1개. 자연 억제.

### 4.2 `deleteBattle` — 낙인 제거 (신규, 본인=패자)

```
클라 호출: recordBattle 반전 또는 B의 명시적 제거
FunctionParameter: { level:int }
```
- key=currentPlayerId 삭제 (`data[pid]=null`). 복수 성공 시 자동 호출.
- 소감/리플레이의 `delete*` 와 동형.

### 4.3 `maintainLeaderboards` GC 통합 (기존 수정)

[cloudscript.js:102](../cloudscript.js#L102) 소감 정리 블록 옆에 **`battles_L%02d` 동일 정리** 추가:
- Top10 밖으로 밀려난 key(B) 삭제 → 고아 낙인 누적 방지.
- **추가**: 격파자 `by`가 Top10 밖이면 그 낙인도 무효(격파자가 사라졌는데 낙인만 남는 것 방지) — 스캔 시 `keepSet[record.by]` 도 확인.
- **TTL**: `now - ts > BATTLE_TTL_MS`(예: 7일) 인 낙인 삭제. 오래된 흉터 페이드.

### 4.4 이름/랭킹 초기화 시
기존 `deleteReplay`/`deleteAwardComment` 호출 지점에서 `deleteBattle`도 함께 호출(내 낙인 정리). 단, 내가 **격파자로 남긴** 타인 낙인은 GC가 처리.

---

## 5. 클라 훅 지점

### 5.1 격파 기록 발동 — 결과 팝업 흐름
[PlayScene.cpp:709](../Classes/PlayScene.cpp#L709) `m_isRace && raceWon` 분기에서, **신기록 제출 완료 콜백 이후**:
```
// isUpward = 도전 시작 시 A 순위 > m_ghostRank (B가 위였음). 하향 도전이면 호출 안 함.
if (m_isRace && raceWon && isNewRecord && m_ghostPlayFabId != myId && isUpward) {
    // 제출 전파(2초) 후 recordBattle 호출 — project_leaderboard_playfab_delay 패턴 재사용
    recordBattle(level, m_ghostPlayFabId, m_ghostRank) → ok면 K.O. 연출 강화
}
// 하향 도전 승리: 일반 WIN 팝업만, 마크/낙인/강화연출 없음.
```
> `isUpward` 판정에 필요한 "도전 시작 시 A 순위"는 레이스 진입 시점에 확보(리더보드에서 내 순위, 이미 조회 가능). ghost 순위는 `m_ghostRank`.
> ✅ **P0 실현성 확인됨**: 리더보드 엔트리는 이미 `e.playFabId`를 보유([LeaderboardManager.cpp:585](../Classes/LeaderboardManager.cpp#L585)). 레이스 실행 호출부([MainScene.cpp:1673](../Classes/MainScene.cpp#L1673))에서 그 엔트리가 손에 있으므로 owner id 접근 가능. 다만 `createRaceScene(lv, blob, nm, rnk, sms)` 시그니처엔 없음 → **playFabId 파라미터 추가 + `m_ghostPlayFabId` 멤버**만 하면 됨. RETRY 경로([PlayScene.cpp:792](../Classes/PlayScene.cpp#L792))도 `gb/gn/gr/gs`처럼 함께 스레딩.

### 5.2 K.O. 연출 — 승리 팝업
[PlayScene.cpp:711](../Classes/PlayScene.cpp#L711) `raceWon` 시:
- 기존 👻 `m_raceGhostIcon`([PlayScene.cpp:1517](../Classes/PlayScene.cpp#L1517))을 팝업 직전 **파괴 연출**(흔들림→낙하→페이드, 기존 Action 조합).
- 팝업 타이틀 "YOU WIN!" 위/아래 "💥 %s K.O.!" 배너.
- 격파 성립(recordBattle ok) 시 "🗡️ 랭킹 강탈!" 추가 라인 + 강조 사운드.
- 사운드는 기존 `efs_*` 재사용.

### 5.3 소감 프리필 — 승자
A가 신기록 후 소감 작성 진입 시, 격파 성립이면 EditBox 기본값 **프리필**(수정 가능):
```
"%s를 짓밟고 올라왔다"  // m_ghostName
```
강제 아님 — 자랑은 본인 손으로 확정할 때 강하다. `writeAwardComment`는 그대로 사용.

### 5.4 복수 훅 (풀 방식) — 패자
B가 **랭킹보드/타이틀 진입** 시 자기 `battles_L%02d[B]` 조회:
- 있으면 배너/자기 row에 "💥 {A깃발} %s에게 추월당함 → [복수]" — 이름·깃발은 `by` id를 id→name/country 맵으로 해소.
- [복수] 탭 → 기존 `createRaceScene(level, A의 blob, A이름, A랭크, A기록)` 로 직행. 인프라 이미 존재.
- 복수 성공(B가 A를 재추월, recordBattle 대칭 성립) → B의 낙인 `deleteBattle` + A에게 낙인 이전(넥서시스 플립).
- **푸시 불필요** — 진입 시 pull. 인프라 갖춰지면 후속으로 푸시 얹기.

### 5.5 랭킹보드 뱃지 렌더 — MainScene

**마커 = 상대의 국가 깃발** (`countryToFlag()` 재활용, [[feedback_flag_emoji]]). 올림픽 테마의 "정복한 깃발" 은유. 격파자 row엔 **꺾은 상대의 깃발**(전리품), 피격자 row엔 **자신을 이긴 상대의 깃발**(낙인)을 붙인다.

리더보드 draw 시 `battles` 그룹 1회 로드 + 렌더 중 **`id→name` / `id→country` 맵** 구성(로드된 row들의 playFabId → 표시명·국가코드). A/B 모두 Top10이라 같은 리더보드 로드로 해소 — **깃발도 이름과 동일하게 레코드에 저장 안 함**. 단, 리더보드 조회에 `ProfileConstraints.ShowLocations`가 켜져 있어야 함(기존 깃발 렌더가 이미 사용 → [[feedback_playfab_profileconstraints]]).

각 row:
- `by==나`인 레코드 존재 → **격파 마커**: `🗡️ + 상대(key=B) 깃발`, 금색 톤. "그 나라를 꺾음".
- key==그 row 주인 → **피격 낙인**: `💥 + 격파자(by=A) 깃발`, 적색 톤. "그 나라에게 당함".
- 방향 구분용 소형 글리프(🗡️/💥) + 색상으로 격파/피격을 구분(깃발만으론 "이김/짐" 모호).
- 한 row가 격파+피격 동시 보유 가능(A가 B를 이겼지만 C에게 짐). LED 밀도상 **동시엔 피격 낙인 우선**(복수 동기 노출), 격파 전리품은 탭 상세로. 상세 UI는 구현 단계.
- 맵에 없으면(로드 셋 밖) 국가 미상 → 중립 아이콘 폴백.

---

## 6. 상수/설정

| 상수 | 값(초안) | 위치 |
|---|---|---|
| `BATTLE_MIN_LEVEL` / `MAX_LEVEL` | 3 / 10 | cloudscript + LeaderboardManager 일치 |
| `BATTLE_TTL_MS` | 7일 | cloudscript (TTL 페이드) |
| 마스터 스위치 | Title Data `battle_enabled` | `award_enabled` 패턴 재사용, fail-open |

---

## 7. 페이즈 분할

- **P0 (선행)**: 레이스 컨텍스트에 **B playFabId** 전파 (`createRaceScene` 시그니처 + 고스트 로드 경로). 이게 없으면 아무것도 못 함.
- **P1 (백엔드)**: `recordBattle`/`deleteBattle` 핸들러 + `maintainLeaderboards` GC/TTL 통합 + `battle_enabled` 스위치.
- **P2 (승자 경험)**: K.O. 연출 + 격파 성립 훅 + 소감 프리필.
- **P3 (뱃지)**: 랭킹보드 🗡️/⚔️ 렌더 (battles 그룹 로드/매칭).
- **P4 (복수 루프)**: 풀 방식 복수 배너/원탭 재도전 + 넥서시스 플립.

---

## 8. 미결/확인 필요

1. ~~B playFabId 확보 경로~~ — **확인 완료**: `e.playFabId` 존재, `createRaceScene` 시그니처 확장만 필요(§5.1).
2. ~~byName 저장~~ — **결정 완료**: 저장 안 함, 렌더 시 `id→name` 맵으로 해소(§3, §5.5).
3. ~~하향 마킹 악용~~ — **결정 완료**: 하향 도전은 무보상(마크 없음). 상향 게이트(`bPos > bRankWas`) + 클라 상향 조건으로 차단(§2, §4.1).
4. ~~LED 뱃지 표기~~ — **결정 완료**: 마커 = 상대 국가 깃발(`countryToFlag()` 재활용), 🗡️/💥 글리프+색상으로 격파/피격 구분, 깃발은 저장 안 하고 `id→country` 맵 해소(§5.5). 세부 배치만 구현 단계에서.
