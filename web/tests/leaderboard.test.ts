import { describe, expect, it } from 'vitest';
import { estimateRank, parseLeaderboard, statName } from '../src/net/leaderboard';

describe('parseLeaderboard', () => {
  const rows = [
    { StatValue: 90000, DisplayName: 'slow', Profile: { Locations: [{ CountryCode: 'US' }] } },
    { StatValue: 0, DisplayName: 'none' },
    { StatValue: 3599590, DisplayName: 'expired' },
    { StatValue: 12340, Profile: { DisplayName: 'fast', Locations: [{ CountryCode: 'KR' }] } },
    { StatValue: 50000 },
  ];
  it('0/만료 제외, 시간 오름차순, 순위 부여, 국가 소문자', () => {
    const r = parseLeaderboard(rows, 10);
    expect(r.map((e) => e.name)).toEqual(['fast', 'Player', 'slow']);
    expect(r.map((e) => e.rank)).toEqual([1, 2, 3]);
    expect(r[0].cc).toBe('kr');
    expect(r[1].cc).toBe('');
  });
  it('max 로 자른다', () => {
    expect(parseLeaderboard(rows, 2)).toHaveLength(2);
  });
});

describe('estimateRank', () => {
  const top = parseLeaderboard(
    Array.from({ length: 10 }, (_, i) => ({ StatValue: 10000 + i * 1000 })),
    10,
  );
  it('시간 값 기준 예상 순위', () => {
    expect(estimateRank(top, 9000)).toBe(1);
    expect(estimateRank(top, 10500)).toBe(2);
    expect(estimateRank(top, 19000)).toBe(10);
  });
  it('10위 밖이면 null, 목록이 10개 미만이면 빈자리 순위', () => {
    expect(estimateRank(top, 25000)).toBeNull();
    expect(estimateRank(top.slice(0, 3), 99999)).toBe(4);
  });
});

describe('statName', () => {
  it('BestTime_Lxx', () => {
    expect(statName(3)).toBe('BestTime_L03');
    expect(statName(10)).toBe('BestTime_L10');
  });
});
