# 운영 도구 (Admin Tools)

UGC 신고 대응·긴급 조치용. PlayFab Title **`119C4E`** 대상.

| 도구 | 대상 |
|---|---|
| [`admin_award.ps1`](admin_award.ps1) | 랭킹 Top10 수상소감 |
| [`admin_board.ps1`](admin_board.ps1) | 웹 게시판 (Runners' Board) |
| PlayFab 대시보드 Run | 스크립트가 없는 나머지 관리자 핸들러 |

---

## 0. 준비 — admin_key

모든 관리자 기능은 Title Internal Data 의 **`admin_key`** 로 잠겨 있습니다.

> ⚠️ **이 저장소는 public 입니다.** 키를 스크립트나 소스에 적지 마세요.
> 노출된 키는 파일에서 지워도 git 히스토리에 남아, **값 교체(rotation)만이 해결책**입니다.

한 번만 등록해 두면 두 도구 모두 입력 없이 동작합니다.

```powershell
[Environment]::SetEnvironmentVariable("HANOI_ADMIN_KEY", "<키>", "User")
```

새 PowerShell 창부터 적용됩니다. 설정하지 않으면 실행 시 입력을 요구합니다(화면에 표시되지 않음).

**키를 새로 만들 때** — 직접 뽑으면 값이 어디에도 기록되지 않습니다.

```powershell
-join ((48..57)+(65..90)+(97..122) | Get-Random -Count 40 | % {[char]$_})
```

→ PlayFab **Content → Title Data → TITLE INTERNAL DATA** 의 `admin_key` 값을 교체.

---

## 1. 수상소감 — `admin_award.ps1`

```powershell
# 전체 조회 (L3~L10, 한글 자동 디코딩)
.\admin_award.ps1

# 특정 유저의 소감 삭제
.\admin_award.ps1 -Delete -Level 3 -TargetId 38F18129ABAD5A47

# 확인 프롬프트 없이
.\admin_award.ps1 -Delete -Level 3 -TargetId 38F18129ABAD5A47 -Force
```

| 파라미터 | 설명 |
|---|---|
| `-Delete` | 삭제 모드 (없으면 조회) |
| `-Level` | 3~10. 삭제 시 필수 |
| `-TargetId` | 작성자 playFabId. 삭제 시 필수 |
| `-Force` | 확인 프롬프트 생략 |

소감은 **레벨별로 1인 1건**이라 `Level` + `TargetId` 조합이 곧 대상입니다.
`TargetId` 는 조회 결과 표의 `targetId` 열에 나옵니다.

---

## 2. 웹 게시판 — `admin_board.ps1`

```powershell
# 전체 조회 (최신순)
.\admin_board.ps1

# 글 하나 삭제
.\admin_board.ps1 -Delete -Id 1754198400000abc123

# 특정 작성자의 글 전부 삭제
.\admin_board.ps1 -Delete -TargetId 38F18129ABAD5A47

# 도배 대응 — 글 전부 삭제 + 웹 세션 차단
.\admin_board.ps1 -Delete -TargetId 38F18129ABAD5A47 -Revoke

# 세션만 차단
.\admin_board.ps1 -Revoke -TargetId 38F18129ABAD5A47

# 게시판 전체 비우기
.\admin_board.ps1 -Delete -All -Force
```

| 파라미터 | 설명 |
|---|---|
| `-Delete` | 삭제 모드 (없으면 조회) |
| `-Id` | 글 id. 조회 결과 `Id` 열 |
| `-TargetId` | 작성자 playFabId. 조회 결과 `Author` 열 |
| `-All` | 전체 삭제 (`-Delete` 와 함께) |
| `-Revoke` | 웹 세션 무효화. 단독 또는 `-Delete -TargetId` 와 조합 |
| `-Force` | 확인 프롬프트 생략 |

**세션 차단(`-Revoke`)의 의미** — 계정 정지가 아니라 **브라우저 연결만 끊는 것**입니다.
해당 유저가 앱에서 💬 를 다시 누르면 새 세션을 받아 복구됩니다.
따라서 반복 도배에는 차단만으로 부족하고, 필요하면 게시판 마스터 스위치를 끄는 편이 확실합니다(§4).

> 조회는 CloudScript 를 거치지 않고 `GetSharedGroupData` 를 직접 읽습니다(보드가 Permission=Public).
> 삭제·차단만 CloudScript 를 호출합니다.

---

## 3. 스크립트가 없는 관리자 핸들러

PlayFab **Game Manager → Automation → Cloud Script → Run Revision** 에서 직접 실행합니다.

| FunctionName | FunctionParameter | 용도 |
|---|---|---|
| `clearRecentPlayers` | `{ "adminKey":"<키>" }` | 하단 티커의 최근 접속자 목록 초기화 |
| `resetAllLeaderboards` | `{ "adminKey":"<키>" }` | **전 레벨(L3~L10) 랭킹 초기화 — 되돌릴 수 없음** |
| `dumpAwardComments` | `{ "adminKey":"<키>" }` | 소감 원본 덤프 (`admin_award.ps1` 이 쓰는 것과 동일) |

### Scheduled Task 두 개

자동으로 도는 태스크라 평소 손댈 일이 없지만, **게이트가 걸려 있어 파라미터에 `adminKey` 가 반드시 들어 있어야** 합니다.

| 태스크 | FunctionParameter | 주기 |
|---|---|---|
| `maintainLeaderboards` | `{ "adminKey":"<키>" }` (필요 시 `"topN":10` 추가) | `0 * * * *` (매시) |
| `cleanupJunkPlayer` | `{ "adminKey":"<키>" }` | 세그먼트 액션 |

> ⚠️ **admin_key 를 교체하면 이 두 태스크의 파라미터도 같이 고쳐야 합니다.**
> 안 고치면 `denied` 로 **조용히** 멈춥니다 — 에러가 눈에 띄지 않으니 주의.
> 순서는 **태스크 파라미터 먼저 → CloudScript 배포 나중**.

---

## 4. 상황별 대응

**욕설 소감이 신고됨**
```powershell
.\admin_award.ps1                                   # targetId 확인
.\admin_award.ps1 -Delete -Level <n> -TargetId <id>
```
재발하면 금칙어를 `server/banned_words.json` 에 추가 → Title Internal Data `banned_words` 갱신
(CloudScript 재배포 불필요).

**게시판 도배**
```powershell
.\admin_board.ps1 -Delete -TargetId <id> -Revoke
```
서버에 이미 **쿨다운 60초 + 작성자당 5슬롯** 제한이 있어 보드 전체가 밀려나지는 않습니다.

**게시판을 잠시 닫아야 함**
Title Data(Internal 아님) `web_board` = `0` → 랜딩에서 섹션이 통째로 사라지고 글쓰기·토큰 발급이 막힙니다.
글과 세션은 보존되어 `1` 로 되돌리면 그대로 복귀합니다. 앱 재배포 불필요.

**랭킹이 오염됨**
`resetAllLeaderboards` 는 전 레벨을 0으로 미는 최후 수단입니다.
평소의 상위권 유지·비활성 정리는 `maintainLeaderboards` 가 매시간 자동으로 합니다.

---

## 5. 문제 해결

| 증상 | 원인 |
|---|---|
| `{"ok":false,"reason":"denied"}` | `admin_key` 불일치. 환경변수 값·앞뒤 공백·`adminKey` 철자(대소문자) 확인 |
| 한글이 `ì†Œê°`처럼 깨짐 | 스크립트의 `Fix()` 가 처리합니다. 그래도 깨지면 콘솔 코드페이지 문제 |
| `.ps1` 실행 시 파싱 에러 | **PowerShell 5.1 은 BOM 없는 파일을 ANSI 로 읽습니다.** 스크립트는 반드시 **UTF-8 BOM** 으로 저장 |
| `{"ok":false,"reason":"no_group"}` | Shared Group 미생성. 새 클라가 한 번 로그인하면 부트스트랩됩니다 |
| 정기 정리가 멈춘 것 같음 | Scheduled Task 실행 이력에서 `denied` 확인 (§3) |

---

관련 문서: [`BOARD_DEPLOY.md`](BOARD_DEPLOY.md) (게시판 배포) · [`PHASE0_DEPLOY.md`](PHASE0_DEPLOY.md) (수상소감 배포)
