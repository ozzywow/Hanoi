# 웹 게시판 (Runners' Board) — 배포 가이드

랜딩(https://ozzywow.github.io/Hanoi/)의 커뮤니티 게시판 서버측 준비입니다.
대상: PlayFab Title **`119C4E`** (classic CloudScript — `maintainLeaderboards`와 동일 리비전).

## 동작 요약

```
[앱] 💬 버튼 → issueWebToken (5분·1회용 토큰)
     → openURL("https://ozzywow.github.io/Hanoi/?wt=<토큰>")
[웹] ?wt= 읽자마자 주소창에서 제거 → redeemWebToken → 세션키(30일) → localStorage
[웹] writeBoardPost(sessionKey, text)
```

- **공유 링크로 들어온 방문자는 세션키가 없어 자동으로 읽기 전용** — 설치 유도 카드가 대신 표시됩니다.
- 이름은 클라이언트가 보내지 않습니다. 서버가 `세션키 → playFabId → DisplayName`으로 직접 붙입니다(사칭 차단).
- 세션은 플레이어당 1개. 앱에서 다시 원탭하면 이전 세션은 즉시 무효(= 원격 로그아웃).

---

## A. CloudScript 배포

`cloudscript.js` 하단에 아래 핸들러가 추가되어 있습니다.

| 핸들러 | 호출자 | 용도 |
|---|---|---|
| `issueWebToken` | 앱 | 원탭 링크용 일회용 토큰 발급 |
| `redeemWebToken` | 웹 | 토큰 → 세션키 교환 |
| `writeBoardPost` | 웹/앱 | 글 작성 (길이·링크·욕설 재검증) |
| `deleteBoardPost` | 웹/앱 | 본인 글 삭제 |
| `adminDeleteBoardPost` | 관리자 | 신고 글 삭제 (`id` 생략 시 전체) |
| `revokeWebSession` | 관리자 | 특정 플레이어 웹 세션 강제 무효화 |

배포: 대시보드 → **Automation → Cloud Script → Revisions** → 편집창을
**`cloudscript.js` 파일 전체로 교체** → **Save and deploy revision**.

> ⚠️ 일부만 붙이면 기존 핸들러가 사라집니다. 반드시 파일 전체.

---

## B. Title Internal Data `web_viewer_id` 등록 (권장)

랜딩은 랭킹 조회를 위해 **공용 뷰어 계정**(CustomId `web_public_viewer`)으로 로그인합니다.
이 계정은 누구나 로그인할 수 있으므로, 누군가 여기에 `DisplayName`을 붙이면 익명 방문자가
그 이름으로 글을 쓸 수 있게 됩니다. 해당 PlayFabId를 등록해 원천 차단합니다.

1. PlayFabId 확인 — 대시보드 → **Players** → 검색창에 `web_public_viewer` (Custom ID로 검색)
2. **Content → Title Data → TITLE INTERNAL DATA → New Internal Title Data**
   - Key: `web_viewer_id`
   - Value: 위에서 확인한 PlayFabId
3. Save

> 미등록 상태여도 이름 없는 계정은 `no_name`에서 걸려 글을 쓸 수 없습니다.
> 이 등록은 "누군가 그 계정에 이름을 붙이는" 경우에 대한 이중 방어입니다.

---

## C. Shared Group `board` / `web_link`

`server.UpdateSharedGroupData`는 **그룹이 이미 존재해야** 씁니다(classic CloudScript에는
`CreateSharedGroup`이 없음 — Client API 전용).

- **자동:** 게임이 로그인 후 `bootstrapWebGroups()`로 두 그룹을 1회 생성합니다
  ("이미 존재" 에러는 무시). → **수동 작업 없음.** 새 클라 배포 후 아무나 한 번 로그인하면 생성됩니다.
- 그 전까지 웹 게시판은 `no_group`으로 빈 보드가 표시됩니다(에러 아님).

---

## D. 마스터 스위치 (선택)

일반 **Title Data**(Internal 아님) 키 `board_enabled` 를 `0` / `false` / `off` 로 두면
글쓰기가 차단됩니다. 키가 없으면 활성(fail-open, `award_enabled`와 동일 정책).

---

## 검증 (Run 버튼)

```
FunctionName: issueWebToken
FunctionParameter: {}
```
- 기대: `{ "ok": true, "token": "...", "ttl": 300000 }`
- `{"ok":false,"reason":"no_name"}` → Run 실행 계정에 DisplayName이 없는 정상 동작.
  실제 검증은 이름이 설정된 플레이어로 앱에서 💬 버튼을 눌러 확인하세요.

```
FunctionName: writeBoardPost
FunctionParameter: { "text": "hello board" }
```
- 기대: `{ "ok": true, "id": "...", "name": "..." }`
- 필터 확인: `"text":"http://x.com"` → `reason:"link"` / `"text":"씨발"` → `reason:"profanity"`
- `{"ok":false,"reason":"no_group"}` → C 참조 (클라 부트스트랩 전)

---

## 배포 체크리스트

- [ ] CloudScript 파일 전체 교체 후 Save & deploy
- [ ] Title Internal Data `web_viewer_id` 등록
- [ ] 새 클라 1회 로그인 → `board` / `web_link` 그룹 생성 확인
- [ ] 앱 💬 버튼 → 브라우저 열림 → 주소창에 `?wt=` 가 **남아있지 않은지** 확인
- [ ] 웹에서 글 작성 성공 + 이름/국기 표시 확인
- [ ] 시크릿 창(세션키 없음)으로 열어 **쓰기 폼 대신 설치 카드**가 뜨는지 확인
