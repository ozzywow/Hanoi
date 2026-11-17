import { describe, expect, it } from 'vitest';
import { HanoiGame, Pole, minMoves } from '../src/core/hanoi';

/** 재귀 최적해를 tap 시퀀스로 실행 */
function solve(g: HanoiGame, n: number, from: Pole, to: Pole, via: Pole): void {
  if (n === 0) return;
  solve(g, n - 1, from, via, to);
  g.tap(from);
  g.tap(to);
  solve(g, n - 1, via, to, from);
}

describe('HanoiGame', () => {
  it('시작 상태: 폴 0에 큰 것부터 쌓임', () => {
    const g = new HanoiGame(3);
    expect(g.poles).toEqual([[3, 2, 1], [], []]);
    expect(g.moves).toBe(0);
    expect(g.solved).toBe(false);
  });

  it('잘못된 레벨은 예외', () => {
    expect(() => new HanoiGame(0)).toThrow();
    expect(() => new HanoiGame(11)).toThrow();
    expect(() => new HanoiGame(3.5)).toThrow();
  });

  it('선택 → 이동 성공', () => {
    const g = new HanoiGame(3);
    expect(g.tap(0)).toEqual({ kind: 'select', pole: 0 });
    expect(g.selected).toBe(0);
    expect(g.tap(1)).toEqual({ kind: 'move', from: 0, to: 1, disc: 1, solved: false });
    expect(g.poles).toEqual([[3, 2], [1], []]);
    expect(g.moves).toBe(1);
    expect(g.selected).toBeNull();
  });

  it('같은 폴 재탭은 선택 취소이며 이동 횟수 불변', () => {
    const g = new HanoiGame(3);
    g.tap(0);
    expect(g.tap(0)).toEqual({ kind: 'deselect', pole: 0 });
    expect(g.selected).toBeNull();
    expect(g.moves).toBe(0);
  });

  it('큰 디스크를 작은 디스크 위로는 실패, 선택은 해제', () => {
    const g = new HanoiGame(3);
    g.tap(0);
    g.tap(1); // 1을 폴1로
    g.tap(0); // 2 선택
    expect(g.tap(1)).toEqual({ kind: 'fail', from: 0, to: 1 });
    expect(g.selected).toBeNull();
    expect(g.moves).toBe(1);
    expect(g.poles).toEqual([[3, 2], [1], []]);
  });

  it('빈 폴 선택은 실패', () => {
    const g = new HanoiGame(3);
    expect(g.tap(1)).toEqual({ kind: 'fail', from: null, to: 1 });
    expect(g.selected).toBeNull();
  });

  it('drop 은 선택→이동과 동일', () => {
    const g = new HanoiGame(3);
    expect(g.drop(0, 2)).toEqual({ kind: 'move', from: 0, to: 2, disc: 1, solved: false });
    expect(g.drop(0, 2)).toEqual({ kind: 'fail', from: 0, to: 2 }); // 2를 1 위로 불가
    expect(g.drop(1, 0)).toEqual({ kind: 'fail', from: null, to: 0 }); // 빈 폴에서 드래그
  });

  it.each([3, 4])('레벨 %i 최적해: 2^n-1 이동에 완료, 목표는 폴 2', (n) => {
    const g = new HanoiGame(n);
    solve(g, n, 0, 2, 1);
    expect(g.solved).toBe(true);
    expect(g.moves).toBe(minMoves(n));
    expect(g.poles[2]).toEqual(Array.from({ length: n }, (_, i) => n - i));
  });

  it('완료 후 입력은 무시', () => {
    const g = new HanoiGame(3);
    solve(g, 3, 0, 2, 1);
    expect(g.tap(0)).toEqual({ kind: 'ignored' });
    expect(g.drop(2, 0)).toEqual({ kind: 'ignored' });
    expect(g.moves).toBe(7);
  });

  it('마지막 이동의 결과에 solved=true', () => {
    const g = new HanoiGame(3);
    let last;
    const orig = g.tap.bind(g);
    g.tap = (p: Pole) => (last = orig(p));
    solve(g, 3, 0, 2, 1);
    expect(last).toMatchObject({ kind: 'move', solved: true });
  });

  it('minMoves', () => {
    expect(minMoves(3)).toBe(7);
    expect(minMoves(4)).toBe(15);
  });
});
