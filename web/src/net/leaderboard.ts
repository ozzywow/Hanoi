// 읽기 전용 랭킹 (앱 LeaderboardManager::fetchLeaderboard 포팅).
// 규칙(앱과 동일):
//  - PlayFab 리더보드는 값 내림차순 → 타임어택은 raw ms 가 작을수록 상위이므로, 20개를 받아 오름차순 정렬 후 자른다
//  - 0(미기록) 또는 3599590 이상(만료 표식)은 제외
//  - 순위는 반드시 시간 값(ms) 기준. PlayFab Position 은 쓰지 않는다
//  - ProfileConstraints 는 ShowLocations + ShowDisplayName 동시 지정

import { pfPost } from './playfab';

export interface RankEntry {
  rank: number;
  name: string;
  cc: string; // ISO 2자리 소문자, 없으면 ''
  ms: number;
}

interface LbRow {
  StatValue: number;
  DisplayName?: string;
  Profile?: { DisplayName?: string; Locations?: { CountryCode?: string }[] };
}

const CACHE_TTL_MS = 10 * 60 * 1000;
const FETCH_COUNT = 20;
const EXPIRED_VAL = 3599590;
const cache = new Map<number, { at: number; entries: RankEntry[] }>();

export function statName(level: number): string {
  return `BestTime_L${String(level).padStart(2, '0')}`;
}

export function parseLeaderboard(rows: LbRow[], max: number): RankEntry[] {
  const list: Omit<RankEntry, 'rank'>[] = [];
  for (const r of rows) {
    const ms = r.StatValue | 0;
    if (ms === 0 || ms >= EXPIRED_VAL) continue;
    const name = r.DisplayName ?? r.Profile?.DisplayName ?? 'Player';
    const cc = (r.Profile?.Locations?.[0]?.CountryCode ?? '').toLowerCase();
    list.push({ name, cc, ms });
  }
  list.sort((a, b) => a.ms - b.ms);
  return list.slice(0, max).map((e, i) => ({ ...e, rank: i + 1 }));
}

/** 레벨별 상위 랭킹. 실패하면 null (호출 측은 홍보 문구로 대체) */
export async function fetchTop(level: number, max = 10): Promise<RankEntry[] | null> {
  const hit = cache.get(level);
  if (hit && Date.now() - hit.at < CACHE_TTL_MS) return hit.entries.slice(0, max);
  try {
    const data = await pfPost<{ Leaderboard: LbRow[] }>('/Client/GetLeaderboard', {
      StatisticName: statName(level),
      MaxResultsCount: FETCH_COUNT,
      StartPosition: 0,
      ProfileConstraints: { ShowLocations: true, ShowDisplayName: true },
    });
    const entries = parseLeaderboard(data.Leaderboard ?? [], FETCH_COUNT);
    cache.set(level, { at: Date.now(), entries });
    return entries.slice(0, max);
  } catch (e) {
    console.warn('[leaderboard] fetch failed', e);
    return null;
  }
}

/** 내 기록이 상위 max 안에 들면 예상 순위, 아니면 null (시간 값 비교 — Position 사용 금지) */
export function estimateRank(entries: RankEntry[], ms: number, max = 10): number | null {
  const rank = entries.filter((e) => e.ms < ms).length + 1;
  return rank <= max ? rank : null;
}
