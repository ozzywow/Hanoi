// 국기 표시: 이모지 지원 환경은 유니코드 국기, 미지원(Windows 크롬/엣지)은 PNG 폴백.
// (landing/index.html 과 같은 감지 방식, PNG 는 landing/flags 사본 → web/public/flags)

import { countryToFlag } from '../core/util';
import { text } from './led';

const BASE = import.meta.env.BASE_URL;

function detectEmojiFlags(): boolean {
  try {
    const cv = document.createElement('canvas');
    cv.width = 30;
    cv.height = 20;
    const c = cv.getContext('2d')!;
    c.fillStyle = '#000';
    c.textBaseline = 'top';
    c.font = '20px sans-serif';
    c.fillText('\u{1F1EF}\u{1F1F5}', 0, 0); // 🇯🇵 — 흰 바탕 붉은 원
    const d = c.getImageData(0, 0, 30, 20).data;
    for (let i = 0; i < d.length; i += 4) {
      if (d[i] > 120 && d[i + 1] < 90 && d[i + 2] < 90 && d[i + 3] > 128) return true;
    }
    return false;
  } catch {
    return true; // 감지 실패 시 이모지 가정(모바일 보호)
  }
}

let emojiOk: boolean | null = null;
export function emojiFlagsSupported(): boolean {
  if (emojiOk === null) emojiOk = detectEmojiFlags();
  return emojiOk;
}

const imgs = new Map<string, HTMLImageElement | 'fail'>();
function flagImage(cc: string): HTMLImageElement | null {
  const hit = imgs.get(cc);
  if (hit === 'fail') return null;
  if (hit) return hit.complete && hit.naturalWidth > 0 ? hit : null;
  const img = new Image();
  img.onerror = () => imgs.set(cc, 'fail');
  img.src = `${BASE}flags/${cc}.png`;
  imgs.set(cc, img);
  return null;
}

/** 국기 폭(디자인 단위). size = 이모지 글자 크기 기준 */
export function flagWidth(cc: string, size: number): number {
  if (!cc || cc.length !== 2) return 0;
  return emojiFlagsSupported() ? size * 1.15 : size * 0.7 * (16 / 11);
}

/** 국기를 (x 왼쪽, y 세로 중앙)에 그린다 — 디자인 공간(y↑) */
export function drawFlag(ctx: CanvasRenderingContext2D, cc: string, x: number, y: number, size: number, alpha = 1): void {
  if (!cc || cc.length !== 2) return;
  if (emojiFlagsSupported()) {
    text(ctx, countryToFlag(cc.toUpperCase()), x + flagWidth(cc, size) / 2, y, size, '#fff', 'center', alpha);
    return;
  }
  const img = flagImage(cc.toLowerCase());
  const h = size * 0.7;
  const w = flagWidth(cc, size);
  if (!img) return;
  ctx.save();
  ctx.globalAlpha = alpha;
  ctx.translate(x, y + h / 2);
  ctx.scale(1, -1);
  ctx.drawImage(img, 0, 0, w, h);
  ctx.restore();
}
