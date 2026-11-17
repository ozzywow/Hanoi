import { App, LOGICAL_H, LOGICAL_W, PointerInfo, Scene } from '../core/app';

// Phase 0 확인용 씬: 캔버스 스케일/레터박스, 포인터 좌표 변환, 키 입력, 프레임 루프 점검.
// Phase 1에서 SplashScene → MainScene 으로 교체된다.
export class BootScene implements Scene {
  private pointer: PointerInfo | null = null;
  private lastKey = '-';
  private fps = 0;
  private frames = 0;
  private acc = 0;
  private t = 0;

  enter(_app: App): void {
    // Phase 1에서 app.setScene(...) 전환에 사용
  }

  update(dt: number): void {
    this.t += dt;
    this.acc += dt;
    this.frames++;
    if (this.acc >= 0.5) {
      this.fps = Math.round(this.frames / this.acc);
      this.frames = 0;
      this.acc = 0;
    }
  }

  onPointer(p: PointerInfo): void {
    this.pointer = p;
  }

  onKey(e: KeyboardEvent): void {
    this.lastKey = e.key === ' ' ? 'Space' : e.key;
  }

  draw(ctx: CanvasRenderingContext2D): void {
    // 논리 영역 테두리(레터박스 확인)
    ctx.strokeStyle = '#2a3b7a';
    ctx.lineWidth = 2;
    ctx.strokeRect(1, 1, LOGICAL_W - 2, LOGICAL_H - 2);

    // 타이틀
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillStyle = '#ffd700';
    ctx.font = 'bold 40px monospace';
    ctx.fillText('TOWER OF HANOI', LOGICAL_W / 2, 220);
    ctx.fillStyle = '#7fe8ff';
    ctx.font = 'bold 22px monospace';
    ctx.fillText('S P E E D R U N  -  W E B', LOGICAL_W / 2, 268);

    // 3개의 폴(렌더링 확인용 간단 도형)
    const poleXs = [LOGICAL_W * 0.25, LOGICAL_W * 0.5, LOGICAL_W * 0.75];
    ctx.fillStyle = '#4a5a9a';
    for (const x of poleXs) ctx.fillRect(x - 4, 340, 8, 150);
    ctx.fillStyle = '#3a4a8a';
    ctx.fillRect(LOGICAL_W * 0.12, 490, LOGICAL_W * 0.76, 10);

    // 상태 표시
    ctx.fillStyle = '#9fd';
    ctx.font = '16px monospace';
    ctx.fillText(`phase 0 scaffold  |  ${this.fps} fps  |  t=${this.t.toFixed(1)}s`, LOGICAL_W / 2, 560);
    const pt = this.pointer
      ? `pointer ${this.pointer.phase} (${this.pointer.x.toFixed(0)}, ${this.pointer.y.toFixed(0)})`
      : 'pointer -';
    ctx.fillText(`${pt}  |  key: ${this.lastKey}`, LOGICAL_W / 2, 590);

    // 포인터 위치 마커
    if (this.pointer && this.pointer.phase !== 'up' && this.pointer.phase !== 'cancel') {
      ctx.strokeStyle = '#ff6a8a';
      ctx.lineWidth = 2;
      ctx.beginPath();
      ctx.arc(this.pointer.x, this.pointer.y, 14, 0, Math.PI * 2);
      ctx.stroke();
    }
  }
}
