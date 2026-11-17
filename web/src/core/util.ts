// common_define.cpp 의 순수 유틸 포팅 (Classes/common_define.cpp).
// 앱과 같은 출력이어야 하므로 로직을 바꾸지 말 것 — tests/util.test.ts 가 기준 값을 고정한다.

export interface RecordTime {
  min: number;
  sec: number;
  /** 센티초(0~99). 밀리초가 아님 — 앱과 동일 */
  ms: number;
}

/** ms → 분/초/센티초. 앱 getRecordTime 과 동일(정수 나눗셈). */
export function getRecordTime(timeMs: number): RecordTime {
  const t = Math.max(0, Math.trunc(timeMs));
  return {
    min: Math.floor(t / 60000),
    sec: Math.floor((t % 60000) / 1000),
    ms: Math.floor(((t % 60000) % 1000) / 10),
  };
}

/** ISO 2자리 국가 코드 → 국기 이모지. 2자리가 아니면 입력을 그대로 반환(앱과 동일).
 *  ※ Windows 크롬/엣지는 국기 이모지를 지역 문자(KR)로 보여 준다 → 화면에는 PNG 폴백을 쓸 것(Phase 2'). */
export function countryToFlag(cc: string): string {
  if (cc.length !== 2) return cc;
  const up = cc.toUpperCase();
  const a = 0x1f1e6 + (up.charCodeAt(0) - 65);
  const b = 0x1f1e6 + (up.charCodeAt(1) - 65);
  return String.fromCodePoint(a, b);
}

/** 좋아요 수 축약 — 버림(반올림 없음). 1267 → "1.2K", 999 → "999". */
export function formatLikeCount(n: number): string {
  if (n < 1000) return String(n);
  const tenths = Math.floor(n / 100);
  return `${Math.floor(tenths / 10)}.${tenths % 10}K`;
}

/** "초.센티초" 표기 (예: 68.42s). 공유 문구/결과 화면용. */
export function formatSeconds(timeMs: number): string {
  const rt = getRecordTime(timeMs);
  const sec = rt.min * 60 + rt.sec;
  return `${sec}.${String(rt.ms).padStart(2, '0')}s`;
}

/** 공유 문구. rank<=0 이면 순위 문장 생략, level<3 또는 timeMs<=0 이면 일반 초대 문구. 앱 makeShareText 와 동일. */
export function makeShareText(level: number, timeMs: number, rank: number): string {
  if (level >= 3 && timeMs > 0) {
    const rt = getRecordTime(timeMs);
    const sec = rt.min * 60 + rt.sec;
    const cs = String(rt.ms).padStart(2, '0');
    const head =
      rank > 0
        ? `\u{1F3C6} I'm #${rank} in the world — Level ${level} in ${sec}.${cs}s`
        : `\u{1F3C6} Level ${level} cleared in ${sec}.${cs}s — Tower of Hanoi Speedrun`;
    return `${head}\nThink you're faster?`;
  }
  return 'Tower of Hanoi - Speedrun — race the clock, climb the world rankings.';
}

const GREETINGS: Record<string, string> = {
  ko: '안녕하세요', en: 'Hello', ja: 'こんにちは', zh: '你好', fr: 'Bonjour', es: 'Hola', de: 'Hallo',
  it: 'Ciao', pt: 'Olá', ru: 'Привет', ar: 'مرحبا', th: 'สวัสดี', nl: 'Hallo', pl: 'Cześć', tr: 'Merhaba',
  no: 'Hei', nb: 'Hei', nn: 'Hei', hu: 'Helló', hi: 'नमस्ते', sv: 'Hej', id: 'Halo', vi: 'Xin chào',
};

/** 브라우저 언어에 맞는 인삿말 (앱 getLocalGreeting + greetings.plist). 없으면 "Hello" */
export function getLocalGreeting(lang: string = typeof navigator !== 'undefined' ? navigator.language : 'en'): string {
  const base = lang.toLowerCase().split(/[-_]/)[0];
  return GREETINGS[base] ?? 'Hello';
}
