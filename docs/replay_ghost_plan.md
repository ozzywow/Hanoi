# 리플레이 & 고스트 대결 기획안

> Tower of Hanoi Olympic — 터치노트 기록/재생 및 랭커 고스트 대결 기능
> 작성일: 2026-07-09 · 상태: 기획 (구현 전)

---

## 1. 개요

플레이어의 디스크 이동 시퀀스를 **타임스탬프와 함께 기록**하여, 판을 그대로 **리플레이 재생**한다. 나아가 랭커의 기록을 PlayFab에 공개 저장하여 다른 플레이어가 **관전**하거나 **고스트로 대결**할 수 있게 확장한다.

기록 갱신(타임어택) 게임이므로 **타이밍 재현이 설계의 중심**이다. 리플레이는 리더보드 기록과 바이트 단위로 정합해야 하며, 이 정합성 검증은 그대로 **부정 점수 방지(anti-cheat)** 수단이 된다.

### 목표
- **관전 가치**: 랭커의 리듬(망설임/폭발 구간)이 그대로 보이는 실시간 재생
- **경쟁 유발**: 랭커 고스트와 같은 시간축에서 대결 → 리텐션 강화
- **무결성**: 리플레이 = 기록의 증명서. 서버에서 합법성 + 시간 정합 검증

### 비목표 (이번 범위 아님)
- 마우스/터치 궤적 좌표 저장 (하노이는 from/to만으로 이동이 100% 결정됨 → 불필요)
- 15디스크 이상 대용량 리플레이 (2차 과제: 청킹/압축)

---

## 2. 핵심 설계 원칙 — 절대 타임스탬프

각 이동을 **시작 시점(`m_dateTime`) 기준 절대 경과 ms**로 저장한다. delta(간격) 방식이 아니다.

**이유:**
1. **드리프트 0** — delta 누적 시 반올림 오차가 쌓여 `리플레이 합계 ≠ 리더보드 기록`이 될 수 있음. 절대값은 마지막 이동의 `t_ms == m_mastTime`이 항상 성립.
2. **오버플로 없음** — delta uint16(최대 65초)는 플레이어가 멈추면 초과. 절대 ms(uint24/32)는 안전.
3. **재생 스케줄 그대로 사용** — `playbackClock`이 t_ms에 도달할 때 이동 실행. 별도 변환 불필요.
4. **검증 용이** — 마지막 t_ms를 제출 스코어와 직접 비교 가능.

delta가 필요하면 재생 시 `t[i] - t[i-1]`로 즉석 계산한다.

---

## 3. 데이터 포맷

### 3.1 이동 레코드

```cpp
struct ReplayMove {
    uint8_t  move;   // (from << 4) | to   — 폴 0~2, 상위 니블 여유
    uint32_t t_ms;   // m_dateTime 기준 경과 밀리초
};   // 논리 5바이트 (직렬화 시 uint24로 t_ms 압축하면 4바이트)
```

- `from` = `pDiscus->GetCurrPoleID()` (이동 전), `to` = `poleID`
- 디스크 ID는 **저장 안 함** — 목적지 폴의 top이 곧 이동 대상이므로 재생 시 복원 가능
- `t_ms` = `getMilliCount() - m_dateTime` (녹화 시점)

### 3.2 리플레이 헤더

```cpp
struct ReplayHeader {
    uint16_t version;      // 포맷 버전 (하위호환)
    uint8_t  discusCount;  // 디스크 수 (레벨)
    uint8_t  targetPole;   // 완성 목표 폴
    uint32_t finalTimeMs;  // == m_mastTime (== 마지막 move.t_ms)
    uint16_t moveCount;    // 이동 수
    // 이후 moveCount 개의 ReplayMove
};
```

### 3.3 직렬화 / 전송 포맷
- 바이너리 → **Base64** 문자열로 PlayFab 저장 (JSON/UserData 값은 문자열)
- 연속 이동 구간이 많으므로 필요 시 RLE/varint로 추가 압축 (2차)

---

## 4. 크기 분석

| 디스크 | 최소 이동 | 랭커 추정 이동 | 원시(5B/move) | Base64 | PlayFab UserData(1000자) |
|:---:|:---:|:---:|:---:|:---:|:---:|
| 5 | 31 | ~40 | ~200B | ~270B | ✅ 여유 |
| 7 | 127 | ~160 | ~800B | ~1.1KB | ⚠️ 청크 1개 |
| 10 | 1023 | ~1300 | ~6.5KB | ~8.7KB | ⚠️ 청크 분할 |
| 15 | 32767 | ~40000 | ~200KB | ~270KB | ❌ 별도 저장소 |

- **5디스크는 완전히 무시 가능한 크기.** 우선 지원 구간.
- PlayFab UserData 값은 키당 기본 1000자 제한 → 7디스크 이상은 **여러 키로 청킹**하거나 압축.
- 15디스크는 이번 범위 밖 (2차).

---

## 5. 저장 / 전송 (PlayFab)

### 5.1 저장 위치
- 리더보드 엔트리에는 직접 첨부 불가 → **플레이어별 UserData**에 저장
- 키 예시: `replay_L05` = Base64 리플레이 (레벨별 자기 최고기록 1건)
- 큰 리플레이는 `replay_L10_0`, `replay_L10_1` … 청크 분할

### 5.2 공개 조회 (랭커 리플레이 관전)
- `GetUserData`는 기본 본인만 → **CloudScript(Server API)로 대상 랭커 조회**
- 기존 `award_comment`의 **playFabId 조인 + CloudScript 필터 패턴 재활용** (`[[project_award_comment]]`)
- 흐름: 리더보드에서 랭커 선택 → playFabId 확보 → CloudScript로 해당 유저 `replay_Lxx` 반환 → 클라 재생

### 5.3 업로드 시점
- 신기록 달성 시([PlayScene.cpp:616](../Classes/PlayScene.cpp#L616) `SetBestRecord` 부근)에만 리플레이 업로드
- 기존 스코어 제출([PlayScene.cpp:621](../Classes/PlayScene.cpp#L621) `submitScore`)과 함께 처리

---

## 6. 코드 통합 지점

### 6.1 녹화 (Recording)
- **단일 훅 포인트**: [AttachDiscusToPole()](../Classes/PlayScene.cpp#L992), `++m_moveCount` 직전/직후 ([PlayScene.cpp:1015](../Classes/PlayScene.cpp#L1015))
  ```cpp
  // AttachDiscusToPole 내부, 이동 확정 후
  m_replay.push_back({ (uint8_t)((currPoleID << 4) | poleID),
                       (uint32_t)(getMilliCount() - m_dateTime) });
  ```
  → 모든 합법 이동이 이 한 곳을 통과하므로 누락 없음
- 게임 시작 시 초기화: [InitGame/Start 부근](../Classes/PlayScene.cpp#L539) `m_dateTime` 설정 지점에서 `m_replay.clear()`
- 완료 시 헤더 채우기: [Finished()](../Classes/PlayScene.cpp#L560) — `finalTimeMs = m_mastTime`, `targetPole` 확정

### 6.2 재생 엔진 (Playback) — 신규
게임 로직을 재실행하지 않고 **상태 재생**만 한다.
```cpp
// 매 프레임 (schedule)
playbackClock += dt * playbackSpeed;      // dt = Director::getDeltaTime()
while (i < moves.size() && moves[i].t_ms <= playbackClock) {
    applyReplayMove(moves[i]);            // 폴 상태 갱신 + 디스크 이동 애니메이션
    ++i;
}
m_labelTime->setString(formatTime(playbackClock));  // getRecordTime 재사용
```
- 타이머 표시: [getRecordTime()](../Classes/common_define.h#L156) 그대로 재사용 → 리더보드와 동일 포맷
- 마지막 이동에서 `playbackClock → m_mastTime` 정확히 수렴
- **연출/진실 분리**: 이동 간격이 매우 짧을 때(폭발 구간) 애니메이션이 못 따라가면, 타이머는 t_ms 진실 그대로 두고 애니메이션만 최소 표시시간으로 살짝 뒤따르게 함
- 재생 컨트롤(선택): 일시정지 / 배속(`playbackSpeed`) / 특정 이동 점프

### 6.3 상태 격리
- 재생 모드에서는 터치 입력 비활성(`m_touchHanderLayer` 무시), RPM/이퀄라이저는 t_ms 간격 기반으로 재구동 가능 (`[[project_rpm_equalizer]]`)

---

## 7. Anti-cheat 검증 (서버측, CloudScript)

리플레이 저장은 **기록 무결성 검증**을 겸한다. 스코어 제출 시 리플레이도 함께 보내 서버에서 검사:

1. **합법성**: 모든 이동이 하노이 규칙 준수 (큰 디스크가 작은 디스크 위에 못 감)
2. **완성**: 최종 상태가 `targetPole`에 전체 디스크 정렬
3. **시간 정합**: `마지막 move.t_ms == 제출 스코어(finalTimeMs)`
4. (선택) **하한**: 물리적으로 불가능한 초고속 이동 간격 필터

3개(+선택 4) 통과 시에만 리더보드 반영 → 가짜 API 스코어 차단. `BestTime_Lxx` 통계 집계 규칙은 변경 금지 (`[[project_record_expiry_playfab_safety]]`).

---

## 8. 단계별 로드맵 (3차 구조)

서버 검증(§7 anti-cheat)은 별도 단계가 아니라 각 차수에 녹아든다: 1차 불필요 → 2차 합법성 검증 시작 → 3차 순위가 바뀌므로 필수.

| 차수 | 기능 | 신규 작업 | 네트워크 | 검증 |
|:---:|:---|:---|:---:|:---:|
| **1차** ⭐ | **내 리플레이 보기** (최소 출시 단위) | 녹화 훅 + 재생 엔진 | 없음 | 불필요 |
| **2차** | **랭킹보드 연동 — 다른 유저 리플레이 공유/관전** | PlayFab 저장/조회 + CloudScript 조인 | 필요 | 합법성 검증 |
| **3차** | **고스트 대결 — 이기면 랭킹 빼앗기** | 오버레이 렌더 + 실시간 비교 HUD + 순위 교체 로직 | 필요 | **필수** |

**포맷을 한 번 확정하면 1→3차가 전부 같은 자료구조 위에 얹힌다.** 1차(로컬)만 제대로 만들면 나머지는 UI/네트워크 작업이지 재설계가 아님.

### 2차 — 랭킹보드 연동 (공유/관전)
- 랭킹보드에서 항목 선택 → playFabId로 해당 유저 `replay_Lxx` 조회 → 재생
- 남의 리플레이가 공개되므로 조회 시 **합법성(하노이 규칙) 검증** 시작

### 3차 — 고스트 대결 & 랭킹 쟁탈 상세
- 화면: 내 3폴 + 고스트 3폴(반투명 오버레이 또는 상단 미니뷰)
- 내 실제 경과시간에 맞춰 고스트 t_ms 재생 → "지금 랭커는 여기까지 왔다" 실시간 표시
- 같은 시점 비교(이동수/시간) → 앞섬/뒤처짐 HUD (레이싱 고스트 패턴 동일)
- **랭킹 쟁탈**: 고스트(대상 랭커)의 기록을 이기면 내 신기록으로 순위 교체
  - ⚠️ 순위가 실제로 바뀌므로 **서버 검증 필수** — 조작 리플레이/스코어로 랭킹 강탈 차단 (§7 3검증)
  - 결정 필요: 특정 랭커 지목 대결인지, 바로 위 순위 자동 매칭인지 (§9 D6)

---

## 9. 미해결 결정사항 / 리스크

| # | 항목 | 선택지 | 비고 |
|:---:|:---|:---|:---|
| D1 | ✅ 주 시나리오 우선순위 | **내 판 다시보기 (결정)** | 1단계=최소 출시 단위, 네트워크 0 |
| D2 | t_ms 직렬화 폭 | uint24(4.6시간) vs uint32 | 크기 vs 단순성 |
| D3 | 7~10디스크 저장 | UserData 청킹 vs 압축(RLE) vs 조건부 미지원 | 초기엔 5디스크만? |
| D4 | 검증 시점 | 클라 제출 시 필수 vs 관전 요청 시 지연 검증 | CloudScript 비용 |
| D5 | 재생 컨트롤 범위 | 단순 재생만 vs 배속/일시정지/점프 | 1차 스코프 |
| D6 | 3차 대결 상대 매칭 | 특정 랭커 지목 vs 바로 위 순위 자동 매칭 | 랭킹 쟁탈 UX |
| R1 | 폭발 구간 애니메이션 추월 | 연출/진실 분리로 완화 | §6.2 |
| R3 | 3차 랭킹 강탈 조작 | 서버 검증 필수(§7)로 차단 | 순위 실변동 |
| R2 | 포맷 버전 하위호환 | 헤더 version 필드로 관리 | §3.2 |

---

## 부록 — 참고 코드 위치

| 항목 | 위치 |
|:---|:---|
| 이동 확정(녹화 훅) | [AttachDiscusToPole](../Classes/PlayScene.cpp#L992) |
| 타이머 시작 | [PlayScene.cpp:539](../Classes/PlayScene.cpp#L539) `m_dateTime` |
| 기록 확정/제출 | [PlayScene.cpp:581-621](../Classes/PlayScene.cpp#L581) |
| 완료 처리 | [Finished()](../Classes/PlayScene.cpp#L560) |
| 시간 포맷 유틸 | [getRecordTime](../Classes/common_define.h#L156) |
| 폴 상태 | `m_pole` (`vector<vector<Discus*>>`), [PlayScene.h:27](../Classes/PlayScene.h#L27) |
