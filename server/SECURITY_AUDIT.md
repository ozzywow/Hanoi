# CloudScript 보안 점검 (2026-11-17)

저장소가 **public** 이라 `cloudscript.js` 는 누구나 읽을 수 있다. 그래서 "코드를 몰라서 안전"이 아니라
"코드를 알아도 안전"해야 한다. 모든 핸들러의 권한 검사를 점검하고 취약 지점을 보강했다.
회귀 테스트: `node --test server/cloudscript.test.mjs` (PlayFab 없이 가짜 `server` 로 실행, 수정 전 리비전에서는 6건 실패).

## 핸들러 점검 결과

| 구분 | 핸들러 | 인증 | 결과 |
|---|---|---|---|
| 관리자 | `maintainLeaderboards` `resetAllLeaderboards` `cleanupJunkPlayer` `clearRecentPlayers` `dumpAwardComments` `adminDeleteAwardComment` `adminDeleteBoardPost` `revokeWebSession` | `awardAdminAuthorized` (Title Internal Data `admin_key`) | ✅ 모두 게이트됨. 키는 코드/저장소에 없음 |
| 본인 데이터 | `writeAwardComment` `deleteAwardComment` `writeReplay` `deleteReplay` `deleteBattle` `touchRecentPlayer` | `currentPlayerId` | ✅ 자기 키만 씀 (아래 보강 참고) |
| 대상 지정 | `likeReplay` (targetId) `recordBattle` (loserId) | `currentPlayerId` + 서버 검증 | ⚠️ likeReplay 보강함. recordBattle 은 서버 통계로 검증(한계: 통계 자체가 클라이언트 쓰기) |
| 웹 연동 | `issueWebToken` `redeemWebToken` | 앱 로그인 / 토큰 | ❌→✅ **토큰 저장소 취약점 수정** |
| 게시판 | `writeBoardPost` `deleteBoardPost` `writeBoardComment` | 세션키 또는 `currentPlayerId` | ⚠️ 공용 뷰어 차단 보강 |

## 수정한 취약점

1. **일회용 토큰 탈취 (높음)** — `issueWebToken` 이 토큰을 공개 Shared Group `web_link` 에 저장했다.
   그룹 ID 는 저장소에 공개돼 있으므로, 로그인만 한 누구나 그룹 전체를 읽어 살아 있는 토큰(5분)으로 세션을 만들 수 있었다.
   → 토큰의 비밀 부분은 **발급받은 플레이어 본인의 Internal Data(`web_token`)** 에만 저장한다.
   토큰 문자열 = `비밀(24자) + playFabId` (문자 `[A-Za-z0-9]`, 랜딩의 `/wt=([A-Za-z0-9]+)/` 와 호환).
   틀린 비밀로는 진짜 토큰을 소각하지 않는다. 앱/랜딩 수정 없이 서버만 바꾸면 된다.

2. **공용 뷰어 계정으로 게시판 쓰기 (중간)** — `web_public_viewer` CustomId 는 공개돼 있어 누구나 그 계정으로 로그인해
   표시명을 바꿀 수 있다. 차단은 Title Internal Data `web_viewer_id` 설정에 의존했다(누락 가능).
   → `GetUserAccountInfo` 의 CustomId 로도 판별해 설정과 무관하게 `read_only` 처리.

3. **최근 접속자 티커 콘텐츠 주입 (중간)** — `touchRecentPlayer` 는 프로필 이름이 없을 때 클라가 보낸 `name` 힌트를
   검증 없이 공개 그룹에 저장했다(모든 앱 사용자 하단 티커에 노출). → 이름 설정과 같은 규칙(길이·링크·욕설)으로 검증.

4. **리플레이 블롭 (중간)** — `writeReplay` 는 크기만 검사했다. → Base64 문자만 허용 + 헤더의 디스크 수가 레벨과 일치 +
   리플레이가 주장하는 기록이 서버 통계보다 빠르면 거부(`time_mismatch`).

5. **좋아요 대상 (낮음)** — `likeReplay` 의 `targetId` 로 임의 키를 만들 수 있었다. → PlayFabId 형식 + 해당 레벨에 리플레이가 있는 대상만.

## 배포 방법
PlayFab Game Manager → Automation → CloudScript → **Revisions** → `cloudscript.js` 업로드 → **Deploy to Live**.
(저장소 푸시만으로는 PlayFab 에 반영되지 않는다.) 배포 직후 확인:
- 앱에서 게시판 버튼(💬)을 눌러 랜딩이 열리고 이름으로 글쓰기가 되는지 (issue → redeem 경로)
- 신기록 후 리플레이 업로드가 성공하는지 (`writeReplay` 의 새 검증)
- 문제가 생기면 이전 리비전으로 되돌리면 된다.
- 선택: 구 토큰 그룹 `web_link` 는 더 이상 쓰이지 않는다(내용은 이미 만료). 필요하면 Game Manager 에서 비워도 된다.

## 남은 위험 (코드만으로 해결 불가)
- **점수는 클라이언트가 직접 `UpdatePlayerStatistics` 로 쓴다.** 앱·웹 모두 조작 가능. 근본 해결 = 앱이 리플레이를 서버에 올려
  서버가 검증 후 통계를 쓰고, "클라이언트 통계 쓰기" 설정을 끄는 것(앱 업데이트 필요, `docs/web_port_plan.md` §11.5).
  단기 대안: `maintainLeaderboards` 에 레벨별 "물리적 하한"(예: 7수 최소 이동 × 사람이 낼 수 있는 최소 간격)을 두고 그 미만은 0 처리.
- `admin_key` 는 **충분히 길고 무작위**(32자 이상)여야 한다 — 코드가 공개라 시도 횟수 제한이 없는 게이트다.
- `writeAwardComment` 는 서버에서 Top10 여부를 확인하지 않는다(내용 검증만). GC 가 Top10 밖 키를 지우므로 노출은 제한적.
- 금칙어 목록(`server/banned_words.json`)이 공개돼 우회가 쉽다. 목록을 Title Internal Data 로 옮기면 가릴 수 있다.
- 이미 공개된 이력은 회수되지 않는다. 민감 설정이 있었다면 교체를 고려.
