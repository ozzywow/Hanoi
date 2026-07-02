# Phase 0 — PlayFab 대시보드 배포 가이드

랭킹 Top10 수상소감 기능의 **서버측 준비**입니다. 클라(Phase 1~)와 독립적으로 먼저 올려두면 됩니다.
대상: PlayFab Title **`119C4E`** (classic CloudScript 기준 — 기존 `maintainLeaderboards`와 동일 리비전).

배포 항목 3가지:
1. 금칙어 목록 → **Title Data**
2. 수상소감 핸들러 → **CloudScript**
3. Shared Group 8개 → **클라 부트스트랩이 자동 생성** (수동 작업 없음)

---

## A. 금칙어 목록 → Title Data

1. (선택) 목록 갱신이 필요하면 재생성:
   ```
   cd server
   python build_banned_words.py
   ```
   → `server/banned_words.json` 생성 (현재 10개 언어 · 약 1,432단어 · 18KB).
   대상 언어 조정은 스크립트 상단 `LANGS`, 한국어 보강은 `banned_extra_ko.txt`.

2. PlayFab 대시보드 → **Content → Title Data 탭 → 아래쪽 "TITLE INTERNAL DATA" →
   New Internal Title Data** (⚠️ 위쪽 일반 Title Data 아님)
   - **Key**: `banned_words`
   - **Value**: `banned_words.json` **파일 내용 전체**(JSON 배열 문자열)를 그대로 붙여넣기
   - Save

> **왜 Internal?** 일반 Title Data는 클라이언트도 `GetTitleData`로 읽을 수 있어 금칙어
> 목록이 노출됩니다. Internal은 서버(CloudScript)만 읽으므로 필터 목록을 숨길 수 있습니다.
> CloudScript는 `server.GetTitleInternalData`로 로드합니다.
> 운영 중 단어 추가/삭제는 여기 Value만 수정하면 됩니다 (CloudScript 재배포 불필요, 18KB 여유 충분).

---

## B. 수상소감 핸들러 → CloudScript

`cloudscript.js`에 아래 핸들러가 **추가**되어 있습니다 (기존 것 유지):
- `handlers.writeAwardComment` — 작성/수정 (길이·링크·욕설 재검증 + 본인 키 저장)
- `handlers.deleteAwardComment` — 삭제

배포:
1. 대시보드 → **Automation → Cloud Script → Revisions**
2. 편집창 내용을 **`cloudscript.js` 파일 전체로 교체** — ⚠️ 일부만 붙이면 기존
   `maintainLeaderboards` / `resetAllLeaderboards`가 사라집니다. **반드시 파일 전체**.
3. **Save and deploy revision** (Deployed 리비전으로 활성화)

검증(Run 버튼):
- FunctionName: `writeAwardComment`
- FunctionParameter:
  ```json
  { "level": 5, "comment": "테스트 소감" }
  ```
- 기대 결과: `{ "ok": true, "level": 5, "comment": "테스트 소감" }`
  - 만약 `{"ok":false,"reason":"no_group"}` → 아직 `awards_L05` 그룹이 없다는 뜻.
    Phase 1 클라 부트스트랩 후 해결됨. 대시보드에서 즉시 테스트하려면 아래 C-2 수동 생성.

필터 동작 확인:
- `"comment":"http://x.com"` → `{"ok":false,"reason":"link"}`
- `"comment":"씨발"` → `{"ok":false,"reason":"profanity"}`
- 31자 이상 → `{"ok":false,"reason":"too_long"}`

---

## C. Shared Group `awards_L03` ~ `awards_L10`

`server.UpdateSharedGroupData`는 **그룹이 이미 존재해야** 씁니다(서버는 그룹 생성 불가).

- **C-1 (권장·자동):** Phase 1에서 클라가 로그인 후 `CreateSharedGroup`을 L03~L10에
  대해 1회 시도하고, "이미 존재" 에러는 무시합니다. → **수동 작업 없음.**

- **C-2 (대시보드 즉시 테스트용, 선택):** Phase 1 전에 B의 Run을 통과시키고 싶다면
  임시로 클라 없이 그룹을 만들 수 있습니다. classic CloudScript에는 CreateSharedGroup이
  없으므로, 임시로 **Client API `CreateSharedGroup`**을 Postman 등으로 호출하거나
  Phase 1 부트스트랩을 먼저 붙이는 편이 간단합니다. (없어도 Phase 1에서 자동 해결)

---

## 배포 체크리스트

- [ ] Title Data `banned_words` 등록
- [ ] CloudScript 파일 전체 교체 후 Save & deploy
- [ ] Run으로 `writeAwardComment` ok:true 확인 (그룹 생성 후)
- [ ] Run으로 link/profanity/too_long 거부 확인

완료되면 Phase 1(클라 계층)에서 부트스트랩·조회·작성 API를 붙입니다.
