// 앱 DrawUtils.cpp 포팅 — LED 패널, 키캡, 벡터 아이콘, 텍스트/이미지 헬퍼.
//
// ★ 좌표계: 앱과 동일한 "디자인 공간"(480×320, 원점 좌하단, y 위로 증가)에서 그린다.
//   씬이 draw() 시작에서 beginDesign(ctx)를 호출하면 앱의 DrawNode 지오메트리를 그대로 옮길 수 있다.
//   텍스트/이미지는 y 반전을 되돌려 똑바로 그리는 헬퍼(text/image)를 쓴다.

import { LOGICAL_H } from '../core/app';

export const DESIGN_W = 480;
export const DESIGN_H = 320;

/** 논리(960×640, y↓) → 디자인(480×320, y↑) 좌표계로 전환. 호출 측이 save/restore 한다. */
export function beginDesign(ctx: CanvasRenderingContext2D): void {
  ctx.translate(0, LOGICAL_H);
  ctx.scale(2, -2);
}

export const rgba = (r: number, g: number, b: number, a = 1): string =>
  `rgba(${Math.round(r * 255)},${Math.round(g * 255)},${Math.round(b * 255)},${a})`;
export const rgb255 = (r: number, g: number, b: number, a = 1): string => `rgba(${r},${g},${b},${a})`;

export const MINT = rgb255(80, 220, 180);

// ── 스프라이트 캐시 (정적 LED 패널의 수천 개 도트를 매 프레임 그리지 않도록) ─────────────
const SPR = 4; // 디자인 1단위당 픽셀 수 (논리 2× × DPR 2 대응)
const cache = new Map<string, HTMLCanvasElement>();

function sprite(key: string, w: number, h: number, draw: (c: CanvasRenderingContext2D) => void): HTMLCanvasElement {
  let cv = cache.get(key);
  if (cv) return cv;
  cv = document.createElement('canvas');
  cv.width = Math.ceil(w * SPR);
  cv.height = Math.ceil(h * SPR);
  const c = cv.getContext('2d')!;
  c.translate(0, cv.height);
  c.scale(SPR, -SPR); // 스프라이트 안도 y↑
  draw(c);
  cache.set(key, cv);
  return cv;
}

/** 스프라이트를 디자인 공간의 (x,y=좌하단)에 그린다 */
function blit(ctx: CanvasRenderingContext2D, cv: HTMLCanvasElement, x: number, y: number, w: number, h: number): void {
  ctx.save();
  ctx.translate(x, y + h);
  ctx.scale(1, -1);
  ctx.drawImage(cv, 0, 0, w, h);
  ctx.restore();
}

export interface LedOpts {
  border?: number;
  gap?: number;
  radius?: number;
  dot?: string;
  /** 패널 중심 기준 x 오프셋의 구분선 */
  dividers?: number[];
  /** 민트 외곽 프레임(좌측 도크) */
  mintFrame?: boolean;
}

/** setupLedBackground + setupLedDotOverlay (+ 선택: 구분선/외곽 프레임). 중심 (cx,cy), 본체 w×h. */
export function ledPanel(ctx: CanvasRenderingContext2D, cx: number, cy: number, w: number, h: number, o: LedOpts = {}): void {
  const border = o.border ?? 3;
  const gap = o.gap ?? 2;
  const radius = o.radius ?? 0.5;
  const dot = o.dot ?? rgba(0.3, 0.3, 0.3, 1);
  const dividers = o.dividers ?? [];
  const sw = w;
  const sh = h + border * 2;
  const key = `led|${w}|${h}|${border}|${gap}|${radius}|${dot}|${dividers.join(',')}|${o.mintFrame ? 1 : 0}`;
  const cv = sprite(key, sw, sh, (c) => {
    c.translate(sw / 2, sh / 2);
    // setupLedBackground: 바깥(테두리색) → 안쪽(청록 어둠)
    c.fillStyle = rgba(0.03, 0.03, 0.03, 1);
    c.fillRect(-w / 2, -h / 2 - border, w, h + border * 2);
    c.fillStyle = rgba(0.03, 0.09, 0.09, 1);
    c.fillRect(-w / 2, -h / 2, w, h);
    if (o.mintFrame) {
      c.strokeStyle = rgba(0.31, 0.86, 0.7, 0.9);
      c.lineWidth = 1;
      c.strokeRect(-w / 2 + 1, -h / 2 - 3, w - 2, h + 6);
    }
    for (const dx of dividers) {
      c.strokeStyle = rgba(0.5, 0.5, 0.5, 0.8);
      c.lineWidth = 1;
      c.beginPath();
      c.moveTo(dx, -h / 2);
      c.lineTo(dx, h / 2);
      c.stroke();
    }
    // setupLedDotOverlay
    c.fillStyle = dot;
    for (let y = -h / 2 + gap / 2; y < h / 2; y += gap) {
      for (let x = -w / 2 + gap / 2; x < w / 2; x += gap) {
        c.beginPath();
        c.arc(x, y, radius, 0, Math.PI * 2);
        c.fill();
      }
    }
  });
  blit(ctx, cv, cx - sw / 2, cy - sh / 2, sw, sh);
}

/** drawKeycap — 앱 DrawUtils.cpp 와 동일한 12각 키캡 */
export function keycap(ctx: CanvasRenderingContext2D, cx: number, cy: number, w: number, h: number, gray = false): void {
  const x0 = cx - w / 2, x1 = cx + w / 2;
  const y0 = cy - h / 2, y1 = cy + h / 2;
  const c = Math.max(2, Math.min(w, h) * 0.1);
  const base = gray ? rgba(0.13, 0.14, 0.16, 1) : rgba(0.08, 0.19, 0.19, 1);
  const hi = gray ? rgba(0.65, 0.65, 0.65, 0.4) : rgba(0.5, 1.0, 0.85, 0.5);
  const shadow = rgba(0, 0, 0, 0.5);
  const border = gray ? rgba(0.42, 0.46, 0.5, 0.6) : rgba(0.31, 0.86, 0.7, 0.55);
  const k = 0.293;
  const poly: [number, number][] = [
    [x0 + c, y0], [x1 - c, y0],
    [x1 - c * k, y0 + c * k], [x1, y0 + c],
    [x1, y1 - c], [x1 - c * k, y1 - c * k],
    [x1 - c, y1], [x0 + c, y1],
    [x0 + c * k, y1 - c * k], [x0, y1 - c],
    [x0, y0 + c], [x0 + c * k, y0 + c * k],
  ];
  const path = (): void => {
    ctx.beginPath();
    poly.forEach(([x, y], i) => (i ? ctx.lineTo(x, y) : ctx.moveTo(x, y)));
    ctx.closePath();
  };
  ctx.save();
  path();
  ctx.fillStyle = base;
  ctx.fill();
  ctx.fillStyle = shadow;
  ctx.fillRect(x0 + c, y0, w - 2 * c, 2);
  ctx.fillRect(x1 - 2, y0 + c, 2, h - 2 * c);
  ctx.fillStyle = hi;
  ctx.fillRect(x0 + c, y1 - 2, w - 2 * c, 2);
  ctx.fillRect(x0, y0 + c, 2, h - 2 * c);
  ctx.beginPath();
  ctx.moveTo(x0, y1 - c); ctx.lineTo(x0 + c, y1); ctx.lineTo(x0 + 2, y1 - 2); ctx.closePath();
  ctx.fillStyle = hi;
  ctx.fill();
  ctx.beginPath();
  ctx.moveTo(x1 - c, y0); ctx.lineTo(x1, y0 + c); ctx.lineTo(x1 - 2, y0 + 2); ctx.closePath();
  ctx.fillStyle = shadow;
  ctx.fill();
  path();
  ctx.strokeStyle = border;
  ctx.lineWidth = 1;
  ctx.stroke();
  ctx.restore();
}

/** drawVecReset (|◀) */
export function resetIcon(ctx: CanvasRenderingContext2D, cx: number, cy: number, sz: number, col: string): void {
  const barW = sz * 0.12, barH = sz * 0.7, triW = sz * 0.45, triH = sz * 0.65, gap = sz * 0.1;
  const totalW = barW + gap + triW;
  const leftX = cx - totalW * 0.5;
  ctx.fillStyle = col;
  ctx.fillRect(leftX, cy - barH / 2, barW, barH);
  const triCx = leftX + barW + gap + triW * 0.5;
  ctx.beginPath();
  ctx.moveTo(triCx + triW * 0.5, cy + triH * 0.5);
  ctx.lineTo(triCx - triW * 0.5, cy);
  ctx.lineTo(triCx + triW * 0.5, cy - triH * 0.5);
  ctx.closePath();
  ctx.fill();
}

// ── 텍스트/이미지 (y 반전 되돌림) ─────────────────────────────────────────────

const FONT = 'Arial, "Segoe UI Emoji", "Apple Color Emoji", sans-serif';

export function text(
  ctx: CanvasRenderingContext2D,
  s: string,
  x: number,
  y: number,
  size: number,
  color: string,
  align: CanvasTextAlign = 'center',
  alpha = 1,
): void {
  ctx.save();
  ctx.translate(x, y);
  ctx.scale(1, -1);
  ctx.globalAlpha = alpha;
  ctx.font = `${size}px ${FONT}`;
  ctx.textAlign = align;
  ctx.textBaseline = 'middle';
  ctx.fillStyle = color;
  ctx.fillText(s, 0, 0);
  ctx.restore();
}

export function measure(ctx: CanvasRenderingContext2D, s: string, size: number): number {
  ctx.save();
  ctx.font = `${size}px ${FONT}`;
  const w = ctx.measureText(s).width;
  ctx.restore();
  return w;
}

/** 앱 makeOutlined — 검은 8방향 외곽선 × thickness 링 + 본문 */
export function outlinedText(
  ctx: CanvasRenderingContext2D,
  s: string,
  x: number,
  y: number,
  size: number,
  color: string,
  thickness: number,
  alpha = 1,
): void {
  const ox = [1, -1, 0, 0, 1, 1, -1, -1];
  const oy = [0, 0, 1, -1, 1, -1, 1, -1];
  for (let d = 1; d <= thickness; d++) {
    for (let i = 0; i < 8; i++) text(ctx, s, x + ox[i] * d, y + oy[i] * d, size, 'rgb(0,0,0)', 'center', alpha);
  }
  text(ctx, s, x, y, size, color, 'center', alpha);
}

/** 이미지를 중심(cx,cy) 기준으로 그린다. anchorY: 0=하단, 0.5=중앙 */
export function image(
  ctx: CanvasRenderingContext2D,
  img: CanvasImageSource & { width: number; height: number },
  cx: number,
  cy: number,
  scale = 1,
  anchorY = 0.5,
): void {
  const w = img.width * scale;
  const h = img.height * scale;
  ctx.save();
  ctx.translate(cx, cy + h * (1 - anchorY));
  ctx.scale(1, -1);
  ctx.drawImage(img, -w / 2, 0, w, h);
  ctx.restore();
}

// ── 이미지 틴트(흰 PNG → 민트, 앱의 setColor 곱연산) ──────────────────────────
const tintCache = new Map<string, HTMLCanvasElement>();
export function tinted(img: HTMLImageElement, key: string, color: string): HTMLCanvasElement {
  const k = `${key}|${color}`;
  let cv = tintCache.get(k);
  if (cv) return cv;
  cv = document.createElement('canvas');
  cv.width = img.width;
  cv.height = img.height;
  const c = cv.getContext('2d')!;
  c.drawImage(img, 0, 0);
  c.globalCompositeOperation = 'multiply';
  c.fillStyle = color;
  c.fillRect(0, 0, cv.width, cv.height);
  c.globalCompositeOperation = 'destination-in';
  c.drawImage(img, 0, 0);
  tintCache.set(k, cv);
  return cv;
}
