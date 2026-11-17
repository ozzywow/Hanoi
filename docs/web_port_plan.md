# 웹 버전(PC 브라우저) 구현 계획서

> Tower of Hanoi - Speedrun — PC 브라우저에서 플레이 가능한 웹 버전 (B안: 웹 재구현)
> 작성일: 2026-11-17 · 상태: 계획 (구현 전) · **2026-11-17 범위 축소: 앱 설치 유도용 홍보 웹 (§0)**

---

## 0. 범위 조정 — 홍보용 최소 웹 (최우선, 아래와 충돌 시 이 절이 우선)

웹 사용자가 얼마나 될지 불확실하므로 **앱 설치를 유도하는 홍보용 체험판**으로 범위를 줄이고, 큰 비용(앱 연동, 서버 검증)은 **사용자 수를 확인한 뒤** 결정한다.

### 확정 사항
| 항목 | 결정 |
|---|---|
| 목적 | 앱 설치 유도 (PC에서 맛보기 → 스토어로) |
| 레벨 | Lv.3~4만 플레이. Lv.5+는 🔒 + 앱 설치 안내 (D3) |
| 신원 | **계정 없음**. 웹 기록은 브라우저 로컬에만 저장, 서버에 쓰지 않음 (D1 → S3 변형) |
| 랭킹 | **읽기 전용**. 앱 랭킹을 보여주고, 내 기록이 그 안에 들면 "앱 랭킹 TOP N급!" 문구로 설치 유도 |
| 앱 연동(S2) · 서버 검증 · 리플레이 · 고스트 · 좋아요 · 게시판 연동 | **보류** (§11은 보류 문서로 보존) |
| 서버 변경 | **없음**. 기존 CloudScript/PlayFab 그대로 |
| 치팅 문제 | 서버에 쓰지 않으므로 **발생하지 않음** (§5.4, §11.5 불필요) |

### 홍보 효과를 위한 핵심 장치
1. **결과 화면 CTA**: 기록 표시 직후 "앱에서 랭킹에 도전하세요" + 스토어 버튼(`SHARE_URL` 스마트 링크, iOS/Android 자동 분기)
2. **내 기록 vs 앱 랭킹 비교**: 읽기 전용 랭킹(상위 20, 앱의 `fetchLeaderboard` 윈도우 보정 로직 재사용)과 내 기록을 비교해 순위권이면 강조
3. **Lv.5+ 🔒**: 눌렀을 때 "전체 레벨은 앱에서" 안내
4. **랜딩 연결**: 랜딩 `landing/index.html`에 "PLAY ON WEB" 버튼, 공유 문구에도 웹 체험 링크

### 읽기 전용 랭킹 접속 방식
기존 랜딩이 이미 쓰는 **공용 뷰어 계정**(CustomId `web_public_viewer`)으로 `LoginWithCustomID` → `GetLeaderboard`만 호출. 방문자마다 계정이 생기지 않고, 쓰기 API는 호출하지 않는다.

### 사용자 수 측정 (투자 확대 판단 근거)
GitHub Pages는 **사이트 방문 통계를 제공하지 않는다**. (저장소 Insights → Traffic은 *저장소 페이지*의 조회수이며 Pages 사이트 방문과 무관.) 또한 정적 호스팅이라 서버 로컬 파일에 로그를 쓸 곳도 없다. 지표는 중요도가 낮으므로 **서버 코드 없이 되는 최소안**을 쓴다.

| 지표 | 수단 | 비용 |
|---|---|---|
| 방문자 수·국가·유입 경로 | **Cloudflare Web Analytics** — 스니펫 1줄(도메인 이전 불필요, 쿠키 없음, 무료) | 10분 |
| 게임 시작 / 완주 / 스토어 클릭 | **PlayFab 합계(Sum) 통계 3개** — 공용 뷰어 계정이 `UpdatePlayerStatistics`로 +1 (`web_start`, `web_finish`, `web_cta`) | 반나절 |

- PlayFab 합계 통계는 서버 코드(CloudScript) 변경 없이 Game Manager에서 통계 3개(집계 방식 **Sum**)를 만들면 된다. 증가는 원자적이라 동시 접속에도 누락이 없다. 값은 Game Manager에서 뷰어 계정 통계로 확인하거나, 필요하면 뷰어 계정으로 `GetPlayerStatistics`를 호출해 읽는다.
- 값은 누적 합계라 **일별 추이는 없다**. 필요하면 주 1회 값을 기록해 차이를 본다(§0 확대 게이트는 4~8주 단위라 충분).
- 한계: 누구나 값을 올릴 수 있으므로 **정확한 수치가 아닌 대략치**(새로고침/조작에 의한 부풀림 가능)로만 사용한다. 이 통계는 랭킹과 무관한 별도 통계이므로 `BestTime_Lxx`에 영향이 없다.
- ⚠️ 구현 시 확인: 해당 타이틀의 "클라이언트 통계 쓰기" 허용 상태(앱이 이미 클라에서 쓰므로 허용 상태), Sum 집계 설정.
- 더 간단히 가려면 Cloudflare만 쓰고 시작·완주·클릭은 포기해도 된다. 이 경우 확대 판단은 방문자 수와 앱 스토어 쪽 유입(`SHARE_URL` 경유 설치 추이)으로 한다.
- 개인정보: Cloudflare는 쿠키가 없고, PlayFab 합계는 개인 식별 정보가 없다. `landing/privacy.html`에 "익명 방문·이용 횟수를 집계한다" 문구만 추가한다.


### 진행 현황 (2026-11-17)
- ✅ Phase 0: 스캐폴딩 (Vite+TS+Canvas2D, 레터박스/DPR, 입력, 씬)
- ✅ Phase 1: 앱 PNG 리소스 기반 플레이 화면(상단 LED·좌측 도크·하단 LED·START·카운트다운·결과 팝업), 효과음·BGM·RPM 연동·이퀄라이저, 첫 방문 가이드(Lv.3)
- ✅ Phase 2': 읽기 전용 랭킹(상단 티커·시상대·내 기록 비교), 국기 PNG 폴백, 이용 지표 `track()`(PlayFab Sum 통계 — `web_cta` 정의 정리 중)
- ⏳ 남음: 랜딩 "PLAY ON WEB" 버튼·개인정보 문구(배포 시점에), gh-pages 시험 배포, 모바일 브라우저 점검
- 배포 계획 없음 — 구현 완료 후 결정 (`deploy.ps1` 은 준비됨)

### 축소된 일정
| 단계 | 내용 | 추정 |
|---|---|---|
| Phase 0 | 스캐폴딩, 캔버스/씬/입력, 배포 스크립트, 분석 삽입 | 2~3일 |
| Phase 1 | 플레이 코어(Lv.3~4), 효과음, 로컬 기록, 결과 팝업 + CTA, 🔒 | 약 1~1.5주 |
| Phase 2' | 읽기 전용 랭킹 + 내 기록 비교 + 랜딩 연결 | 약 3~4일 |
| **합계** | | **약 2~2.5주** (기존 MVP 2.5~3.5주 → 단축, 서버·계정·이름 입력 흐름이 모두 빠짐) |

Phase 3(스플래시/타이틀 연출, BGM)은 **최소화**: 스플래시와 START는 유지하되 BGM 플레이어·티커·다이얼로그는 만들지 않는다. BGM은 플레이 중 루프 1곡만.

### 확대 판단 게이트 (출시 후 4~8주 관찰)
| 관찰 결과 | 다음 행동 |
|---|---|
| 방문이 적고 스토어 클릭도 미미 | 유지만 하고 투자 중단 (서버 비용 0이라 부담 없음) |
| 스토어 클릭 전환이 의미 있음 | 이름 입력 + 웹 랭킹 등록(S1) 추가, 앱 홍보 문구 개선 |
| 웹 재방문·완주율 높음 | 그때 서버 검증 + 앱 연동(§11) 재검토 |

---

## 1. 개요

C++ 앱(Cocos2d-x 4.0)을 **웹 기술(TypeScript + Canvas 2D)로 다시 구현**해 PC 브라우저에서 플레이하게 한다. 서버(PlayFab + CloudScript)는 **그대로 재사용**하므로 앱과 웹이 **같은 랭킹**을 공유한다. 호스팅은 기존 GitHub Pages(`ozzywow.github.io/Hanoi`)를 쓴다.

### 목표
- PC에서 설치 없이 바로 플레이 (랜딩 → PLAY 한 번 클릭)
- 앱과 **동일한 랭킹보드**(레벨별 BestTime) 읽기/쓰기
- 앱의 룩앤필(LED 패널, 키캡, 픽셀 폰트, 네이비 톤) 유지
- 키보드/마우스 입력에 맞춘 조작

### 비목표 (이번 범위 아님)
- 인앱 결제(IAP) — 웹은 결제 없음 (§6 잠금 정책 참고)
- 기존 C++ 앱의 Emscripten 포팅 (A안 기각: Cocos2d-x 4.0은 공식 웹 타깃 없음)
- 앱 푸시/인앱리뷰/네이티브 공유 시트

---

## 2. 기술 스택 결정

| 항목 | 선택 | 이유 |
|---|---|---|
| 언어 | **TypeScript** | 서버 응답/리플레이 포맷에 타입을 걸어 포맷 버그 방지 |
| 렌더링 | **Canvas 2D 직접** (엔진 없음) | 앱 화면이 `DrawNode` 벡터 + 픽셀 폰트 중심 → Canvas 2D 경로 API와 1:1 대응. PixiJS/Cocos Creator 의존성 불필요 |
| 번들러 | **Vite** | 해시 파일명(캐시 문제 해결) + 정적 빌드 → gh-pages에 그대로 배포 |
| 오디오 | **Web Audio API** | 효과음 지연 최소화, BGM 크로스페이드 |
| 네트워크 | **fetch** (PlayFab REST) | 서버 로직은 이미 CloudScript. 랜딩 `landing/index.html`에서 동일 방식 사용 중 |
| 테스트 | **Vitest** (로직·직렬화) | 하노이 규칙/리플레이 직렬화/버전 비교는 단위 테스트가 가치 큼 |

> 논리 해상도는 앱과 같은 **960×640**. 캔버스를 창 크기에 맞춰 비율 유지 스케일(레터박스). 고DPI는 `devicePixelRatio`로 보정.

---

## 3. 저장소 구조 / 배포

```
Hanoi/                      (master)
├─ web/                     ← 신규: 웹 게임 소스
│  ├─ index.html
│  ├─ package.json, vite.config.ts, tsconfig.json
│  ├─ src/
│  │  ├─ main.ts            부트/씬 매니저/게임 루프
│  │  ├─ core/              game 규칙, 타이머, 입력, 씬 베이스
│  │  ├─ draw/              DrawUtils 포팅 + PixelFont 포팅
│  │  ├─ scenes/            Splash, Main(타이틀/랭킹/BGM), Play
│  │  ├─ net/               playfab.ts, leaderboard.ts, replay codec
│  │  ├─ audio/             SoundFactory 포팅
│  │  └─ data/              local storage(UserDataManager 대응)
│  ├─ public/audio/         ogg/mp3로 변환된 사운드
│  └─ tests/
├─ landing/                 (기존) 마케팅/지원 랜딩
└─ docs/web_port_plan.md    (본 문서)
```

- **배포 경로**: gh-pages 브랜치의 `/play/`. 랜딩(`/`)과 같은 사이트 → 랜딩에 "PLAY ON WEB" 버튼 추가.
- **빌드**: `web/`에서 `npm run build` → `web/dist`를 gh-pages `/play/`로 복사 (PowerShell 스크립트 `web/deploy.ps1`).
- **⚠️ gh-pages 이중 관리 주의**: 랜딩은 현재 gh-pages에서 직접 수정/마스터 `landing/`과 이중 관리. 웹 게임은 *소스는 master `web/`, 산출물만 gh-pages `/play/`*로 단방향. gh-pages 쪽 `/play/`는 직접 편집 금지.
- **저장소 공개 주의**: public 저장소 → 클라이언트에는 `Title ID(119C4E)`만. 관리자 키/시크릿 절대 금지 (`feedback_public_repo_secrets`).

---

## 4. 앱 → 웹 포팅 매핑

| 앱(C++) | 웹(TS) | 비고 |
|---|---|---|
| `common_define.h/.cpp` (상수, `countryToFlag`, `formatLikeCount`, `getRecordTime`) | `core/util.ts` | 순수 함수 → 거의 그대로 옮김 + 테스트 |
| `DrawUtils.cpp` (LED 패널·키캡·벡터 아이콘·팝업 프레임·칩버튼) | `draw/*.ts` | `DrawNode` → `CanvasRenderingContext2D` 경로. 가장 손이 많이 가는 부분 |
| `PixelFont.cpp` (5×7 글리프) | `draw/pixelFont.ts` | 글리프 데이터 테이블 그대로 이식 |
| `PlayScene*.cpp` (코어/HUD/Fx/Replay) | `scenes/play/*.ts` | 규칙·입력·HUD·연출·리플레이 4분할 유지 |
| `MainScene*.cpp` (타이틀/랭킹/다이얼로그/BGM) | `scenes/main/*.ts` | BGM 플레이어는 후순위 |
| `SplashScene.cpp` | `scenes/splash.ts` | 시그니처 인트로. 첫 클릭 = **오디오 잠금 해제 지점**으로 활용 |
| `LeaderboardManager.cpp` | `net/leaderboard.ts` | 로그인/캐시/재시도 정책 이식 (아래 §5) |
| `UserDataManager.cpp` (UserDefault) | `data/store.ts` (`localStorage`) | 12h 기록 초기화 규칙 포함 |
| `SoundFactory.cpp` | `audio/sound.ts` | Web Audio 버퍼 |
| `IAP*`, `NativeShare`, `Clipboard`, `ReviewManager` | 제외 / 웹 대체 | 공유는 `navigator.share`/클립보드로 대체 가능 |

### 이식 시 반드시 보존할 앱 규칙 (메모리 기반)
- **랭킹 Position은 원시 ms 내림차순**(느릴수록 낮음) → 순위 비교는 시간 값으로, Position 금지 (`project_leaderboard_position_inversion`)
- 리더보드 조회는 `fetchCount=20` 등 내림차순 윈도우 보정 + **캐시는 truncate 전 전체 저장** (`feedback_leaderboard_cache`)
- 신기록 후 **PlayFab 전파 지연** → 약 2초 후 `invalidateCache` + 재조회, 재시도 3단 + 짧은 TTL (`project_leaderboard_playfab_delay`)
- 로컬 기록 12h 초기화 시 서버 Min 집계가 기록 보호 → `BestTime_Lxx` 집계 설정 변경 금지
- 로그인 **in-flight 가드**(동시 login 방지, 콜백 대기열) (`project_recent_players_ticker`)
- 문자열 자르기는 **코드포인트 단위** (`feedback_utf8_truncate_ios`; JS는 `Array.from()`)
- `ProfileConstraints`는 `ShowLocations` + `ShowDisplayName` 동시 지정 (`feedback_playfab_profileconstraints`)

---

## 5. 서버 연동 / 신원(Identity) 설계

### 5.1 현재 앱 방식
- `LoginWithCustomID` + **기기별 랜덤 CustomId**(`playfab_custom_id`) 로 로그인. 점수는 클라이언트가 `UpdatePlayerStatistics`로 직접 제출(`BestTime_Lxx`).
- 웹은 같은 방식이면 **웹 전용 계정**이 따로 생긴다(앱 계정과 별개).

### 5.2 결정이 필요한 사항 — 신원 모델

| 안 | 내용 | 장점 | 단점 |
|---|---|---|---|
| **S1. 웹 독립 계정** (권장 1차) | 브라우저 `localStorage`에 CustomId 생성 → 웹 전용 PlayFab 계정. 이름 설정은 앱과 동일 흐름 | 서버 변경 0, 구현 빠름 | 앱 기록과 별개 랭킹 항목(같은 보드엔 보이나 동일인 아님). 브라우저 데이터 삭제 시 계정 유실 |
| **S2. 앱 → 웹 원탭 링크** | 앱 설정에서 토큰 발급(`issueWebToken`) → 웹 `?wt=` → 같은 PlayFab 계정으로 로그인. Runners' Board가 이미 이 방식 | 앱과 같은 사람/랭킹 | 현재 `redeemWebToken`은 **게시판용 세션키**만 발급 — 점수 제출까지 쓰려면 서버 확장 필요 |
| **S3. 이름 링크 없이 읽기 전용** | 웹은 랭킹 조회/리플레이 관전만 | 가장 안전 | "PC에서 플레이"라는 목표와 불일치 |

→ **권장: 1차 S1, 이후 S2를 2차로 확장.** S1은 서버 수정 없이 시작 가능하고, S2는 별도 단계에서 서버 설계가 필요하다.

### 5.3 사용할 PlayFab API (모두 Client API, 앱과 동일)
- `LoginWithCustomID`, `UpdateUserTitleDisplayName`, `UpdatePlayerStatistics`
- `GetLeaderboard`(StatisticName=`BestTime_Lxx`, ProfileConstraints), `GetTitleData`(공지/버전/스위치)
- `ExecuteCloudScript`: `validateName`, `touchRecentPlayer`, `writeReplay`, `fetchReplays`(관전), `likeReplay`, `writeAwardComment` 등 **기존 핸들러 재사용**

### 5.4 부정행위(치팅) 위험 — 반드시 인지
- 웹 클라이언트는 소스가 공개되고 devtools로 통신을 조작하기 쉬워, 임의 점수(예: 1ms) 제출이 앱보다 훨씬 쉽다. (앱도 네트워크 도구로 가능하지만 장벽이 있음.)
- 현재 점수 제출은 **서버 검증이 없다**(클라가 통계를 직접 갱신).
- 대응 단계:
  1. **1차(MVP)**: 웹은 *별도 리더보드 통계*(`WebBestTime_Lxx`) 또는 웹 전용 뱃지로 분리하는 안도 검토. 최소한 이상치(물리적으로 불가능한 시간) 컷은 클라/서버에 둔다.
  2. **2차**: CloudScript `submitScoreVerified` — 리플레이(이벤트 로그)를 함께 올리면 서버가 **하노이 규칙 합법성 + 최소 이동수 + 마지막 t_ms == 제출 시간**을 검증 후 `UpdatePlayerStatistics`(Server API)로 기록. `docs/replay_ghost_plan.md`가 이미 이 정합성을 설계해 둠(리플레이=증명서).
  3. 기존 앱 보드와 합칠지(같은 통계) 분리할지는 **§8 결정사항 D2**.

---

## 6. 게임 정책 (웹)

| 항목 | 정책(안) | 비고 |
|---|---|---|
| 레벨 | 앱은 **Lv.3~10** (`canDec = 디스크 > 3`, `MAX_PLAY_LEVEL=10`) — Lv.1~2는 존재하지 않음 | 앱과 동일한 번호 체계 사용 |
| 잠금 (**D3 확정**) | 앱은 LITE 빌드로 Lv.5+ 구매 전 잠금(`MAX_LIMIT_LEVEL_FOR_LITE=4`) | 웹은 **Lv.3~4만 개방**. Lv.5+ 버튼은 🔒로 표시하고 누르면 "앱 설치" 안내(랜딩 스마트 링크 `SHARE_URL`)로 연결 |
| 입력 | 마우스 클릭(탭-탭), 드래그&드롭, 키보드 `1/2/3`(폴 선택) `←→`·`Space` | 앱의 탭-탭/드래그 두 경로 모두 지원 |
| 일시정지/포커스 | 탭 비활성화(`visibilitychange`) 시 타이머 처리 규칙 필요 | 공정성: 포커스 이탈 중 시간은 흐르게 하되 소리만 정지 |
| 타이머 | `performance.now()` 기반 ms. `Date.now()` 사용 금지 | 시스템 시계 변경 영향 방지 |
| 오디오 | 첫 사용자 제스처에서 `AudioContext.resume()` | 스플래시/START 클릭에서 해제 |

---

## 7. 단계별 구현 계획 (마일스톤)

> 추정은 Claude가 구현하는 기준의 순수 작업량이며, 디자인 검수/수정 라운드는 별도. 각 단계 끝에 **데모 가능한 상태**를 만든다.

### Phase 0 — 기반 (약 2~3일)
- `web/` 스캐폴딩(Vite+TS), 캔버스 부트(960×640 레터박스, DPR), 씬 매니저, 게임 루프, 입력(마우스/키보드)
- `util.ts` 포팅(`getRecordTime`, `countryToFlag`, `formatLikeCount`, `compareVersion`) + Vitest
- 배포 스크립트 `web/deploy.ps1` + gh-pages `/play/` 시험 배포
- **완료 기준**: 빈 캔버스 + 씬 전환이 gh-pages에서 열림

### Phase 1 — 플레이 코어 MVP (약 1~1.5주)
- `DrawUtils`/`PixelFont` 핵심 포팅(LED 패널, 키캡, 디스크/폴 렌더, 픽셀 텍스트)
- 하노이 규칙/입력(탭-탭·드래그), 카운트다운(신호등), 타이머, 완료 판정, 결과 팝업
- 로컬 기록 저장(12h 초기화 규칙 포함), 효과음(Web Audio)
- **완료 기준**: Lv.3~4를 끝까지 플레이하고 기록이 로컬에 남음. Lv.5+는 🔒 + 앱 설치 안내

### Phase 2 — 랭킹 연동 (약 1주)
- `playfab.ts`(login in-flight 가드), 이름 설정(`validateName`+클라 필터), 점수 제출(§5.4 정책 적용)
- 랭킹보드(레벨 전환, 페이지, 캐시, 전파 지연 보정, 만국기 이모지)
- 첫 진입 플로우(이름 없이 먼저 플레이 → 점수판 탭 시 이름 입력, `HasPendingSubmit`)
- **완료 기준**: 웹에서 낸 기록이 랭킹보드에 반영되고 앱에서도 조회됨

### Phase 3 — 메인/타이틀 완성 (약 1~1.5주)
- 스플래시(시그니처 인트로), 타이틀(START 픽셀 애니), 설정/다이얼로그, 공지·버전 티커(Title Data), 최근 접속자 티커
- BGM 플레이어(카세트/스피커), 사운드 옵션
- 랜딩에 "PLAY ON WEB" 버튼, OG/SEO 갱신
- **완료 기준**: 앱과 같은 흐름으로 타이틀→플레이→랭킹→복귀

### Phase 4 — 리플레이/관전/고스트 (약 1~1.5주) — ⏸ *보류 (§0)*
- 리플레이 포맷(delta-varint) 인코더/디코더 — **앱과 바이트 호환**(웹에서 올린 리플레이를 앱이 재생 가능, 반대도 동일). 호환 테스트 필수
- 결과 팝업 [REPLAY], 랭커 카드 [▶ WATCH], 고스트 레이스, 좋아요
- 서버 검증 제출(`submitScoreVerified`)과 함께 진행하면 치팅 방지 효과
- **완료 기준**: 앱 랭커 리플레이를 웹에서 관전

### Phase 5 — 신원 링크(S2) 및 마감 (약 1~1.5주) — ⏸ *보류 (§0)*
- 앱 ↔ 웹 **기기 코드 연동**으로 같은 계정 사용 (서버 확장 + 앱 UI 포함, 상세 §11)
- 성능/접근성/모바일 브라우저 대응(터치 입력 유지), 오류 리포팅

### 일정 요약

| 범위 | 기간(추정) |
|---|---|
| **MVP** (Phase 0~2: 플레이 + 랭킹 조회/등록) | **약 2.5~3.5주** |
| **앱 수준 완성** (+ Phase 3) | 약 4~5주 |
| **풀 기능** (+ Phase 4~5) | 약 6~8주 |

---

## 8. 결정이 필요한 사항

| # | 질문 | 선택지 | 권장 |
|---|---|---|---|
| D1 | 신원 모델 | S1 독립 / S2 앱 링크 / S3 읽기전용 | ✅ **확정(수정): 계정 없는 읽기 전용(§0)**. S1/S2는 확대 게이트 통과 시 재검토 |
| D2 | 랭킹 통합 | 앱 보드와 동일 통계 공유 / 웹 별도 통계 | ✅ 웹은 서버에 쓰지 않으므로 해당 없음 (§0) |
| D3 | 잠금 정책 | 웹 Lv.3~4만 / Lv.5+ 무료 개방 | ✅ **확정: Lv.3~4만 개방, Lv.5+는 앱 설치 유도** |
| D4 | 호스팅 경로 | gh-pages `/play/` / 별도 레포 | **`/play/`** (랜딩과 동일 사이트) |
| D5 | 모바일 웹 | PC 전용 / 모바일 브라우저도 허용 | 터치는 이미 지원하므로 허용하되 앱 설치 배너 노출 |

---

## 9. 리스크 및 대응

| 리스크 | 영향 | 대응 |
|---|---|---|
| 점수 조작(웹 클라 신뢰 불가) | 랭킹 신뢰 손상 | §5.4 단계 대응. 최소한 불가능 시간 컷, 이후 리플레이 서버 검증 |
| 오디오 자동재생 차단 | 첫 진입 무음 | 첫 클릭에서 `resume()`, UI에 "SOUND ON" 안내 |
| 리소스 용량 (Sound 약 44MB WAV) | 로딩 지연/대역폭 | **ogg/mp3로 변환**(ffmpeg), BGM은 지연 로드. 사용하는 이미지(`Resources/NewUI` 약 14MB 중 실사용분)만 선별 |
| 브라우저별 차이(Safari 오디오 포맷, 폰트 이모지 렌더) | 일부 브라우저 깨짐 | mp3/AAC 폴백, 만국기는 앱과 동일하게 유니코드 이모지(Windows 크롬은 국기 이모지 미지원 → **PNG 국기 폴백**) |
| gh-pages 이중 관리 | 배포 덮어쓰기 사고 | 단방향 배포 스크립트, `/play/`는 수동 편집 금지 |
| 시계/포커스 이탈로 인한 타이머 편법 | 공정성 | `performance.now()` 사용, 비활성 처리 정책 확정 |
| 리플레이 포맷 불일치 | 앱↔웹 재생 호환 깨짐 | 앱의 `PlayScene_Replay.cpp` 직렬화를 바이트 단위 테스트 벡터로 고정 |

> **⚠️ 국기 이모지 주의**: Windows(특히 Chrome/Edge)는 국기 이모지를 지역 문자 두 글자(KR)로 보여준다. 앱은 이모지를 선호하지만(`feedback_flag_emoji`), 웹은 PC 사용자가 주 대상이므로 `landing/flags/`의 PNG 자산을 폴백으로 재사용하는 방안을 Phase 2에서 확정한다.

---

## 10. 다음 액션
1. D1~D5 결정 확정 (특히 **D1 신원, D3 잠금**)
2. Phase 0 착수: `web/` 스캐폴딩 + gh-pages `/play/` 시험 배포
3. Phase 1 진행 중 중간 데모로 UI 톤(앱과의 일치) 검수

---

## 11. [보류] 2차 설계 — 앱 ↔ 웹 계정 연동 (S2)

> ⏸ §0에 따라 **보류**. 사용자 수 확인 후 재검토할 때 쓰는 참고 설계. 현재 구현 대상 아님.

### 11.1 목표
PC 웹에서 플레이한 기록이 **앱 계정(같은 이름·국가·랭킹)** 으로 올라가게 한다. 웹 독립 계정(S1)으로 시작한 사용자가 한 번의 연동으로 앱 계정으로 전환할 수 있어야 한다.

### 11.2 기존 자산 (재사용)
이미 Runners' Board가 같은 문제를 푼 인프라가 있다 (`cloudscript.js` `issueWebToken` / `redeemWebToken` / `webMintSession` / `webLookupSession`).
- 앱이 인증된 상태로 **일회용 토큰(5분)** 을 발급 → 웹이 **세션키(30일 슬라이딩, 플레이어당 1개)** 로 교환
- 웹은 PlayFab 계정으로 직접 로그인하지 않고, **공용 뷰어 계정 + 세션키를 CloudScript에 실어** 보낸다. 서버가 세션키에서 소유자 `playFabId`를 풀어 그 사람 명의로 동작한다.
- 이름/국가는 서버 프로필에서 읽어 오므로 웹에서 조작 불가

→ **점수 제출도 같은 패턴**으로 처리한다: 웹은 앱 계정의 클라이언트 티켓을 받지 않고, 서버가 대리로 통계를 기록한다.

### 11.3 연동 방식 비교

| 방식 | 흐름 | 평가 |
|---|---|---|
| **L1. 링크 보내기** | 앱 버튼 → `issueWebToken` → 웹 링크(`?wt=`)를 공유 시트/클립보드로 PC에 전달 | 서버 변경 0, 앱 UI 1개. 단 폰→PC로 링크를 옮기는 번거로움(5분 내 사용) |
| **L2. 기기 코드** (권장) | PC 웹이 **8자리 코드**를 표시 → 앱 설정에서 코드 입력 → 웹이 자동 연동 | TV 앱 로그인과 같은 UX. PC에서 시작해 폰으로 승인 → 링크를 옮길 필요 없음 |
| L3. QR | 웹이 QR 표시 → 앱이 스캔 | 가장 매끄럽지만 앱에 QR 스캐너(카메라 권한·라이브러리) 추가 필요. L2 이후 확장 |

### 11.4 L2 기기 코드 흐름

```
[PC 웹]                         [PlayFab CloudScript]                [앱]
  │ createLinkCode (게스트 G 명의) →│ 코드 K7M4QX9P 발급, G에 묶음        │
  │ ← code 표시, 2초마다 poll →    │ (TTL 3분, 일회용)                    │
  │                                │              ← 사용자가 코드 입력 ── │
  │                                │ approveLinkCode(code) (인증된 앱 사용자 A)
  │                                │  A가 이름 보유? → 세션키 발급(webMintSession(A))
  │                                │  세션키를 G의 Internal Data에 저장(공유 그룹 X)
  │ pollLinkCode (G) →             │ G 본인에게 1회 전달 후 삭제
  │ ← sessionKey, name, cc         │
  │ localStorage 저장, 연동 완료 UI │
```

**신규 CloudScript 핸들러** (모두 `boardEnabled()`와 같은 마스터 스위치 `web_link` 보유, fail-open)

| 핸들러 | 호출자 | 동작 |
|---|---|---|
| `createLinkCode` | 웹(게스트 G) | 코드 생성(혼동 문자 제외 알파벳, 8자) → `code→G` 인덱스 저장, G 쿨다운(분당 1~2회) |
| `approveLinkCode` | 앱(A) | 코드로 G 조회 → 이름 없으면 `no_name` → `webMintSession(A)` → G의 Internal Data `pending_session`에 기록 → 코드 소각. **실패 횟수 제한**(A당 시도 5회/10분)으로 코드 무차별 대입 차단 |
| `pollLinkCode` | 웹(G) | G의 `pending_session`이 있으면 1회 반환 후 삭제. 만료/없음은 `pending`/`expired` |
| `submitScoreVerified` | 웹(G 또는 세션키) | §11.5 |
| `unlinkWeb` | 앱 또는 웹 | 세션 폐기(기존 `revokeWebSession` 재사용) |

**앱 변경**
- 설정(또는 메인) 팝업에 **"LINK PC"** 항목 추가 → 코드 입력(이름 입력 다이얼로그의 에디트박스 패턴 재사용) → `approveLinkCode` 호출 → 토스트 `LINKED`
- 이름이 없으면 먼저 이름 설정 유도 (`no_name`)
- iOS/Android 공통 코드(C++)이므로 네이티브 작업 없음

### 11.5 서버 대리 점수 제출 (`submitScoreVerified`)
연동 사용자 입장에서 가장 중요한 부분이며, **치팅 방어(§5.4)의 2단계와 동일**하다.

1. 웹이 `{ sessionKey(또는 G 티켓), level, replayBlob }` 전송 — 리플레이는 앱과 **같은 포맷**(`docs/replay_ghost_plan.md`)
2. 서버가 검증:
   - 하노이 규칙 합법성(이동 가능 여부, 큰 디스크가 작은 디스크 위에 올라가지 않음)
   - 최소 이동 수(2^N−1) 이상, 마지막 `t_ms == finalTimeMs == 제출 시간`
   - 이벤트 간 시간이 사람 입력으로 가능한 최소 간격 이상(너무 균일/너무 빠르면 거부)
3. 통과 시 `server.UpdatePlayerStatistics({ PlayFabId: 소유자, Statistics: [BestTime_Lxx] })` — 통계 집계는 **Min 유지**(`project_record_expiry_playfab_safety`). 신기록이면 리플레이도 `writeReplay`로 저장
4. 반환: `{ ok, newRecord }` → 웹은 앱과 같은 **2초 지연 후 재조회** 패턴 적용

> **한계**: 현재 앱은 점수를 클라이언트가 직접 `UpdatePlayerStatistics`로 올린다. PlayFab의 "클라이언트 통계 쓰기 허용" 타이틀 설정은 전체 스위치라 끄면 앱 제출이 깨진다. 따라서 **웹이 서버 검증을 쓰더라도, 앱 쪽 제출도 `submitScoreVerified`로 옮기고 해당 설정을 끄기 전까지는** 직접 API를 호출하는 조작은 막지 못한다. 이는 별도 프로젝트(앱 업데이트 + 버전 게이트 `project_version_gate` 연계)로 분리한다.

### 11.6 보안 점검 항목
- ⚠️ **기존 `web_link` 토큰 저장소 점검 필요**: `issueWebToken`이 토큰을 공유 그룹 `web_link`(Permission `Public`)에 저장한다. 저장소가 public이라 그룹 ID가 공개되어 있다. 클라이언트가 그룹 전체를 읽을 수 있다면 살아 있는 토큰(5분)을 훔쳐 세션을 얻을 수 있다. **구현 착수 전 실제 읽기 가능 여부를 확인**하고, 가능하면 토큰/코드 저장 위치를 클라이언트가 읽을 수 없는 곳(소유자 Internal Data 등)으로 옮긴다. L2 설계는 세션키를 공유 그룹에 두지 않도록 이미 이 점을 반영했다(§11.4).
- **세션키 보관 위치**: 브라우저 `localStorage`. `ozzywow.github.io` 는 다른 프로젝트 페이지와 **origin을 공유**하므로, 같은 origin의 다른 페이지가 세션키를 읽을 수 있다 → 이 origin에는 외부 스크립트를 로드하는 다른 페이지를 두지 않는다. 세션은 서버에서 언제든 폐기 가능(`unlinkWeb`).
- **세션 단일성**: 현재 플레이어당 세션 1개(덮어쓰기). 두 번째 브라우저에서 연동하면 첫 번째는 무효화된다. 게시판 세션과 **공유**되므로, 웹 게임 연동이 게시판 세션을 갱신한다는 점은 정상 동작으로 문서화한다.
- **S1 → S2 전환 시 기록**: 게스트(G) 기록은 G 계정에 남고 앱 계정으로 합치지 않는다(합치면 조작 경로가 열림). 연동 후부터 앱 계정으로 올라가며 UI에 "연동 전 기록은 이 브라우저 로컬에만 남음"을 안내한다.
- 코드 입력 실패 횟수 제한, 코드 TTL 3분·일회용, 마스터 스위치 `web_link`(Title Data)로 즉시 비활성화 가능.

### 11.7 작업 항목 및 추정

| 작업 | 내용 | 추정 |
|---|---|---|
| 서버 | 핸들러 4종(§11.4) + 점수 검증기(§11.5) + 보안 점검(§11.6) | 3~4일 |
| 앱 | "LINK PC" 코드 입력 UI + `LeaderboardManager::approveLinkCode` + 에러/토스트 | 1~2일 |
| 웹 | 연동 화면(코드 표시·폴링·성공/만료), 연동 모드 제출 경로, 연동 해제 | 2~3일 |
| 테스트 | 리플레이 검증기 단위 테스트(정상/위조/경계), 코드 만료·재사용·무차별 대입, 앱↔웹 실기 | 2일 |
| **합계** | | **약 1.5~2주** (D2 통합/앱 제출 전환은 별도) |

### 11.8 선행 조건
- Phase 4(리플레이 코덱)가 선행되어야 `submitScoreVerified`를 쓸 수 있다. Phase 4를 건너뛰면 S2는 **연동까지만** 제공하고 점수는 직접 제출(검증 없음)로 임시 운영한다.
- 앱 배포가 필요하므로(LINK PC UI) 스토어 심사 일정에 맞춰 Phase 5 서버 배포 → 앱 릴리스 → 웹 활성화 순으로 진행한다.
