"""
BGM2 재생성 - A단조 (bgm_intro 동일 조성)
A natural minor, BPM 124
진행: Am | F | C | G  / Dm | Am | Em | Am
"""
import wave, struct, math, os

SR = 44100
OUTPUT_DIR = os.path.join(os.path.dirname(__file__), '..', 'Resources', 'Sound')

NOTE = {
    'G2': 98.00, 'A2':110.00,
    'C3':130.81, 'D3':146.83, 'E3':164.81, 'F3':174.61, 'G3':196.00, 'A3':220.00,
    'B3':246.94, 'C4':261.63, 'D4':293.66, 'E4':329.63, 'F4':349.23, 'G4':392.00,
    'A4':440.00, 'B4':493.88, 'C5':523.25, 'D5':587.33, 'E5':659.25, 'F5':698.46,
    'G5':783.99, 'A5':880.00,
}

def sine(freq, dur, amp=0.5):
    n = int(SR * dur)
    return [amp * math.sin(2 * math.pi * freq * i / SR) for i in range(n)]

def square(freq, dur, amp=0.4, duty=0.5):
    n = int(SR * dur)
    period = SR / freq
    return [amp if (i % period) < duty * period else -amp for i in range(n)]

def triangle(freq, dur, amp=0.22):
    """직접 삼각파 - 고조파 합성보다 빠름"""
    n = int(SR * dur)
    period = SR / freq
    return [amp * (2 * abs(2 * ((i % period) / period) - 1) - 1) for i in range(n)]

def adsr(samples, a=0.01, d=0.05, s=0.7, r=0.1):
    n = len(samples)
    ai, di, ri = int(a * SR), int(d * SR), int(r * SR)
    si = max(0, n - ai - di - ri)
    result = []
    for i, v in enumerate(samples):
        if i < ai:
            env = i / ai if ai else 1.0
        elif i < ai + di:
            env = 1.0 - (1.0 - s) * (i - ai) / di if di else s
        elif i < ai + di + si:
            env = s
        else:
            pos = i - ai - di - si
            env = s * (1.0 - pos / ri) if ri else 0.0
        result.append(v * max(0.0, env))
    return result

def mix(*tracks):
    length = max(len(t) for t in tracks)
    out = [0.0] * length
    for t in tracks:
        for i, v in enumerate(t):
            out[i] += v
    peak = max(abs(v) for v in out) if out else 1.0
    scale = 0.9 / peak if peak > 0.9 else 1.0
    return [v * scale for v in out]

def concat(*parts):
    out = []
    for p in parts:
        out.extend(p)
    return out

def crossfade_loop(samples, fade_s=0.05):
    """끝 fade_s초에 처음을 크로스페이드 → 갭 마스킹, 50ms라 겹침 비가청"""
    n = len(samples)
    fade_n = int(SR * fade_s)
    out = list(samples)
    for i in range(fade_n):
        t = i / fade_n
        out[n - fade_n + i] = samples[n - fade_n + i] * (1.0 - t) + samples[i] * t
    return out

def save(filename, samples):
    path = os.path.join(OUTPUT_DIR, filename)
    frames = b''.join(struct.pack('<h', max(-32767, min(32767, int(v * 32767)))) for v in samples)
    with wave.open(path, 'w') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(frames)
    dur = len(samples) / SR
    print(f"  {filename:20s}  {dur:.2f}s")

def sub_thump(dur_s, amp=0.28):
    """베이스 타격감 보강: 250Hz→120Hz 피치스윕, 70ms — 드럼 아닌 타격감"""
    note_n    = int(SR * 0.070)
    silence_n = max(0, int(SR * dur_s) - note_n)
    k = 10.5  # 주파수 감쇠 상수: 250*exp(-k*0.07)≈120Hz
    phase = 0.0
    out = []
    for i in range(note_n):
        t    = i / SR
        freq = max(120.0, 250.0 * math.exp(-k * t))
        phase += 2 * math.pi * freq / SR
        ei   = i / note_n
        env  = (ei / 0.05) if ei < 0.05 else math.exp(-(ei - 0.05) * 8.0)
        out.append(amp * math.sin(phase) * env)
    return out + [0.0] * silence_n


def gen_bgm2():
    BPM = 124
    q = 60 / BPM   # 4분음표
    e = q / 2      # 8분음표
    s = q / 4      # 16분음표

    def sq(note, dur, amp=0.32, duty=0.25):
        return adsr(square(NOTE[note], dur, amp, duty), a=0.005, d=0.02, s=0.75, r=0.04)

    def sq_bass(note, dur, amp=0.22):
        return adsr(sine(NOTE[note], dur, amp), a=0.08, d=0.08, s=0.70, r=0.20)

    def sq_bass_oct(note, dur, amp=0.16):
        """옥타브 위 베이스 — 핸드폰 스피커 보강 (freq×2)"""
        return adsr(sine(NOTE[note] * 2, dur, amp), a=0.04, d=0.06, s=0.65, r=0.12)

    def tri(note, dur):
        return adsr(triangle(NOTE[note], dur, amp=0.40), a=0.004, d=0.01, s=0.8, r=0.04)

    # ── A섹션 멜로디: Am | F | C | G (8 half-bar = 4 full measure) ──
    mel_A = concat(
        # Am
        sq('E5',e), sq('A5',e), sq('G5',e), sq('E5',e),
        sq('A4',q), sq('C5',q),
        # F
        sq('F5',e), sq('E5',e), sq('C5',e), sq('A4',e),
        sq('D5',q), sq('F5',q),
        # C
        sq('C5',e), sq('E5',e), sq('G5',e), sq('E5',e),
        sq('E5',e), sq('D5',e), sq('C5',q),
        # G → Am resolve
        sq('B4',e), sq('D5',e), sq('G5',e), sq('D5',e),
        sq('E5',e), sq('C5',e), sq('A4',q),
    )

    # ── B섹션 멜로디: Dm | Am | Em | Am (변주, 더 긴장감) ─────────
    mel_B = concat(
        # Dm
        sq('D5',e), sq('F5',e), sq('A5',e), sq('F5',e),
        sq('E5',q), sq('D5',q),
        # Am
        sq('C5',e), sq('A4',e), sq('E5',e), sq('A4',e),
        sq('A4',q), sq('C5',q),
        # Em (단조 5도 - 긴장)
        sq('E5',e), sq('G5',e), sq('B4',e), sq('G5',e),
        sq('G5',e), sq('F5',e), sq('E5',q),
        # Am 해결 → 루프
        sq('A5',e), sq('G5',e), sq('E5',e), sq('C5',e),
        sq('E5',e), sq('C5',e), sq('A4',e), sq('E5',e),
    )
    melody = concat(mel_A, mel_B)

    # ── 베이스: 한 옥타브 내린 루트음, 하프노트 드론 ───────────
    # 16 entries × h(반음표) = arp 16마디와 길이 일치
    h = q * 2
    bass_roots = [
        'A2','A2', 'F3','F3', 'C3','C3', 'G2','G2',   # Am F C G
        'A2','A2', 'F3','F3', 'C3','C3', 'G2','G2',   # Am F C G
        'D3','D3', 'A2','A2', 'E3','E3', 'A2','A2',   # Dm Am Em Am
    ]
    bass_parts = [sq_bass(n, h) for n in bass_roots]
    bass = concat(*bass_parts)
    bass_oct = concat(*[sq_bass_oct(n, h) for n in bass_roots])

    # ── 아르페지오 삼각파: 16 patterns, 각 pattern = 16분음표 16개 (1마디) ──
    arp_patterns = [
        # Am F C G
        ['A3','C4','E4','C4'], ['F3','A3','C4','A3'],
        ['C4','E4','G4','E4'], ['G3','B3','D4','B3'],
        # Am F C G
        ['A3','C4','E4','C4'], ['F3','A3','C4','A3'],
        ['C4','E4','G4','E4'], ['G3','B3','D4','B3'],
        # Dm Am Em Am
        ['D3','F3','A3','F3'], ['A3','C4','E4','C4'],
        ['E3','G3','B3','G3'], ['A3','C4','E4','C4'],
    ]
    arp_parts = []
    for pattern in arp_patterns:
        for note in pattern * 4:  # 4음 패턴 × 4 = 16분음표 16개
            arp_parts.append(tri(note, s))
    arp = concat(*arp_parts)

    # 베이스 루트 24개 × 반음표(h=2q) = 48비트 → thump 1회/비트
    thump_seq = concat(*[sub_thump(q) for _ in range(48)])
    max_len = max(len(bass), len(bass_oct), len(arp), len(thump_seq))
    def pad(t): return t + [0.0] * (max_len - len(t))
    b, bo, a, th = pad(bass), pad(bass_oct), pad(arp), pad(thump_seq)
    raw = [b[i]*0.43 + bo[i]*0.27 + a[i]*0.17 + th[i]*0.13 for i in range(max_len)]
    peak = max(abs(v) for v in raw) if raw else 1.0
    scale = 1.035 / peak if peak > 0 else 1.0  # +15% 음량
    mixed = [v * scale for v in raw]
    looped = mixed * 4
    save('bgm_universe.wav', looped)
    print(f"  {'(x4 loop)':20s}  {len(looped)/SR:.1f}s  → 게임에서 무한루프")


if __name__ == '__main__':
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    print('[gen_bgm2_am] A단조 PlayScene BGM 생성...\n')
    gen_bgm2()
    print('\n완료! Resources/Sound/BGM2.wav 교체됨')
