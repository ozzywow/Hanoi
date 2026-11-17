// Web Audio 사운드 매니저 (앱 SoundFactory 대응).
//  - 효과음: web/public/audio/efs_*.wav (앱 원본 그대로) — 즉시 재생
//  - BGM: web/public/audio/bgm_*.mp3 (96kbps) — 루프, 볼륨은 RPM 연동(씬이 setBgmVolume 호출)
//  - 브라우저 자동재생 정책: 첫 사용자 입력에서 unlock() 해야 AudioContext 가 시작된다.

const BASE = import.meta.env.BASE_URL;

export const EFFECTS = [
  'efs_cancel_select', 'efs_clean', 'efs_click', 'efs_count_sec', 'efs_go',
  'efs_move_disc_ok', 'efs_new_record', 'efs_select_disc', 'efs_start_countdown',
] as const;
export type EffectName = (typeof EFFECTS)[number];

/** 앱 PlayScene::Start 의 BGM_LIST 와 동일 */
export const BGM_LIST = ['bgm_space', 'bgm_universe', 'bgm_cosmos', 'bgm_nova', 'bgm_moon', 'bgm_earth'];

export class Sound {
  private ctx: AudioContext | null = null;
  private buffers = new Map<string, AudioBuffer>();
  private loading = new Map<string, Promise<AudioBuffer | null>>();
  private effectsRequested = false;

  private bgmSrc: AudioBufferSourceNode | null = null;
  private bgmGain: GainNode | null = null;
  private bgmName = '';
  private bgmVol = 0; // 0..1 (RPM 연동 값)

  enabled = true;
  readonly mastVolume = 1.0; // 앱 MASTER_VOLUME

  /** 첫 사용자 입력에서 호출 — 컨텍스트 생성/재개 + 효과음 로드 시작 */
  unlock(): void {
    if (!this.ctx) {
      const AC = window.AudioContext ?? (window as unknown as { webkitAudioContext?: typeof AudioContext }).webkitAudioContext;
      if (!AC) return;
      try {
        this.ctx = new AC();
      } catch {
        return;
      }
    }
    if (this.ctx.state === 'suspended') void this.ctx.resume();
    if (!this.effectsRequested) {
      this.effectsRequested = true;
      EFFECTS.forEach((n) => void this.load(n, 'wav'));
    }
  }

  private load(name: string, ext: 'wav' | 'mp3'): Promise<AudioBuffer | null> {
    const hit = this.buffers.get(name);
    if (hit) return Promise.resolve(hit);
    const pending = this.loading.get(name);
    if (pending) return pending;
    const ctx = this.ctx;
    if (!ctx) return Promise.resolve(null);
    const p = fetch(`${BASE}audio/${name}.${ext}`)
      .then((r) => r.arrayBuffer())
      .then((ab) => ctx.decodeAudioData(ab))
      .then((buf) => {
        this.buffers.set(name, buf);
        return buf;
      })
      .catch((e) => {
        console.warn('[sound] load failed', name, e);
        this.loading.delete(name);
        return null;
      });
    this.loading.set(name, p);
    return p;
  }

  play(name: EffectName): void {
    const ctx = this.ctx;
    const buf = this.buffers.get(name);
    if (!this.enabled || !ctx || !buf) return;
    const src = ctx.createBufferSource();
    src.buffer = buf;
    const g = ctx.createGain();
    g.gain.value = this.mastVolume;
    src.connect(g).connect(ctx.destination);
    src.start();
  }

  /** BGM 미리 로드(카운트다운 중) */
  preloadBgm(name: string): void {
    void this.load(name, 'mp3');
  }

  /** 앱 switchBGM + 볼륨 0 시작 (RPM 이 오르면서 페이드인) */
  async startBgm(name: string): Promise<void> {
    this.stopBgm();
    const ctx = this.ctx;
    if (!ctx) return;
    this.bgmName = name;
    const buf = await this.load(name, 'mp3');
    if (!buf || this.bgmName !== name) return; // 로딩 중 다른 곡/정지됨
    const src = ctx.createBufferSource();
    src.buffer = buf;
    src.loop = true;
    const g = ctx.createGain();
    g.gain.value = this.enabled ? this.bgmVol * this.mastVolume : 0;
    src.connect(g).connect(ctx.destination);
    src.start();
    this.bgmSrc = src;
    this.bgmGain = g;
  }

  /** RPM 연동 볼륨(0..1) */
  setBgmVolume(v: number): void {
    this.bgmVol = v;
    if (this.bgmGain && this.ctx) {
      this.bgmGain.gain.setTargetAtTime(this.enabled ? v * this.mastVolume : 0, this.ctx.currentTime, 0.05);
    }
  }

  fadeOutBgm(seconds: number): void {
    const ctx = this.ctx;
    const g = this.bgmGain;
    const src = this.bgmSrc;
    if (!ctx || !g || !src) return;
    this.bgmName = '';
    this.bgmSrc = null;
    this.bgmGain = null;
    g.gain.cancelScheduledValues(ctx.currentTime);
    g.gain.setValueAtTime(g.gain.value, ctx.currentTime);
    g.gain.linearRampToValueAtTime(0, ctx.currentTime + seconds);
    src.stop(ctx.currentTime + seconds + 0.05);
  }

  stopBgm(): void {
    this.bgmName = '';
    if (this.bgmSrc) {
      try {
        this.bgmSrc.stop();
      } catch {
        /* 이미 정지 */
      }
    }
    this.bgmSrc = null;
    this.bgmGain = null;
    this.bgmVol = 0;
  }

  /** 모바일 사파리 등은 click/touchend 같은 특정 제스처에서만 오디오를 허용 — 여러 이벤트에 걸어 둔다 */
  attachUnlock(): void {
    const evs = ['pointerdown', 'pointerup', 'touchend', 'click', 'keydown'];
    const handler = (): void => {
      this.unlock();
      if (this.ctx && this.ctx.state === 'running') evs.forEach((e) => window.removeEventListener(e, handler, true));
    };
    evs.forEach((e) => window.addEventListener(e, handler, true));
  }

  /** 탭이 가려지면 소리를 멈추고(컨텍스트 suspend) 돌아오면 재개 */
  setPaused(paused: boolean): void {
    const ctx = this.ctx;
    if (!ctx) return;
    if (paused) void ctx.suspend();
    else void ctx.resume();
  }

  setEnabled(on: boolean): void {
    this.enabled = on;
    if (this.bgmGain && this.ctx) {
      this.bgmGain.gain.setTargetAtTime(on ? this.bgmVol * this.mastVolume : 0, this.ctx.currentTime, 0.05);
    }
  }
}
