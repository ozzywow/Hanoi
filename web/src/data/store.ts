// 브라우저 로컬 저장(최고 기록). 홍보용 웹은 서버에 쓰지 않는다 (docs/web_port_plan.md §0).
// localStorage 는 시크릿 모드/차단 시 예외가 나므로 모든 접근을 try/catch 로 감싼다.

function key(level: number): string {
  return `hanoi_web_best_L${level}`;
}

export function loadBest(level: number): number | null {
  try {
    const v = localStorage.getItem(key(level));
    if (!v) return null;
    const n = parseInt(v, 10);
    return Number.isFinite(n) && n > 0 ? n : null;
  } catch {
    return null;
  }
}

export function saveBest(level: number, timeMs: number): void {
  try {
    localStorage.setItem(key(level), String(Math.round(timeMs)));
  } catch {
    /* 저장 불가 환경 — 무시 */
  }
}

const GUIDE_KEY = 'hanoi_web_seen_guide';

/** 첫 방문 안내(규칙 이미지 + 하단 가이드)를 이미 봤는지 */
export function hasSeenGuide(): boolean {
  try {
    return localStorage.getItem(GUIDE_KEY) === '1';
  } catch {
    return false;
  }
}

export function markGuideSeen(): void {
  try {
    localStorage.setItem(GUIDE_KEY, '1');
  } catch {
    /* 무시 */
  }
}
