// 하노이 규칙/상태 (순수 로직 — 렌더링·입력·시간과 무관).
// 앱(PlayScene.cpp)의 규칙과 동일:
//  - 시작: 모든 디스크가 폴 0에 쌓임, 목표: 폴 2 (CheckSuccess = 폴 0,1이 비면 완료)
//  - 큰 디스크를 작은 디스크 위로 옮길 수 없음 (IsAbleToMoveDiscus)
//  - 같은 폴 재탭 = 선택 취소(DESELECT), 이동 불가 = 실패(FAIL, 선택 해제)
//  - 이동 횟수는 성공 이동만 센다
// 디스크 크기: 1 = 가장 작음 … n = 가장 큼.

export type Pole = 0 | 1 | 2;
export const POLE_COUNT = 3;
export const TARGET_POLE: Pole = 2;

/** 웹 체험판에서 열어 주는 레벨(= 디스크 수). 앱은 3~10, 웹은 3~4 (docs/web_port_plan.md D3) */
export const MIN_LEVEL = 3;
export const MAX_WEB_LEVEL = 4;
export const MAX_APP_LEVEL = 10;

export type TapResult =
  | { kind: 'select'; pole: Pole }
  | { kind: 'deselect'; pole: Pole }
  | { kind: 'move'; from: Pole; to: Pole; disc: number; solved: boolean }
  | { kind: 'fail'; from: Pole | null; to: Pole }
  | { kind: 'ignored' }; // 이미 완료된 판 등

export function minMoves(level: number): number {
  return 2 ** level - 1;
}

export class HanoiGame {
  readonly level: number;
  /** poles[p] = 아래→위 순서의 디스크 크기 배열 */
  readonly poles: number[][];
  private _moves = 0;
  private _selected: Pole | null = null;
  private _solved = false;

  constructor(level: number) {
    if (!Number.isInteger(level) || level < 1 || level > MAX_APP_LEVEL) {
      throw new RangeError(`invalid level: ${level}`);
    }
    this.level = level;
    this.poles = [[], [], []];
    for (let size = level; size >= 1; size--) this.poles[0].push(size);
  }

  get moves(): number {
    return this._moves;
  }
  /** 현재 선택된 폴(없으면 null). 선택된 폴의 맨 위 디스크가 이동 대상 */
  get selected(): Pole | null {
    return this._selected;
  }
  get solved(): boolean {
    return this._solved;
  }

  top(pole: Pole): number | null {
    const p = this.poles[pole];
    return p.length ? p[p.length - 1] : null;
  }

  canMove(from: Pole, to: Pole): boolean {
    if (from === to) return false;
    const disc = this.top(from);
    if (disc === null) return false;
    const dest = this.top(to);
    return dest === null || disc < dest;
  }

  /** 폴 탭(클릭) 처리 — 앱의 선택/이동/취소/실패 흐름과 동일 */
  tap(pole: Pole): TapResult {
    if (this._solved) return { kind: 'ignored' };

    if (this._selected === null) {
      if (this.top(pole) === null) return { kind: 'fail', from: null, to: pole }; // 빈 폴 선택
      this._selected = pole;
      return { kind: 'select', pole };
    }

    const from = this._selected;
    if (from === pole) {
      this._selected = null;
      return { kind: 'deselect', pole };
    }
    this._selected = null; // 성공/실패 모두 선택 해제(앱: 라이트 OFF)
    if (!this.canMove(from, pole)) return { kind: 'fail', from, to: pole };

    const disc = this.poles[from].pop() as number;
    this.poles[pole].push(disc);
    this._moves++;
    this._solved = this.poles[0].length === 0 && this.poles[1].length === 0;
    return { kind: 'move', from, to: pole, disc, solved: this._solved };
  }

  /** 드래그&드롭: from 폴의 맨 위 디스크를 to 폴에 놓기. tap 두 번(선택→목적)과 같은 결과 */
  drop(from: Pole, to: Pole): TapResult {
    if (this._solved) return { kind: 'ignored' };
    if (this._selected !== null) this._selected = null;
    if (this.top(from) === null) return { kind: 'fail', from: null, to };
    this._selected = from;
    return this.tap(to);
  }

  /** 선택 해제(포커스 이탈 등). */
  clearSelection(): void {
    this._selected = null;
  }
}
