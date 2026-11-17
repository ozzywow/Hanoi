// 입력 장치 판별. 키보드 안내(키캡 힌트·안내 카드 등)는 마우스/키보드가 주 입력인 환경에서만 보여 준다.

interface MediaQueryLike {
  matches: boolean;
}

/** 주 입력이 정밀 포인터(마우스/트랙패드)이고 hover 가 가능하면 키보드 UI 를 보여 준다.
 *  폰/태블릿(터치 주 입력: pointer coarse, hover none)은 false. matchMedia 를 못 쓰면 false(안전한 쪽). */
export function detectKeyboardUi(
  matchMedia: ((query: string) => MediaQueryLike) | undefined = typeof window !== 'undefined' ? window.matchMedia?.bind(window) : undefined,
): boolean {
  if (!matchMedia) return false;
  try {
    return matchMedia('(hover: hover) and (pointer: fine)').matches;
  } catch {
    return false;
  }
}
