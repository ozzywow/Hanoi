// 앱 PixelFont.cpp 포팅 — 5×7 픽셀 폰트. 현재는 START 버튼에 필요한 글자만(S,T,A,R).
// 글자를 추가할 때는 Classes/PixelFont.cpp 의 pixelGlyph() 와 같은 패턴을 쓴다.

const GW = 5;
const GH = 7;

const GLYPHS: Record<string, string> = {
  S: '.####' + '#....' + '#....' + '.###.' + '....#' + '....#' + '####.',
  T: '#####' + '..#..' + '..#..' + '..#..' + '..#..' + '..#..' + '..#..',
  A: '.###.' + '#...#' + '#...#' + '#####' + '#...#' + '#...#' + '#...#',
  R: '####.' + '#...#' + '#...#' + '####.' + '#.#..' + '#..#.' + '#...#',
};

export function pixelTextWidth(s: string, px: number): number {
  return s.length === 0 ? 0 : (GW * px + px) * s.length - px;
}

/** 디자인 공간(y↑)에서 중심 (cx,cy) 기준으로 그림. 그림자(+1.5,-1.5) 포함 — 앱 makePixelText 와 동일 */
export function drawPixelText(
  ctx: CanvasRenderingContext2D,
  s: string,
  px: number,
  cx: number,
  cy: number,
  color: string,
): void {
  const advance = GW * px + px;
  const cell = px - 0.6;
  const x0 = cx - pixelTextWidth(s, px) / 2;
  const y0 = cy - (GH * px) / 2;

  const pass = (fill: string, ox: number, oy: number): void => {
    ctx.fillStyle = fill;
    let x = x0;
    for (const ch of s) {
      const g = GLYPHS[ch];
      if (g) {
        for (let row = 0; row < GH; row++) {
          for (let c = 0; c < GW; c++) {
            if (g[row * GW + c] !== '#') continue;
            ctx.fillRect(x + c * px + ox, y0 + (GH - 1 - row) * px + oy, cell, cell);
          }
        }
      }
      x += advance;
    }
  };
  pass('rgba(0,0,0,0.6)', 1.5, -1.5);
  pass(color, 0, 0);
}
