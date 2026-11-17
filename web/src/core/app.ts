// 캔버스 부트 / 게임 루프 / 씬 매니저 / 입력 디스패치.
// 논리 해상도는 앱과 동일한 960×640. 창 크기에 맞춰 비율 유지 스케일(레터박스), 고DPI 보정.

export const LOGICAL_W = 960;
export const LOGICAL_H = 640;
export const CLEAR_COLOR = '#0a0a26'; // 앱의 엔진 클리어 컬러(네이비)와 동일

export type PointerPhase = 'down' | 'move' | 'up' | 'cancel';

export interface PointerInfo {
  phase: PointerPhase;
  /** 논리 좌표(0..960, 0..640). y는 위에서 아래로 증가(캔버스 좌표계) */
  x: number;
  y: number;
  id: number;
}

export interface Scene {
  enter?(app: App): void;
  exit?(): void;
  /** dt: 초 단위(최대 0.1로 클램프). 게임 시간은 dt가 아닌 performance.now()로 잴 것 */
  update?(dt: number): void;
  draw(ctx: CanvasRenderingContext2D): void;
  onPointer?(p: PointerInfo): void;
  onKey?(e: KeyboardEvent): void;
}

const PREVENT_KEYS = new Set([' ', 'ArrowLeft', 'ArrowRight', 'ArrowUp', 'ArrowDown']);

export class App {
  readonly canvas: HTMLCanvasElement;
  readonly ctx: CanvasRenderingContext2D;

  private scene: Scene | null = null;
  private dpr = 1;
  private scale = 1;
  private offsetX = 0; // css px
  private offsetY = 0; // css px
  private last = 0;
  private rafId = 0;

  constructor(canvas: HTMLCanvasElement) {
    const ctx = canvas.getContext('2d');
    if (!ctx) throw new Error('Canvas 2D is not supported');
    this.canvas = canvas;
    this.ctx = ctx;

    window.addEventListener('resize', this.resize);
    // 캔버스의 CSS 크기가 바뀌는 모든 경우(창 크기, 전체화면 전환, 크기 제한 등)를 한 곳에서 처리
    if (typeof ResizeObserver !== 'undefined') new ResizeObserver(() => this.resize()).observe(canvas);
    window.addEventListener('orientationchange', this.resize);
    canvas.addEventListener('contextmenu', (e) => e.preventDefault());
    canvas.addEventListener('pointerdown', (e) => this.handlePointer(e, 'down'));
    canvas.addEventListener('pointermove', (e) => this.handlePointer(e, 'move'));
    canvas.addEventListener('pointerup', (e) => this.handlePointer(e, 'up'));
    canvas.addEventListener('pointercancel', (e) => this.handlePointer(e, 'cancel'));
    window.addEventListener('keydown', this.handleKey);
    this.resize();
  }

  start(first: Scene): void {
    this.setScene(first);
    this.last = performance.now();
    this.rafId = requestAnimationFrame(this.frame);
  }

  stop(): void {
    cancelAnimationFrame(this.rafId);
  }

  setScene(next: Scene): void {
    this.scene?.exit?.();
    this.scene = next;
    next.enter?.(this);
  }

  /** 화면 좌표(clientX/Y) → 논리 좌표 */
  toLogical(clientX: number, clientY: number): { x: number; y: number } {
    const rect = this.canvas.getBoundingClientRect();
    return {
      x: (clientX - rect.left - this.offsetX) / this.scale,
      y: (clientY - rect.top - this.offsetY) / this.scale,
    };
  }

  private resize = (): void => {
    const rect = this.canvas.getBoundingClientRect();
    const cssW = Math.max(1, rect.width);
    const cssH = Math.max(1, rect.height);
    this.dpr = Math.min(window.devicePixelRatio || 1, 2); // 모바일 성능 — 논리 2×이므로 실제 4×까지면 충분
    this.canvas.width = Math.round(cssW * this.dpr);
    this.canvas.height = Math.round(cssH * this.dpr);

    this.scale = Math.min(cssW / LOGICAL_W, cssH / LOGICAL_H);
    this.offsetX = (cssW - LOGICAL_W * this.scale) / 2;
    this.offsetY = (cssH - LOGICAL_H * this.scale) / 2;
  };

  private frame = (now: number): void => {
    const dt = Math.min((now - this.last) / 1000, 0.1);
    this.last = now;

    this.scene?.update?.(dt);

    const ctx = this.ctx;
    // 1) 전체 클리어(레터박스 영역 포함)
    ctx.setTransform(1, 0, 0, 1, 0, 0);
    ctx.fillStyle = CLEAR_COLOR;
    ctx.fillRect(0, 0, this.canvas.width, this.canvas.height);

    // 2) 논리 좌표계로 전환 + 논리 영역 밖은 클립
    const k = this.dpr * this.scale;
    ctx.setTransform(k, 0, 0, k, this.dpr * this.offsetX, this.dpr * this.offsetY);
    ctx.save();
    ctx.beginPath();
    ctx.rect(0, 0, LOGICAL_W, LOGICAL_H);
    ctx.clip();
    this.scene?.draw(ctx);
    ctx.restore();

    this.rafId = requestAnimationFrame(this.frame);
  };

  private handlePointer(e: PointerEvent, phase: PointerPhase): void {
    if (phase === 'down') {
      this.canvas.focus();
      try {
        this.canvas.setPointerCapture(e.pointerId);
      } catch {
        /* 무시 */
      }
    }
    const { x, y } = this.toLogical(e.clientX, e.clientY);
    this.scene?.onPointer?.({ phase, x, y, id: e.pointerId });
  }

  private handleKey = (e: KeyboardEvent): void => {
    if (PREVENT_KEYS.has(e.key)) e.preventDefault(); // 페이지 스크롤 방지
    this.scene?.onKey?.(e);
  };
}
