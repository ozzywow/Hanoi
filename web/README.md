# Hanoi Web (홍보용 체험판)

Tower of Hanoi - Speedrun 의 PC 브라우저 체험판. 계획서: [../docs/web_port_plan.md](../docs/web_port_plan.md) (§0 범위 우선)

## 개발
```
cd web
npm install
npm run dev        # http://localhost:5173/
npm test           # 유틸 단위 테스트
npm run typecheck
npm run build      # dist/
```

## 배포
`.\deploy.ps1` — 빌드 후 gh-pages 워크트리(`../Hanoi-gh-pages`)의 `play/` 로 복사한다. 커밋/푸시는 직접.
gh-pages 의 `play/` 는 **직접 편집 금지**(master `web/` 에서만 수정).

## 구조
- `src/core/app.ts` — 캔버스 부트(960×640 레터박스·DPR)/게임 루프/씬 매니저/입력
- `src/core/util.ts` — `common_define.cpp` 유틸 포팅 (`tests/util.test.ts` 로 앱과 동일 출력 고정)
- `src/core/analytics.ts` — 이용 지표 자리(Phase 2'에서 연결)
- `src/scenes/boot.ts` — Phase 0 확인용 씬
