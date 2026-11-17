// 플레이 씬 (앱 PlayScene 포팅 — 초안). 모든 좌표는 앱의 디자인 공간(480×320, y↑)을 그대로 쓴다.
// 초안 범위: 레이아웃/PNG/상태 흐름(대기→카운트다운→플레이→결과)/입력(탭·드래그)/결과 팝업.
// 아직 없음: 효과음, BGM, 이퀄라이저, 랭킹 티커/시상대, 리플레이.

import { PointerInfo, Scene } from '../core/app';
import { BGM_LIST, Sound } from '../audio/sound';
import { track } from '../core/analytics';
import { Images } from '../core/assets';
import { HanoiGame, MAX_APP_LEVEL, MAX_WEB_LEVEL, MIN_LEVEL, Pole, TapResult } from '../core/hanoi';
import { detectKeyboardUi } from '../core/device';
import { getLocalGreeting, getRecordTime } from '../core/util';
import { estimateRank, fetchTop, RankEntry } from '../net/leaderboard';
import { hasSeenGuide, loadBest, markGuideSeen, saveBest } from '../data/store';
import { beginDesign, image, keycap, ledPanel, measure, MINT, outlinedText, resetIcon, rgb255, rgba, text, tinted } from '../draw/led';
import { drawFlag, flagWidth } from '../draw/flags';
import { drawPixelText } from '../draw/pixelFont';

export const IMAGE_LIST = [
  'main_bg.png', 'block_selectEff.png', 'btn_home_n.png',
  'block0.png', 'block2.png', 'block3.png', 'block4.png', 'block6.png',
  'arrow.png', 'text_en.png',
];

// 한국어 체험판(/play/ko/)에서는 홈도 한국어 랜딩(/ko/)으로 보낸다 — 언어가 영어로 튕기지 않도록
const IS_KO = typeof location !== 'undefined' && /\/play\/ko(\/|$)/.test(location.pathname);
const SHARE_URL = IS_KO ? 'https://ozzywow.github.io/Hanoi/ko/' : 'https://ozzywow.github.io/Hanoi/';
const APP_STORE_URL = 'https://apps.apple.com/app/id430261581';
const PLAY_STORE_URL = 'https://play.google.com/store/apps/details?id=com.ozzywow.hanoi';

/** iOS(iPadOS 포함)/Android 는 해당 스토어, 그 외(PC·판별 불가)는 SHARE_URL. */
function storeUrlForDevice(): string {
  try {
    const ua = navigator.userAgent || '';
    if (/Android/i.test(ua)) return PLAY_STORE_URL;
    if (/iPhone|iPad|iPod/i.test(ua)) return APP_STORE_URL;
    // iPadOS 13+ 는 데스크톱 Safari UA(Macintosh)로 위장 — 터치 지원 여부로 구분
    if (/Macintosh/i.test(ua) && navigator.maxTouchPoints > 1) return APP_STORE_URL;
  } catch {
    /* 판별 실패 → 홈 */
  }
  return SHARE_URL;
}

// 앱 PlayScene.cpp 상수
const POLE_X = [105, 241, 377];
const DISC_BASE_Y = 140;
const DISC_STEP_Y = 15;
const BOT_Y = 28;
const TOP_Y = 307; // RESOURCE_HEIGHT - 13
const DIV1 = (POLE_X[0] + POLE_X[1]) / 2;
const DIV2 = (POLE_X[1] + POLE_X[2]) / 2;
const DOCK_X = 24;

type State = 'ready' | 'countdown' | 'play' | 'done';
type PopupKind = 'result' | 'locked' | null;

interface Hit {
  x: number; y: number; w: number; h: number; // 중심 기준(디자인 공간, y↑)
  fn: () => void;
  quiet?: boolean; // true면 클릭음 생략
}

const IDLE_SETS: { t: string; c: string; s: number }[][] = [
  [
    { t: '🎯 LV.%L', c: rgb255(80, 220, 180), s: 10 },
    { t: '⚡ CHALLENGE', c: rgb255(255, 210, 50), s: 10 },
    { t: '🏆 READY?', c: rgb255(255, 120, 80), s: 10 },
  ],
  [
    { t: '🎮 TOWER', c: rgb255(100, 200, 255), s: 15 },
    { t: '⚡ OF', c: rgb255(255, 210, 50), s: 15 },
    { t: '🏰 HANOI', c: rgb255(255, 120, 80), s: 15 },
  ],
  [
    { t: '⏱ BEST TIME?', c: rgb255(255, 210, 50), s: 10 },
    { t: '⭐ CHALLENGE', c: rgb255(80, 220, 180), s: 10 },
    { t: 'LV.%L', c: rgb255(255, 215, 0), s: 28 },
  ],
  [
    { t: '⚡', c: rgb255(255, 210, 50), s: 30 },
    { t: 'START', c: rgb255(80, 220, 180), s: 28 },
    { t: '🏆', c: rgb255(255, 215, 0), s: 30 },
  ],
];

/** 마우스/키보드가 있는 환경(데스크톱)에서만 키보드 안내를 보여 준다 — 터치 기기에서는 숨김 */
const HAS_KEYBOARD = detectKeyboardUi();

/** 대기 중 하단 패널 순환에 끼워 넣는 키보드 안내 (START 키캡이 가운데를 덮으므로 좌/우 칸에 핵심을 둔다) */
const KEY_TIP_START: { t: string; c: string; s: number }[] = [
  { t: '⌨ PRESS SPACE', c: rgb255(80, 220, 180), s: 11 },
  { t: '⌨', c: rgb255(255, 210, 50), s: 18 },
  { t: '= START', c: rgb255(255, 120, 80), s: 12 },
];
const KEY_TIP_MOVE: { t: string; c: string; s: number }[] = [
  { t: '⌨ J · K · L', c: rgb255(80, 220, 180), s: 12 },
  { t: 'PICK → DROP', c: rgb255(255, 210, 50), s: 11 },
  { t: '= POLE A · B · C', c: rgb255(255, 120, 80), s: 10 },
];
const IDLE_ALL = HAS_KEYBOARD
  ? [IDLE_SETS[0], KEY_TIP_START, IDLE_SETS[1], IDLE_SETS[2], KEY_TIP_MOVE, IDLE_SETS[3]]
  : IDLE_SETS;

const TICKER_TEXT =
  '★  TOWER OF HANOI - SPEEDRUN  ★  Race the clock, climb the world rankings - get the app!  ★  Full levels, global rankings, ghost races & replays in the app  ★';

/** 키보드: J/K/L = 폴 A/B/C (오른손 홈 포지션) */
const KEY_POLE: Record<string, Pole> = { j: 0, k: 1, l: 2 };

type Seg = { t: string } | { cc: string };

// 앱 startGuideAnimation (첫 플레이 Lv.3 가이드): 페이지당 패널 순차 등장 → 유지 → 다음 페이지
const GUIDE_T_IN = 1.2;
const GUIDE_T_HOLD = 3.5;
const GUIDE_T_GAP = 0.6;
const GUIDE_PAGE = 2 * GUIDE_T_IN + GUIDE_T_HOLD; // 패널 3개 등장(2.4s) + 유지
const GUIDE_PAGES: { c: string; blink: number; items: { t: string; s: number }[] }[] = [
  { c: rgb255(80, 220, 180), blink: 0, items: [{ t: '👆 TOUCH HERE', s: 12 }, { t: 'TO SELECT', s: 12 }, { t: 'A DISC', s: 12 }] },
  { c: rgb255(255, 210, 50), blink: -1, items: [{ t: 'DISC SELECTED!', s: 11 }, { t: 'MOVE ➡', s: 13 }, { t: 'THIS POLE', s: 11 }] },
  { c: rgb255(255, 100, 80), blink: 2, items: [{ t: 'PLACE DISC', s: 12 }, { t: 'TAP BELOW', s: 12 }, { t: '👇 HERE!', s: 12 }] },
];

// 데스크톱이면 가이드 마지막에 키보드 페이지를 더한다
const GUIDE_KEY_PAGE = {
  c: rgb255(120, 200, 255),
  blink: -1,
  items: [{ t: '⌨ KEYBOARD', s: 12 }, { t: 'PICK → DROP', s: 12 }, { t: 'J · K · L', s: 13 }],
};
const GUIDE_ALL = HAS_KEYBOARD ? [...GUIDE_PAGES, GUIDE_KEY_PAGE] : GUIDE_PAGES;

const PODIUM_SEC = 6.5; // 앱: 5~8초 시상대 감상 후 idle 연출
const clamp01 = (v: number): number => Math.max(0, Math.min(1, v));
const trunc = (s: string, n: number): string => Array.from(s).slice(0, n).join(''); // 코드포인트 단위 자르기

const pad2 = (n: number): string => String(n).padStart(2, '0');
function fmt(ms: number): string {
  const r = getRecordTime(ms);
  return `${pad2(r.min)}:${pad2(r.sec)}.${pad2(r.ms)}`;
}

export class PlayScene implements Scene {
  private level = MIN_LEVEL;
  private game = new HanoiGame(MIN_LEVEL);
  private state: State = 'ready';

  private idleStart = performance.now();
  private tickerX = 485;
  private cdStart = 0;
  private playStart = 0;
  private finishAt = 0;
  private finalMs = 0;
  private finalRpm = 0;
  private newRecord = false;
  private touchCount = 0;

  private popup: PopupKind = null;
  private popupAt = 0;
  private hits: Hit[] = [];

  private dragFrom: Pole | null = null;
  private hover: number = -1;
  private soundOn = true;
  private keyCardSince = 0; // 키보드 안내 카드가 보이기 시작한 시각(0=숨김) — 페이드인 기준
  private lastInputKb = false; // 마지막 입력이 키보드이면 선택 후 도움말을 보여 준다

  // 사운드 / RPM / 이퀄라이저 (앱 DrawTime·_updateEqualizer 포팅)
  private bgmName = '';
  private cdStep = 0; // 카운트다운 효과음 진행(0=없음, 1..3=3·2·1, 4=GO)
  private rpmSmoothed = 0;
  private bgmCur = 0;
  private rpmTickAcc = 0;
  private eqTickAcc = 0;
  private eqH: number[][] = [new Array(6).fill(0), new Array(6).fill(0), new Array(6).fill(0)];
  private eqPeak: number[][] = [new Array(6).fill(0), new Array(6).fill(0), new Array(6).fill(0)];

  // 읽기 전용 랭킹 (null=로딩 중/실패 → 홍보 문구 티커)
  private ranks: RankEntry[] | null = null;
  private ranksToken = 0;
  private tickerSegs: Seg[] = [];
  private tickerW = 0;
  private guideSeen = hasSeenGuide(); // false = 첫 방문 → Lv.3 에서 규칙 이미지/가이드 표시
  private myRank: number | null = null;
  private top10Ms: number | null = null;

  constructor(
    private readonly img: Images,
    private readonly sound: Sound,
  ) {
    this.loadRanks();
  }

  private loadRanks(): void {
    const token = ++this.ranksToken;
    const level = this.level;
    this.ranks = null;
    this.tickerSegs = [{ t: TICKER_TEXT }];
    this.tickerW = this.segsWidth(this.tickerSegs);
    void fetchTop(level, 10).then((entries) => {
      if (token !== this.ranksToken) return; // 레벨이 바뀐 뒤 도착한 응답
      if (!entries) return; // 실패 → 홍보 문구 유지
      this.ranks = entries;
      this.buildTicker();
    });
  }

  /** 앱 startRankTicker: "인삿말!  PLAYER.     1.🇰🇷name   |   2.…" */
  private buildTicker(): void {
    const segs: Seg[] = [{ t: `${getLocalGreeting()}!  PLAYER.          ` }];
    const entries = this.ranks ?? [];
    if (entries.length === 0) {
      segs.push({ t: '---  BE THE FIRST TO RANK!  ---' });
    } else {
      entries.forEach((e, i) => {
        if (i > 0) segs.push({ t: '   |   ' });
        segs.push({ t: `${e.rank}.` });
        if (e.cc) segs.push({ cc: e.cc });
        segs.push({ t: e.name });
      });
    }
    this.tickerSegs = segs;
    this.tickerW = this.segsWidth(segs);
    this.tickerX = 485;
  }

  private segsWidth(segs: Seg[]): number {
    const c = this.dummyCtx();
    return segs.reduce((w, sg) => w + ('t' in sg ? measure(c, sg.t, 11) : flagWidth(sg.cc, 11) + 2), 0);
  }

  private drawSegs(ctx: CanvasRenderingContext2D, segs: Seg[], x: number, y: number, size: number): void {
    let cx = x;
    for (const sg of segs) {
      if ('t' in sg) {
        text(ctx, sg.t, cx, y, size, '#fff', 'left');
        cx += measure(ctx, sg.t, size);
      } else {
        drawFlag(ctx, sg.cc, cx, y, size);
        cx += flagWidth(sg.cc, size) + 2;
      }
    }
  }

  // ── 상태 전이 ─────────────────────────────────────────────────────────────
  private resetToReady(level = this.level): void {
    this.level = level;
    this.game = new HanoiGame(level);
    this.state = 'ready';
    this.popup = null;
    this.dragFrom = null;
    this.hover = -1;
    this.touchCount = 0;
    this.idleStart = performance.now();
    this.sound.stopBgm();
    this.resetPlayFx();
    this.loadRanks(); // 캐시 우선 — 레벨 변경/재도전 시 최신 목록으로 시상대·티커 재생
  }

  private resetPlayFx(): void {
    this.cdStep = 0;
    this.rpmSmoothed = 0;
    this.bgmCur = 0;
    this.rpmTickAcc = 0;
    this.eqTickAcc = 0;
    for (let s = 0; s < 3; s++) {
      this.eqH[s].fill(0);
      this.eqPeak[s].fill(0);
    }
  }

  private startCountdown(): void {
    if (this.state !== 'ready') return;
    this.state = 'countdown';
    this.cdStart = performance.now();
    this.guideSeen = true; // 앱: CountDown() 에서 규칙 이미지 제거 — 한 번 시작하면 다시 보이지 않는다
    markGuideSeen();
    this.resetPlayFx();
    this.sound.play('efs_start_countdown');
    // 앱: 판마다 랜덤 BGM. 카운트다운(5초) 동안 미리 로드
    this.bgmName = BGM_LIST[Math.floor(Math.random() * BGM_LIST.length)];
    this.sound.preloadBgm(this.bgmName);
    track('start');
  }

  private finish(): void {
    const now = performance.now();
    this.state = 'done';
    this.finishAt = now;
    this.finalMs = Math.max(100, Math.round(now - this.playStart));
    const rpmElapsed = Math.max(2000, now - this.playStart);
    this.finalRpm = Math.min(999, Math.round(this.touchCount / (rpmElapsed / 60000)));
    const best = loadBest(this.level);
    this.newRecord = best === null || best > this.finalMs;
    if (this.newRecord) saveBest(this.level, this.finalMs);
    // 내 기록이 앱 랭킹(상위 10) 안에 드는지 — 읽기 전용 비교(시간 값 기준)
    this.myRank = this.ranks ? estimateRank(this.ranks, this.finalMs) : null;
    this.top10Ms = this.ranks && this.ranks.length >= 10 ? this.ranks[9].ms : null;
    this.sound.fadeOutBgm(1.0);
    this.sound.play(this.newRecord ? 'efs_new_record' : 'efs_clean');
    track('finish');
  }

  /** 모바일이면 해당 스토어, PC·불분명하면 홈(랜딩). */
  private openStore(): void {
    track('cta');
    window.open(storeUrlForDevice(), '_blank', 'noopener');
  }

  private changeLevel(delta: number): void {
    const next = this.level + delta;
    if (next < MIN_LEVEL) return;
    if (next > MAX_WEB_LEVEL) {
      this.popup = 'locked';
      this.popupAt = performance.now();
      return;
    }
    this.resetToReady(next);
  }

  // ── 업데이트 ──────────────────────────────────────────────────────────────
  update(dt: number): void {
    const now = performance.now();
    if (this.state === 'ready') {
      this.tickerX -= 80 * dt; // 앱 SPEED = 80
      if (this.tickerX < -(this.tickerW + 5)) this.tickerX = 485;
    } else if (this.state === 'countdown') {
      const t = (now - this.cdStart) / 1000;
      // 3·2·1 효과음 (t=2,3,4), GO 효과음 + 플레이 시작 (t=5)
      while (this.cdStep < 3 && t >= 2 + this.cdStep) {
        this.cdStep++;
        this.sound.play('efs_count_sec');
      }
      if (t >= 5) {
        this.cdStep = 4;
        this.sound.play('efs_go');
        this.state = 'play';
        this.playStart = now;
        this.touchCount = 0;
        this.rpmSmoothed = 0;
        this.bgmCur = 0;
        this.rpmTickAcc = 0;
        void this.sound.startBgm(this.bgmName);
      }
    } else if (this.state === 'play') {
      this.updateRpm(dt, now);
      this.updateEqualizer(dt, now);
    } else if (this.state === 'done') {
      if (this.popup === null && now - this.finishAt >= 2000) {
        this.popup = 'result';
        this.popupAt = now;
        this.sound.play('efs_click');
      }
    }
  }

  /** 앱 DrawTime: 0.1초 틱마다 RPM EMA 갱신 + BGM 목표 볼륨 추종 */
  private updateRpm(dt: number, now: number): void {
    this.rpmTickAcc += dt;
    while (this.rpmTickAcc >= 0.1) {
      this.rpmTickAcc -= 0.1;
      const elapsed = Math.max(2000, now - this.playStart);
      const raw = Math.min(this.touchCount / (elapsed / 60000), 200);
      const alpha = raw > this.rpmSmoothed ? 0.05 : 0.1; // 비대칭 EMA
      this.rpmSmoothed = this.rpmSmoothed * (1 - alpha) + raw * alpha;

      // 목표 볼륨: RPM<=50→0, >=100→1, 중간 선형. 0.02/틱씩 추종
      const rpm = this.rpmSmoothed;
      const target = rpm >= 100 ? 1 : rpm > 50 ? (rpm - 50) / 50 : 0;
      if (this.bgmCur < target) this.bgmCur = Math.min(target, this.bgmCur + 0.02);
      else if (this.bgmCur > target) this.bgmCur = Math.max(target, this.bgmCur - 0.02);
      this.sound.setBgmVolume(this.bgmCur);
    }
  }

  /** 앱 _updateEqualizer: 0.05초 틱, 막대 낙하 + 볼륨 비례 랜덤 kick */
  private updateEqualizer(dt: number, now: number): void {
    if (now - this.playStart < 1500) return;
    this.eqTickAcc += dt;
    const BAR_MAXH = 42;
    const maxH = BAR_MAXH * this.bgmCur;
    while (this.eqTickAcc >= 0.05) {
      this.eqTickAcc -= 0.05;
      for (let s = 0; s < 3; s++) {
        for (let i = 0; i < 6; i++) {
          this.eqH[s][i] = Math.max(0, this.eqH[s][i] - 95 * 0.05);
          if (Math.random() < 0.5) {
            const kick = Math.random() * maxH;
            if (kick > this.eqH[s][i]) this.eqH[s][i] = kick;
          }
          if (this.eqH[s][i] >= this.eqPeak[s][i]) this.eqPeak[s][i] = this.eqH[s][i];
          else this.eqPeak[s][i] = Math.max(0, this.eqPeak[s][i] - 28 * 0.05);
        }
      }
    }
  }

  private _ctx: CanvasRenderingContext2D | null = null;
  private dummyCtx(): CanvasRenderingContext2D {
    if (!this._ctx) this._ctx = document.createElement('canvas').getContext('2d')!;
    return this._ctx;
  }

  // ── 입력 ──────────────────────────────────────────────────────────────────
  private poleAt(dx: number): number {
    if (dx > 15 && dx < 165) return 0;
    if (dx > 165 && dx < 315) return 1;
    if (dx > 315 && dx < 465) return 2;
    return -1;
  }

  /** 탭 결과 → 효과음/RPM 카운트 (앱: 성공/선택/취소는 카운트, 실패는 제외) */
  private applyResult(r: TapResult): void {
    switch (r.kind) {
      case 'select':
        this.touchCount++;
        this.sound.play('efs_select_disc');
        break;
      case 'deselect':
        this.touchCount++;
        this.sound.play('efs_move_disc_ok'); // 앱: 같은 폴 재탭도 이 소리
        break;
      case 'move':
        this.touchCount++;
        this.sound.play('efs_move_disc_ok');
        if (r.solved) this.finish();
        break;
      case 'fail':
        this.sound.play('efs_cancel_select');
        break;
      default:
        break;
    }
  }

  private tapPole(pole: Pole): void {
    if (this.state !== 'play') return;
    const r = this.game.tap(pole);
    if (r.kind === 'select') this.dragFrom = pole;
    this.applyResult(r);
  }

  onPointer(p: PointerInfo): void {
    const dx = p.x / 2;
    const dy = (640 - p.y) / 2;

    if (p.phase === 'down') {
      this.sound.unlock(); // 브라우저 자동재생 정책: 첫 입력에서 오디오 시작
      this.lastInputKb = false;
      for (const h of this.hits) {
        if (Math.abs(dx - h.x) <= h.w / 2 && Math.abs(dy - h.y) <= h.h / 2) {
          if (!h.quiet) this.sound.play('efs_click');
          h.fn();
          return;
        }
      }
      if (this.popup) return; // 모달
      if (this.state === 'play') {
        const pole = this.poleAt(dx);
        if (pole >= 0) {
          this.hover = pole;
          this.tapPole(pole as Pole);
        }
      }
    } else if (p.phase === 'move') {
      if (this.dragFrom !== null) this.hover = this.poleAt(dx);
    } else if (p.phase === 'up' || p.phase === 'cancel') {
      if (this.dragFrom !== null && this.state === 'play' && p.phase === 'up') {
        const pole = this.poleAt(dx);
        if (pole >= 0 && pole !== this.dragFrom && this.game.selected === this.dragFrom) {
          this.applyResult(this.game.tap(pole as Pole));
        }
      }
      this.dragFrom = null;
      this.hover = -1;
    }
  }

  /** 키보드는 두 가지뿐: Space = START, J/K/L = 폴 A/B/C. 나머지 조작은 마우스/터치. */
  onKey(e: KeyboardEvent): void {
    this.sound.unlock();
    if (e.repeat || e.ctrlKey || e.metaKey || e.altKey || this.popup) return;
    if (this.state === 'ready' && e.key === ' ') {
      this.startCountdown();
    } else if (this.state === 'play') {
      // J/K/L 은 마우스와 같이 '선택 → 이동', 같은 폴 재입력은 선택 취소
      const pole = KEY_POLE[e.key.toLowerCase()];
      if (pole !== undefined) {
        this.lastInputKb = true;
        this.tapPole(pole);
      }
    }
  }

  // ── 그리기 ────────────────────────────────────────────────────────────────
  draw(ctx: CanvasRenderingContext2D): void {
    this.hits = [];
    const now = performance.now();
    ctx.save();
    beginDesign(ctx);

    this.drawBackground(ctx);
    this.drawDiscs(ctx);
    this.drawSelectionMark(ctx);
    if (this.guideActive()) this.drawGuideOverlay(ctx);
    this.drawTopBar(ctx, now);
    this.drawDock(ctx);
    this.drawBottomPanel(ctx, now);
    this.drawKeyHints(ctx);
    this.drawKeyCard(ctx, now);
    if (this.state === 'ready' && !this.popup) this.drawStartButton(ctx, now);
    if (this.popup) this.drawPopup(ctx, now);

    ctx.restore();
  }

  private drawBackground(ctx: CanvasRenderingContext2D): void {
    image(ctx, this.img['main_bg.png'], 240, 160);
  }

  private discImage(size: number): HTMLImageElement {
    // 앱: Discus ID = i*offset (i=0 가장 큼), offset = 10/레벨. size 1(작음)…n(큼) → i = n - size
    const offset = Math.floor(10 / this.level);
    return this.img[`block${(this.level - size) * offset}.png`];
  }

  private drawDiscs(ctx: CanvasRenderingContext2D): void {
    for (let p = 0; p < 3; p++) {
      this.game.poles[p].forEach((size, j) => {
        image(ctx, this.discImage(size), POLE_X[p], DISC_BASE_Y + DISC_STEP_Y * j);
      });
    }
  }

  private drawSelectionMark(ctx: CanvasRenderingContext2D): void {
    if (this.state !== 'play') return;
    const pole = this.dragFrom !== null && this.hover >= 0 ? this.hover : this.game.selected;
    if (pole === null || pole < 0) return;
    ctx.save();
    ctx.globalCompositeOperation = 'lighter'; // 앱: BlendFunc(SRC_ALPHA, ONE)
    ctx.globalAlpha = 100 / 255;
    image(ctx, this.img['block_selectEff.png'], POLE_X[pole], 110, 1, 0);
    ctx.restore();
  }

  private drawTopBar(ctx: CanvasRenderingContext2D, now: number): void {
    ledPanel(ctx, 240, TOP_Y, 480, 20, { border: 3 });

    if (this.state === 'ready') {
      this.drawSegs(ctx, this.tickerSegs, this.tickerX, TOP_Y, 11);
      return;
    }

    text(ctx, `LV.${this.level} / ${MAX_APP_LEVEL}`, 120, TOP_Y, 13, '#fff');

    if (this.state === 'countdown' || this.state === 'play') {
      const ms = this.state === 'play' ? now - this.playStart : 0;
      text(ctx, fmt(ms), 362, TOP_Y, 13, '#fff');
    }

    // 신호등 — 카운트다운 동안, 플레이 시작 1초 후 0.4초 페이드아웃
    const sinceStart = this.state === 'play' ? (now - this.playStart) / 1000 : -1;
    const dotAlpha = sinceStart < 1 ? 1 : Math.max(0, 1 - (sinceStart - 1) / 0.4);
    if (this.state === 'countdown' || (this.state === 'play' && dotAlpha > 0)) {
      const t = this.state === 'countdown' ? (now - this.cdStart) / 1000 : 99;
      for (let i = 0; i < 3; i++) {
        let col = rgba(0.35, 0.35, 0.35, 0.85);
        if (t >= 5) col = rgba(0, 0.7, 1, 1);
        else if (t >= 2 + i) col = rgba(1, 0.84, 0, 1);
        ctx.save();
        ctx.globalAlpha = dotAlpha;
        ctx.fillStyle = col;
        ctx.beginPath();
        ctx.arc(240 + (i - 1) * 28, TOP_Y, 5.5, 0, Math.PI * 2);
        ctx.fill();
        ctx.restore();
      }
    }

    // RPM — 시작 1초 뒤 0.4초 페이드인
    if (this.state === 'play' && sinceStart >= 1) {
      const fadeIn = Math.min(1, (sinceStart - 1) / 0.4);
      const rpm = this.rpmSmoothed;
      // 앱: 빨강(≤50) → 금색(75) → 녹색(≥100) 그라데이션, ≤50 이면 0.3s 페이드 깜빡임
      const lerp = (a: number[], b: number[], k: number): string => {
        const t = Math.max(0, Math.min(1, k));
        return rgb255(Math.round(a[0] + (b[0] - a[0]) * t), Math.round(a[1] + (b[1] - a[1]) * t), Math.round(a[2] + (b[2] - a[2]) * t));
      };
      const RED = [255, 60, 60], GOLD = [255, 215, 0], GREEN = [80, 220, 80];
      const col = rpm <= 50 ? lerp(RED, RED, 0) : rpm >= 100 ? lerp(GREEN, GREEN, 0) : rpm <= 75 ? lerp(RED, GOLD, (rpm - 50) / 25) : lerp(GOLD, GREEN, (rpm - 75) / 25);
      const blink = rpm <= 50 ? Math.abs(1 - 2 * (((now - this.playStart) / 1000 % 0.6) / 0.6)) : 1;
      text(ctx, `RPM ${Math.floor(rpm)}`, 240, TOP_Y, 11, col, 'center', fadeIn * blink);
    }

    // 결과 한 줄 (1초 간격 깜빡임)
    if (this.state === 'done') {
      const on = Math.floor((now - this.finishAt) / 1000) % 2 === 0;
      if (on) {
        const praise = this.newRecord ? 'Perfect!' : 'Great!';
        const result = this.newRecord ? 'NEW RECORD!' : 'LEVEL CLEAR';
        const col = this.newRecord ? rgb255(255, 215, 80) : rgb255(80, 220, 180);
        text(ctx, `${praise}  ${fmt(this.finalMs)}  ${result}`, 480 * 0.65, TOP_Y, 11, col);
      }
    }
  }

  private drawDock(ctx: CanvasRenderingContext2D): void {
    const KW = 32;
    ledPanel(ctx, DOCK_X, 177, 40, 190, { border: 4, gap: 3, radius: 0.5, dot: rgba(0.35, 0.4, 0.38, 0.2), mintFrame: true });
    keycap(ctx, DOCK_X, 254, KW, 30);
    keycap(ctx, DOCK_X, 216, KW, 30);
    keycap(ctx, DOCK_X, 158, KW, 70);
    keycap(ctx, DOCK_X, 100, KW, 30);

    // HOME — 흰 PNG를 민트 틴트, 0.72배
    const homeImg = tinted(this.img['btn_home_n.png'], 'home', MINT);
    image(ctx, homeImg, DOCK_X, 254, 0.72);
    this.addHit(DOCK_X, 254, KW, 30, () => {
      window.location.href = SHARE_URL;
    });

    // RESET — 검은 8방향 그림자 + 민트 본체
    const ox = [1, -1, 0, 0, 1, 1, -1, -1];
    const oy = [0, 0, 1, -1, 1, -1, 1, -1];
    for (let i = 0; i < 8; i++) resetIcon(ctx, DOCK_X + ox[i] * 2, 216 + oy[i] * 2, 20, rgba(0, 0, 0, 0.75));
    resetIcon(ctx, DOCK_X, 216, 20, rgba(80 / 255, 220 / 255, 180 / 255, 1));
    this.addHit(DOCK_X, 216, KW, 30, () => {
      if (!this.popup) this.resetToReady();
    });

    // ▲ / ▼ 스테퍼 (앱: 한계치 dim — 앱의 최대 레벨은 10)
    const canInc = this.level < MAX_APP_LEVEL;
    const canDec = this.level > MIN_LEVEL;
    outlinedText(ctx, '▲', DOCK_X, 178, 20, MINT, 2, canInc ? 1 : 0.35);
    outlinedText(ctx, '▼', DOCK_X, 138, 20, MINT, 2, canDec ? 1 : 0.35);
    this.addHit(DOCK_X, 178, KW, 22, () => {
      if (this.state === 'ready' && !this.popup) this.changeLevel(1);
    });
    this.addHit(DOCK_X, 138, KW, 22, () => {
      if (this.state === 'ready' && !this.popup) this.changeLevel(-1);
    });

    // 디스크 수 readout — 촘촘한 도트 패치 + 어두운 민트 숫자
    ctx.fillStyle = rgba(0.35, 0.45, 0.42, 0.45);
    for (let y = -9; y <= 9; y += 2) {
      for (let x = -KW / 2 + 2; x <= KW / 2 - 2; x += 2) {
        ctx.beginPath();
        ctx.arc(DOCK_X + x, 158 + y, 0.5, 0, Math.PI * 2);
        ctx.fill();
      }
    }
    text(ctx, String(this.level), DOCK_X, 158, 17, rgb255(58, 158, 130));

    // BGM (오디오는 후속 단계 — 지금은 ON/OFF 표시만)
    outlinedText(ctx, 'BGM', DOCK_X, 100, 11, MINT, 2, this.soundOn ? 1 : 70 / 255);
    this.addHit(DOCK_X, 100, KW, 30, () => {
      this.soundOn = !this.soundOn;
      this.sound.setEnabled(this.soundOn);
    });
  }

  private addHit(x: number, y: number, w: number, h: number, fn: () => void, quiet = false): void {
    this.hits.push({ x, y, w, h, fn, quiet });
  }

  private drawBottomPanel(ctx: CanvasRenderingContext2D, now: number): void {
    ledPanel(ctx, 240, BOT_Y, 480, 50, { border: 3, dividers: [DIV1 - 240, DIV2 - 240] });

    if (this.state === 'ready') {
      const age = (now - this.idleStart) / 1000;
      if (this.guideActive()) {
        this.drawGuidePanels(ctx, age);
        return;
      }
      if (this.ranks && this.ranks.length > 0 && age < PODIUM_SEC) {
        this.drawPodium(ctx, age);
        return;
      }
      const base = this.ranks && this.ranks.length > 0 ? PODIUM_SEC : 0;
      const set = IDLE_ALL[Math.floor(Math.max(0, now - this.idleStart - base * 1000) / 3000) % IDLE_ALL.length];
      set.forEach((m, i) => text(ctx, m.t.replace('%L', String(this.level)), POLE_X[i], BOT_Y, m.s, m.c));
      return;
    }

    if (this.state === 'play' && now - this.playStart >= 1500) {
      this.drawEqualizer(ctx);
      return;
    }

    if (this.state === 'countdown' || (this.state === 'play' && now - this.playStart < 1500)) {
      const t = this.state === 'countdown' ? (now - this.cdStart) / 1000 : 5 + (now - this.playStart) / 1000;
      const nums: { at: number; s: string; c: string }[] = [
        { at: 2, s: '3', c: rgb255(255, 70, 70) },
        { at: 3, s: '2', c: rgb255(255, 165, 0) },
        { at: 4, s: '1', c: rgb255(255, 215, 0) },
      ];
      if (t >= 5) {
        const blink = 0.5 + 0.5 * Math.cos(((t - 5) / 0.3) * Math.PI * 2); // FadeOut/FadeIn 0.15s 왕복
        for (let i = 0; i < 3; i++) text(ctx, 'GO!', POLE_X[i], BOT_Y, 20, MINT, 'center', blink);
      } else {
        nums.forEach((n, i) => {
          if (t < n.at) return;
          const age = t - n.at;
          const scale = 1 + 0.3 * Math.sin(Math.PI * Math.min(1, age / 0.3)); // pulse
          ctx.save();
          ctx.translate(POLE_X[i], BOT_Y);
          ctx.scale(scale, scale);
          text(ctx, n.s, 0, 0, 22, n.c);
          ctx.restore();
        });
      }
    }
  }

  /** 화면 중앙의 키보드 안내 카드 — 대기 상태에서 표시(첫 방문 가이드 중에는 제외). START 1초 뒤 페이드아웃 */
  private drawKeyCard(ctx: CanvasRenderingContext2D, now: number): void {
    const inReady = HAS_KEYBOARD && !this.popup && this.state === 'ready' && !this.guideActive();
    // START 후 1초 동안 그대로 두었다가 0.4초에 걸쳐 페이드아웃. 대기 중에 보이지 않았던 카드(첫 방문 가이드 등)는 새로 띄우지 않는다.
    const inCountdown = HAS_KEYBOARD && !this.popup && this.state === 'countdown' && this.keyCardSince !== 0 && now - this.cdStart < 1400;
    if (!inReady && !inCountdown) {
      this.keyCardSince = 0;
      return;
    }
    if (this.keyCardSince === 0) this.keyCardSince = now;
    let a = clamp01((now - this.keyCardSince) / 300);
    if (inCountdown) a = Math.min(a, 1 - clamp01(((now - this.cdStart) / 1000 - 1) / 0.4));

    const W = 214, H = 86, CX = 240, CY = 188;
    ctx.save();
    ctx.globalAlpha = a;
    ctx.translate(CX - W / 2, CY - H / 2);
    ctx.fillStyle = rgb255(10, 15, 50, 232 / 255);
    ctx.fillRect(0, 0, W, H);
    ctx.strokeStyle = rgba(0.31, 0.86, 0.7, 0.85);
    ctx.lineWidth = 1;
    ctx.strokeRect(0, 0, W, H);

    text(ctx, '⌨  KEYBOARD CONTROLS', W / 2, H - 12, 9.5, MINT);
    ctx.strokeStyle = rgba(0.5, 0.5, 0.5, 0.6);
    ctx.beginPath();
    ctx.moveTo(12, H - 22);
    ctx.lineTo(W - 12, H - 22);
    ctx.stroke();

    // 1행: [SPACE] START
    keycap(ctx, 44, H - 38, 44, 15);
    text(ctx, 'SPACE', 44, H - 38, 8, MINT);
    text(ctx, '=  START', 84, H - 38, 11, '#fff', 'left');
    // 2행: [J][K][L] POLE A · B · C
    ['J', 'K', 'L'].forEach((k, i) => {
      keycap(ctx, 30 + i * 18, H - 58, 15, 15);
      text(ctx, k, 30 + i * 18, H - 58, 9, MINT);
    });
    text(ctx, '=  POLE A · B · C', 84, H - 58, 11, '#fff', 'left');
    // 3행: 동작 설명
    text(ctx, 'PRESS TO PICK UP  →  PRESS ANOTHER TO DROP', W / 2, H - 76, 8, rgb255(150, 160, 175));
    ctx.restore();
  }

  /** 각 폴 아래 키캡 힌트 [J] [K] [L] — 데스크톱에서만. 집은 폴의 키는 금색으로 강조하고, 키보드로 집었을 때 다음 동작을 안내 */
  private drawKeyHints(ctx: CanvasRenderingContext2D): void {
    if (!HAS_KEYBOARD || this.popup || this.state === 'done') return;
    const sel = this.state === 'play' ? this.game.selected : null;
    ['J', 'K', 'L'].forEach((k, i) => {
      const on = sel === i;
      keycap(ctx, POLE_X[i], 62, 15, 12);
      if (on) {
        ctx.fillStyle = rgba(1, 0.84, 0, 0.28);
        ctx.fillRect(POLE_X[i] - 7.5, 56, 15, 12);
      }
      text(ctx, k, POLE_X[i], 62, 9, on ? rgb255(255, 215, 0) : MINT);
    });
    if (sel !== null && this.lastInputKb) {
      text(ctx, 'PRESS ANOTHER KEY TO MOVE  ·  SAME KEY TO CANCEL', 240, 74, 8.5, rgb255(255, 215, 0));
    }
  }

  /** 첫 방문 + Lv.3 + 대기 상태에서만 규칙 가이드 */
  private guideActive(): boolean {
    return !this.guideSeen && this.level === MIN_LEVEL && this.state === 'ready' && !this.popup;
  }

  /** 앱: arrow.png 를 (240,220) 에, 규칙 이미지(text_en.png)를 그 위 중앙-80 에 얹는다 */
  private drawGuideOverlay(ctx: CanvasRenderingContext2D): void {
    image(ctx, this.img['arrow.png'], 240, 220);
    image(ctx, this.img['text_en.png'], 240, 220 - 80);
  }

  private drawGuidePanels(ctx: CanvasRenderingContext2D, age: number): void {
    const loop = GUIDE_ALL.length * GUIDE_PAGE + GUIDE_T_GAP;
    const t = age % loop;
    if (t >= GUIDE_ALL.length * GUIDE_PAGE) return; // 페이지 사이 암전
    const page = Math.floor(t / GUIDE_PAGE);
    const within = t - page * GUIDE_PAGE;
    const g = GUIDE_ALL[page];
    g.items.forEach((it, k) => {
      if (within < k * GUIDE_T_IN) return;
      // 깜빡임: FadeOut 0.7 / FadeIn 0.7 (표시 시작 시점 기준)
      const blink = g.blink === k ? Math.abs(1 - 2 * (((within - k * GUIDE_T_IN) % 1.4) / 1.4)) : 1;
      text(ctx, it.t, POLE_X[k], BOT_Y, it.s, g.c, 'center', blink);
    });
  }

  /** 메달+국기 배지 */
  private drawBadge(ctx: CanvasRenderingContext2D, medal: string, cc: string, x: number, y: number, size: number, align: 'center' | 'right', alpha: number): void {
    const mw = measure(ctx, medal, size);
    const fw = flagWidth(cc, size);
    const total = mw + (fw ? 2 + fw : 0);
    const x0 = align === 'center' ? x - total / 2 : x - total;
    text(ctx, medal, x0, y, size, '#fff', 'left', alpha);
    if (fw) drawFlag(ctx, cc, x0 + mw + 2, y, size, alpha);
  }

  /** 앱 showPodiumRanking(START 버튼이 중앙을 덮는 배치): 패널A=1위 큰 깃발, 패널C=2·3위 소형 2줄 */
  private drawPodium(ctx: CanvasRenderingContext2D, age: number): void {
    const e = this.ranks!;
    // 1위 — 깃발이 아래에서 올라오며(2.8s EaseOut) 페이드인(1.4s), 이름은 고정
    const p = clamp01(age / 2.8);
    const y1 = BOT_Y + 10 - 50 + 50 * Math.pow(p, 1 / 3);
    this.drawBadge(ctx, '🥇', e[0].cc, POLE_X[0], y1, 22, 'center', clamp01(age / 1.4));
    text(ctx, trunc(e[0].name || '???', 14), POLE_X[0], BOT_Y - 14, 11, rgb255(255, 215, 0));

    const rows = [
      { i: 1, y: BOT_Y + 9, col: rgb255(192, 192, 192), medal: '🥈' },
      { i: 2, y: BOT_Y - 9, col: rgb255(205, 127, 50), medal: '🥉' },
    ];
    rows.forEach((r, k) => {
      const en = e[r.i];
      if (!en) return;
      const a = clamp01((age - (0.8 + k * 0.35)) / 0.9);
      const cx = POLE_X[2];
      this.drawBadge(ctx, r.medal, en.cc, cx - 15, r.y, 13, 'right', a);
      text(ctx, trunc(en.name || '???', 12), cx - 10, r.y, 10, r.col, 'left', a);
    });
  }

  /** 앱 _updateEqualizer 의 렌더 부분: 3구역 × 6막대, 레드→퍼플 그라데이션 + 흰 peak 선 */
  private drawEqualizer(ctx: CanvasRenderingContext2D): void {
    const BARS = 6, BAR_BOT = 5, BAR_MAXH = 42;
    const sectX = [2, DIV1 + 2, DIV2 + 2];
    const sectW = [DIV1 - 4, DIV2 - DIV1 - 4, 480 - DIV2 - 4];
    const STOPS = [
      [1.0, 0.22, 0.22], [1.0, 0.55, 0.05], [1.0, 0.92, 0.05],
      [0.15, 1.0, 0.4], [0.1, 0.82, 1.0], [0.78, 0.18, 1.0],
    ];
    for (let s = 0; s < 3; s++) {
      const barW = sectW[s] / (BARS * 2 - 1);
      for (let i = 0; i < BARS; i++) {
        const h = this.eqH[s][i];
        const pk = this.eqPeak[s][i];
        const x = sectX[s] + i * barW * 2;
        const w = barW * 0.82;
        if (h > 0.5) {
          const gt = (s * BARS + i) / (3 * BARS - 1);
          const pos = gt * 5;
          const ci = Math.min(Math.floor(pos), 4);
          const ct = pos - ci;
          const n = [0, 1, 2].map((k) => STOPS[ci][k] + (STOPS[ci + 1][k] - STOPS[ci][k]) * ct);
          const t = h / BAR_MAXH;
          const dim = 0.22 + t * 0.78;
          ctx.fillStyle = rgba(n[0] * dim, n[1] * dim, n[2] * dim, 0.45 + t * 0.55);
          ctx.fillRect(x, BAR_BOT, w, h);
        }
        if (pk > 2) {
          ctx.fillStyle = rgba(1, 1, 1, 0.95);
          ctx.fillRect(x, BAR_BOT + pk - 1, w, 2);
        }
      }
    }
  }

  private drawStartButton(ctx: CanvasRenderingContext2D, now: number): void {
    const BW = 126, BH = 42;
    // lub-dub 심박: 1.09 → 1.0 → 1.05 → 1.0 → 쉼 (총 1.04s)
    const t = ((now - this.idleStart) / 1000) % 1.04;
    const ease = (a: number, b: number, k: number): number => a + (b - a) * Math.sin((Math.PI / 2) * k);
    let s = 1;
    if (t < 0.11) s = ease(1, 1.09, t / 0.11);
    else if (t < 0.21) s = ease(1.09, 1, (t - 0.11) / 0.1);
    else if (t < 0.3) s = ease(1, 1.05, (t - 0.21) / 0.09);
    else if (t < 0.42) s = ease(1.05, 1, (t - 0.3) / 0.12);

    ctx.save();
    ctx.translate(240, BOT_Y);
    ctx.scale(s, s);
    keycap(ctx, 0, 0, BW, BH);
    drawPixelText(ctx, 'START', 3.3, 0, 0, rgb255(80, 220, 180));
    ctx.restore();
    this.addHit(240, BOT_Y, BW, BH, () => this.startCountdown(), true);
  }

  // ── 팝업 (앱 makePopupFrame / makePopupChipButton) ─────────────────────────
  private chip(ctx: CanvasRenderingContext2D, label: string, col: [number, number, number], lx: number, ly: number, bx: number, by: number, fn: () => void): void {
    const w = 112, h = 38;
    const x0 = lx - w / 2, y0 = ly - h / 2;
    ctx.fillStyle = rgba((col[0] / 255) * 0.2, (col[1] / 255) * 0.2, (col[2] / 255) * 0.2, 0.92);
    ctx.fillRect(x0 + 2, y0 + 2, w - 4, h - 4);
    ctx.strokeStyle = rgb255(col[0], col[1], col[2], 0.85);
    ctx.lineWidth = 1;
    ctx.strokeRect(x0 + 2, y0 + 2, w - 4, h - 4);
    text(ctx, label, lx, ly, 14, rgb255(col[0], col[1], col[2]));
    this.addHit(bx + lx, by + ly, w, h, fn);
  }

  /** 결과 보드에 그릴 행 수 (내 행 포함, 빈 보드는 3) — 보드/팝업 높이를 행 수에 맞춘다 */
  private boardRows(): number {
    const r = this.ranks;
    if (!r || r.length === 0) return 3;
    return this.myRank !== null ? Math.min(10, r.length + 1) : r.length + 1;
  }

  /** 결과 팝업: 앱 랭킹보드 룩(LED 패널·민트 테두리·#/NAME/TIME 컬럼) + 내 예상 순위 행(깜빡임) + 앱 도전 문구 */
  private drawResultBoard(ctx: CanvasRenderingContext2D, PW: number, PH: number, BH: number, now: number): void {
    const y = (d: number): number => PH - d; // 위에서 d 만큼 내려간 로컬 y(y↑)
    const GOLD = rgb255(255, 215, 0);

    text(ctx, this.newRecord ? 'NEW RECORD!' : 'LEVEL CLEAR', PW / 2, y(15), 18, GOLD);
    ctx.strokeStyle = rgba(0.5, 0.5, 0.5, 0.8);
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(16, y(29));
    ctx.lineTo(PW - 16, y(29));
    ctx.stroke();

    // 내 기록 한 줄
    const rpmCol = this.finalRpm >= 100 ? rgb255(80, 220, 80) : this.finalRpm > 50 ? GOLD : rgb255(255, 80, 80);
    text(ctx, `LEVEL ${this.level}`, 14, y(43), 10, rgb255(160, 160, 160), 'left');
    text(ctx, fmt(this.finalMs), PW / 2, y(43), 17, '#fff');
    text(ctx, `AVG RPM ${this.finalRpm}`, PW - 14, y(43), 10, rpmCol, 'right');

    // 보드 패널
    const BX = 8, BW = PW - 16, BTOP = y(54);
    ledPanel(ctx, PW / 2, BTOP - BH / 2, BW, BH, { border: 0, gap: 3, radius: 0.5 });
    ctx.strokeStyle = rgba(0.31, 0.86, 0.7, 0.85);
    ctx.strokeRect(BX, BTOP - BH, BW, BH);
    const colY = BTOP - 9;
    text(ctx, '#', BX + 8, colY, 9, rgb255(130, 130, 150), 'left');
    text(ctx, 'NAME', BX + 44, colY, 9, rgb255(130, 130, 150), 'left');
    text(ctx, 'ONLINE RANK', PW / 2 + 10, colY, 8, rgb255(160, 160, 200));
    text(ctx, 'TIME', BX + BW - 8, colY, 9, rgb255(130, 130, 150), 'right');
    ctx.strokeStyle = rgba(0.3, 0.5, 0.5, 0.5);
    ctx.beginPath();
    ctx.moveTo(BX + 6, BTOP - 17);
    ctx.lineTo(BX + BW - 6, BTOP - 17);
    ctx.stroke();

    type Row = { rank: string; name: string; cc: string; ms: number; me?: boolean };
    const ranks = this.ranks;
    if (!ranks || ranks.length === 0) {
      text(ctx, ranks ? 'No records yet' : 'Rankings unavailable', PW / 2, BTOP - BH / 2 - 6, 11, rgb255(180, 180, 180));
    } else {
      // 내 기록을 시간 순서에 맞춰 끼워 넣는다(상위 10위 안). 밀려난 10위는 보드에서 빠진다 — 실제 보드와 같다.
      const list: Row[] = ranks.map((e) => ({ rank: '', name: e.name, cc: e.cc, ms: e.ms }));
      if (this.myRank !== null) list.splice(this.myRank - 1, 0, { rank: '', name: 'YOU', cc: '', ms: this.finalMs, me: true });
      const rows = list.slice(0, 10);
      rows.forEach((r, i) => (r.rank = String(i + 1)));
      if (this.myRank === null) rows.push({ rank: '10+', name: 'YOU', cc: '', ms: this.finalMs, me: true }); // 순위권 밖

      const blinkOn = Math.floor(now / 450) % 2 === 0; // 0.45초 간격 깜빡임
      rows.forEach((r, i) => {
        const ry = BTOP - 26 - i * 12;
        const col = r.me ? GOLD : '#fff';
        const size = r.me ? 11.5 : 10.5;
        const a = r.me ? (blinkOn ? 1 : 0.25) : 1;
        if (r.me) {
          ctx.fillStyle = rgba(1, 0.84, 0, blinkOn ? 0.18 : 0.05);
          ctx.fillRect(BX + 3, ry - 6, BW - 6, 12);
        }
        text(ctx, r.rank, BX + 8, ry, size, col, 'left', a);
        if (r.cc) drawFlag(ctx, r.cc, BX + 24, ry, size + 1);
        text(ctx, r.me ? 'YOU (EST.)' : trunc(r.name || '???', 10), BX + 44, ry, size, col, 'left', a);
        text(ctx, fmt(r.ms), BX + BW - 8, ry, size, col, 'right', a);
      });
    }

    // 예상 순위 한 줄
    const capY = BTOP - BH - 10;
    if (this.myRank !== null) {
      text(ctx, `⚡ ESTIMATED RANK #${this.myRank} IN THE WORLD!`, PW / 2, capY, 11, GOLD);
    } else if (this.top10Ms !== null) {
      text(ctx, `TOP 10 NEEDS ${fmt(this.top10Ms)} OR FASTER`, PW / 2, capY, 10, MINT);
    }

    // 앱 정식 도전 유도 문구
    text(ctx, 'THIS IS A PRACTICE RUN — MAKE IT OFFICIAL!', PW / 2, capY - 15, 10, MINT);
    text(ctx, 'Challenge the world rankings in the app', PW / 2, capY - 28, 11, '#fff');
  }

  private drawPopup(ctx: CanvasRenderingContext2D, now: number): void {
    const age = (now - this.popupAt) / 1000;
    // 백드롭 딤 160 (0.18s 페이드인)
    ctx.fillStyle = `rgba(0,0,0,${(160 / 255) * Math.min(1, age / 0.18)})`;
    ctx.fillRect(0, 0, 480, 320);

    const PW = 284; // 앱 랭킹보드 패널 폭과 동일
    // 결과 팝업 높이 = 위 54 + 보드(행 수에 맞춤) + 아래 91(캡션·도전 문구·버튼). 행 수가 적으면 그만큼 줄어든다.
    const rowsN = this.boardRows();
    const BH = 23 + 12 * rowsN;
    const PH = this.popup === 'result' ? 145 + BH : 160;
    const bx = 240 - PW / 2, by = 160 - PH / 2;
    const s = age < 0.15 ? 0.7 + 0.35 * (age / 0.15) : age < 0.23 ? 1.05 - 0.05 * ((age - 0.15) / 0.08) : 1;

    const GOLD: [number, number, number] = [255, 215, 0];
    const GRAY: [number, number, number] = [152, 166, 188];
    const FUNC: [number, number, number] = [120, 200, 255];
    const RACE: [number, number, number] = [255, 180, 90];
    const DISMISS: [number, number, number] = [170, 178, 195];

    ctx.save();
    ctx.translate(240, 160);
    ctx.scale(s, s);
    ctx.translate(-PW / 2, -PH / 2);

    ctx.fillStyle = rgb255(10, 15, 50, 240 / 255);
    ctx.fillRect(0, 0, PW, PH);
    const border = this.popup === 'result' ? GOLD : GRAY;
    ctx.strokeStyle = rgb255(border[0], border[1], border[2], 0.9);
    ctx.lineWidth = 1;
    ctx.strokeRect(0, 0, PW, PH);

    if (this.popup !== 'result') {
      text(ctx, 'IN THE APP', PW / 2, PH - 28, 22, rgb255(255, 215, 0));
      ctx.strokeStyle = rgba(0.5, 0.5, 0.5, 0.8);
      ctx.beginPath();
      ctx.moveTo(16, PH - 52);
      ctx.lineTo(PW - 16, PH - 52);
      ctx.stroke();
    }

    if (this.popup === 'result') {
      this.drawResultBoard(ctx, PW, PH, BH, now);
      this.chip(ctx, '↻ PLAY AGAIN', FUNC, 74, 26, bx, by, () => this.resetToReady());
      this.chip(ctx, 'GET THE APP', RACE, PW - 74, 26, bx, by, () => this.openStore());
    } else {
      text(ctx, `LEVEL ${MAX_WEB_LEVEL + 1}+ IS APP ONLY`, PW / 2, PH - 78, 13, '#fff');
      text(ctx, 'Global rankings, ghost races & more', PW / 2, PH - 100, 11, rgb255(150, 160, 175));
      this.chip(ctx, '✕ CLOSE', DISMISS, 74, 26, bx, by, () => {
        this.popup = null;
      });
      this.chip(ctx, 'GET THE APP', RACE, PW - 74, 26, bx, by, () => this.openStore());
    }
    ctx.restore();
  }
}
