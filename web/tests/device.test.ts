import { describe, expect, it } from 'vitest';
import { detectKeyboardUi } from '../src/core/device';

/** 쿼리 문자열에 따라 결과를 돌려주는 가짜 matchMedia */
const fake = (caps: { hover: boolean; fine: boolean }) => (q: string) => ({
  matches: q === '(hover: hover) and (pointer: fine)' ? caps.hover && caps.fine : false,
});

describe('detectKeyboardUi', () => {
  it('데스크톱(마우스: hover + fine pointer) → 키보드 UI 표시', () => {
    expect(detectKeyboardUi(fake({ hover: true, fine: true }))).toBe(true);
  });
  it('폰/태블릿(터치: hover none, coarse pointer) → 숨김', () => {
    expect(detectKeyboardUi(fake({ hover: false, fine: false }))).toBe(false);
  });
  it('hover 만 되고 pointer 가 거친 기기, 또는 그 반대 → 숨김', () => {
    expect(detectKeyboardUi(fake({ hover: true, fine: false }))).toBe(false);
    expect(detectKeyboardUi(fake({ hover: false, fine: true }))).toBe(false);
  });
  it('matchMedia 가 없거나 예외를 던지면 숨김(안전한 쪽)', () => {
    expect(detectKeyboardUi(undefined)).toBe(false);
    expect(
      detectKeyboardUi(() => {
        throw new Error('boom');
      }),
    ).toBe(false);
  });
});
