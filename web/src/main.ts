import { App } from './core/app';
import { Sound } from './audio/sound';
import { loadImages } from './core/assets';
import { IMAGE_LIST, PlayScene } from './scenes/play';

const canvas = document.getElementById('game') as HTMLCanvasElement | null;
if (!canvas) throw new Error('#game canvas not found');

const app = new App(canvas);

// 전체화면 토글 버튼 (기본 화면은 최대 1280×853 로 제한, 전체화면에서만 창 크기에 맞춰 키운다)
const fsBtn = document.getElementById('fs') as HTMLButtonElement | null;
const docEl = document.documentElement as HTMLElement & { webkitRequestFullscreen?: () => Promise<void> };
const doc = document as Document & { webkitFullscreenElement?: Element; webkitExitFullscreen?: () => Promise<void>; webkitFullscreenEnabled?: boolean };
if (fsBtn && (document.fullscreenEnabled || doc.webkitFullscreenEnabled)) {
  fsBtn.style.display = 'inline-flex';
  const fsLabel = document.getElementById('fs-label');
  const sync = (): void => {
    const on = !!(document.fullscreenElement || doc.webkitFullscreenElement);
    if (fsLabel) fsLabel.textContent = on ? 'EXIT' : 'FULLSCREEN';
    fsBtn.title = on ? 'Exit fullscreen' : 'Fullscreen';
  };
  document.addEventListener('fullscreenchange', sync);
  document.addEventListener('webkitfullscreenchange', sync);
  fsBtn.addEventListener('click', () => {
    const on = document.fullscreenElement || doc.webkitFullscreenElement;
    if (on) void (document.exitFullscreen?.() ?? doc.webkitExitFullscreen?.());
    else void (docEl.requestFullscreen?.() ?? docEl.webkitRequestFullscreen?.());
    canvas.focus();
  });
}

const sound = new Sound();
sound.attachUnlock();
document.addEventListener('visibilitychange', () => sound.setPaused(document.hidden));

loadImages(IMAGE_LIST)
  .then((images) => app.start(new PlayScene(images, sound)))
  .catch((err) => {
    console.error(err);
    document.body.insertAdjacentHTML(
      'beforeend',
      '<p style="position:fixed;top:16px;left:16px;color:#f88;font:16px monospace">Failed to load assets. Please reload.</p>',
    );
  });
