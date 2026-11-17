import { describe, expect, it } from 'vitest';
import { countryToFlag, formatLikeCount, formatSeconds, getLocalGreeting, getRecordTime, makeShareText } from '../src/core/util';

describe('getRecordTime', () => {
  it('ms → 분/초/센티초 (앱과 동일한 정수 나눗셈)', () => {
    expect(getRecordTime(0)).toEqual({ min: 0, sec: 0, ms: 0 });
    expect(getRecordTime(68420)).toEqual({ min: 1, sec: 8, ms: 42 });
    expect(getRecordTime(59999)).toEqual({ min: 0, sec: 59, ms: 99 });
    expect(getRecordTime(60000)).toEqual({ min: 1, sec: 0, ms: 0 });
    expect(getRecordTime(1234)).toEqual({ min: 0, sec: 1, ms: 23 });
  });
  it('음수/소수는 0·버림 처리', () => {
    expect(getRecordTime(-5)).toEqual({ min: 0, sec: 0, ms: 0 });
    expect(getRecordTime(1234.9)).toEqual({ min: 0, sec: 1, ms: 23 });
  });
});

describe('countryToFlag', () => {
  it('2자리 코드는 국기 이모지로 (대소문자 무관)', () => {
    expect(countryToFlag('kr')).toBe('\u{1F1F0}\u{1F1F7}');
    expect(countryToFlag('US')).toBe('\u{1F1FA}\u{1F1F8}');
  });
  it('2자리가 아니면 입력 그대로', () => {
    expect(countryToFlag('')).toBe('');
    expect(countryToFlag('KOR')).toBe('KOR');
  });
});

describe('formatLikeCount', () => {
  it('앱 규칙: 버림, 1000 이상은 X.YK', () => {
    expect(formatLikeCount(999)).toBe('999');
    expect(formatLikeCount(1000)).toBe('1.0K');
    expect(formatLikeCount(1267)).toBe('1.2K');
    expect(formatLikeCount(12345)).toBe('12.3K');
  });
});

describe('formatSeconds / makeShareText', () => {
  it('초.센티초 표기', () => {
    expect(formatSeconds(68420)).toBe('68.42s');
    expect(formatSeconds(5005)).toBe('5.00s');
  });
  it('순위가 있으면 순위 문장', () => {
    expect(makeShareText(4, 68420, 7)).toBe(
      "\u{1F3C6} I'm #7 in the world — Level 4 in 68.42s\nThink you're faster?",
    );
  });
  it('순위가 없으면 클리어 문장', () => {
    expect(makeShareText(3, 12340, 0)).toBe(
      "\u{1F3C6} Level 3 cleared in 12.34s — Tower of Hanoi Speedrun\nThink you're faster?",
    );
  });
  it('기록이 없거나 레벨<3이면 일반 초대 문구', () => {
    const generic = 'Tower of Hanoi - Speedrun — race the clock, climb the world rankings.';
    expect(makeShareText(0, 0, 0)).toBe(generic);
    expect(makeShareText(2, 5000, 1)).toBe(generic);
  });
});

describe('getLocalGreeting', () => {
  it('언어 코드별 인삿말, 모르면 Hello', () => {
    expect(getLocalGreeting('ko-KR')).toBe('안녕하세요');
    expect(getLocalGreeting('en-US')).toBe('Hello');
    expect(getLocalGreeting('pt_BR')).toBe('Olá');
    expect(getLocalGreeting('xx')).toBe('Hello');
  });
});
