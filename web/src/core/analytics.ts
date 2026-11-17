// 이용 지표(시작/완주/스토어 클릭) 집계 — 대략치용. docs/web_port_plan.md §0.
// 공용 뷰어 계정으로 PlayFab 합계(Sum) 통계를 +1 한다. 통계(web_start/web_finish/web_cta)는
// PlayFab Game Manager 에서 집계 방식 "Sum" 으로 미리 만들어 둬야 한다.
// ⚠ 정의가 없는 이름으로 쓰면 PlayFab 이 Last 집계 통계를 자동 생성해 버린다 — 배포 전 3개가 존재하는지 확인할 것.
// (스토어 클릭은 이름이 꼬인 web_cta 대신 web_store 를 쓴다.)
// 개발 중(npm run dev)에는 집계를 부풀리지 않도록 보내지 않는다.

import { pfPost } from '../net/playfab';

export type WebEvent = 'start' | 'finish' | 'cta';

const STAT_NAME: Record<WebEvent, string> = {
  start: 'web_start',
  finish: 'web_finish',
  cta: 'web_store',
};

export function track(event: WebEvent): void {
  if (import.meta.env.DEV) {
    console.debug('[analytics]', event);
    return;
  }
  void pfPost('/Client/UpdatePlayerStatistics', {
    Statistics: [{ StatisticName: STAT_NAME[event], Value: 1 }],
  }).catch(() => {
    /* 지표 실패는 게임에 영향 없음 */
  });
}
