"""
BGM Nova — E단조, BPM 112
멜로디(스퀘어파 25% 듀티) + 베이스+옥타브 + 삼각파 아르페지오 + 윈드차임
진행: Em | Am | C | B  (각 2마디, 총 8마디 루프 × 3)
"""
import wave, struct, math, os

SR = 44100
OUTPUT_DIR = os.path.join(os.path.dirname(__file__), '..', 'Resources', 'Sound')

NOTE = {
    'E2': 82.41,  'A2':110.00,  'B2':123.47,  'C3':130.81,
    'D3':146.83,  'D#3':155.56, 'E3':164.81,  'F#3':185.00,
    'G3':196.00,  'A3':220.00,  'B3':246.94,
    'C4':261.63,  'D4':293.66,  'D#4':311.13, 'E4':329.63,
    'F#4':369.99, 'G4':392.00,  'A4':440.00,  'B4':493.88,
    'C5':523.25,  'D5':587.33,  'D#5':622.25, 'E5':659.25,
    'F#5':739.99, 'G5':783.99,  'A5':880.00,
}

def save(filename, samples):
    path = os.path.join(OUTPUT_DIR, filename)
    frames = b''.join(struct.pack('<h', max(-32767, min(32767, int(v * 32767)))) for v in samples)
    with wave.open(path, 'w') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
        w.writeframes(frames)
    print(f"  {filename}  {len(samples)/SR:.2f}s")

def concat(*parts):
    out = []
    for p in parts: out.extend(p)
    return out

def sub_thump(dur_s, amp=0.28):
    """베이스 타격감 보강: 250Hz→120Hz 피치스윕, 70ms"""
    note_n    = int(SR * 0.070)
    silence_n = max(0, int(SR * dur_s) - note_n)
    k = 10.5
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

def gen():
    BPM = 112
    q   = 60 / BPM
    e   = q / 2
    s16 = q / 4
    h   = q * 2
    bar = q * 4

    # ─── 베이스: compound sine (8분음표) ─────────────────────────────
    def bass_hit(freq_low, amp=0.80):
        freq_hi   = freq_low * 2
        note_n    = int(SR * e * 0.88)
        silence_n = int(SR * e) - note_n
        out = [0.0] * note_n
        for i in range(note_n):
            out[i] = (amp        * math.sin(2*math.pi * freq_low * i / SR)
                    + amp * 0.40 * math.sin(2*math.pi * freq_hi  * i / SR))
        a_, d_, s_, r_ = 0.020, 0.040, 0.82, 0.17
        ai = int(a_*SR); di = int(d_*SR); ri = int(r_*SR)
        si = max(0, note_n - ai - di - ri)
        result = []
        for i, v in enumerate(out):
            if   i < ai:        env = i / ai
            elif i < ai+di:     env = 1.0 - (1.0-s_)*(i-ai)/di
            elif i < ai+di+si:  env = s_
            else:
                pos = i - ai - di - si
                env = s_ * (1.0 - pos/ri) if ri else 0.0
            result.append(v * max(0.0, env))
        return result + [0.0] * silence_n

    def bass_bar(note):
        freq = NOTE[note]; bar_ = []
        for _ in range(8): bar_.extend(bass_hit(freq))
        return bar_

    bass_seq = concat(
        bass_bar('E2'), bass_bar('E2'),   # Em ×2
        bass_bar('A2'), bass_bar('A2'),   # Am ×2
        bass_bar('C3'), bass_bar('C3'),   # C  ×2
        bass_bar('B2'), bass_bar('B2'),   # B  ×2
    )

    # ─── 베이스 옥타브 보강 ───────────────────────────────────────────
    # E2×2=E3(164Hz) vs Em멜루트 E4(329Hz): 충돌없음 ✓
    # A2×2=A3(220Hz) vs Am멜루트 A4(440Hz): 충돌없음 ✓
    # C3×2=C4(261Hz) vs C 멜루트 C5(523Hz): 충돌없음 ✓
    # B2×2=B3(246Hz) vs B 멜루트 B4(493Hz): 충돌없음 ✓
    def bass_oct_hit(freq_low, amp=0.56, mult=2.0):
        freq_oct  = freq_low * mult
        note_n    = int(SR * e * 0.88)
        silence_n = int(SR * e) - note_n
        out = [0.0] * note_n
        for i in range(note_n):
            out[i] = (amp        * math.sin(2*math.pi * freq_oct   * i / SR)
                    + amp * 0.30 * math.sin(2*math.pi * freq_oct*2 * i / SR))
        a_, d_, s_, r_ = 0.015, 0.035, 0.78, 0.18
        ai = int(a_*SR); di = int(d_*SR); ri = int(r_*SR)
        si = max(0, note_n - ai - di - ri)
        result = []
        for i, v in enumerate(out):
            if   i < ai:        env = i / ai
            elif i < ai+di:     env = 1.0 - (1.0-s_)*(i-ai)/di
            elif i < ai+di+si:  env = s_
            else:
                pos = i - ai - di - si
                env = s_ * (1.0 - pos/ri) if ri else 0.0
            result.append(v * max(0.0, env))
        return result + [0.0] * silence_n

    def bass_oct_bar(note, mult=2.0):
        freq = NOTE[note]; bar_ = []
        for _ in range(8): bar_.extend(bass_oct_hit(freq, mult=mult))
        return bar_

    bass_oct_seq = concat(
        bass_oct_bar('E2'), bass_oct_bar('E2'),
        bass_oct_bar('A2'), bass_oct_bar('A2'),
        bass_oct_bar('C3'), bass_oct_bar('C3'),
        bass_oct_bar('B2'), bass_oct_bar('B2'),
    )

    # ─── 멜로디: 스퀘어파 25% 듀티 (8비트 리드) ─────────────────────
    def sq_note(note, dur, amp=0.28, duty=0.25):
        freq   = NOTE[note]
        n      = int(SR * dur)
        period = SR / freq
        out    = [amp if (i % period) < duty * period else -amp for i in range(n)]
        a_, d_, s_, r_ = 0.005, 0.02, 0.80, 0.05
        ai = int(a_*SR); di = int(d_*SR); ri = int(r_*SR)
        si = max(0, n - ai - di - ri)
        result = []
        for i, v in enumerate(out):
            if   i < ai:        env = i / ai
            elif i < ai+di:     env = 1.0 - (1.0-s_)*(i-ai)/di
            elif i < ai+di+si:  env = s_
            else:
                pos = i - ai - di - si
                env = s_ * (1.0 - pos/ri) if ri else 0.0
            result.append(v * max(0.0, env))
        return result

    mel_seq = concat(*[sq_note(n, e) for n in [
        # Em ×2 — 하강 후 상승
        'B4','G4','E5','D5','B4','G4','E4','B3',
        'G4','A4','B4','D5','E5','B4','G4','E4',
        # Am ×2 — Am 아르페지오 윤곽
        'A4','C5','E5','C5','A4','G4','E4','A3',
        'C5','E5','G5','E5','A4','G4','E4','C4',
        # C ×2 — C 장조 상승 후 하강
        'C5','E5','G5','E5','C5','B4','G4','C4',
        'G5','E5','C5','B4','A4','G4','E4','C5',
        # B ×2 — D# 이끔음으로 긴장, E4로 토닉 해결
        'D#5','F#5','B4','D#5','F#5','D#5','B4','A4',
        'G4', 'F#4','D#4','B3', 'F#4','G4', 'B4','E4',
    ]])

    # ─── 아르페지오: 삼각파 16분음표 ─────────────────────────────────
    def tri_note(note, dur, amp=0.20):
        freq   = NOTE[note]
        n      = int(SR * dur)
        period = SR / freq
        out    = [amp * (2 * abs(2 * ((i % period) / period) - 1) - 1) for i in range(n)]
        a_, d_, s_, r_ = 0.004, 0.015, 0.75, 0.05
        ai = int(a_*SR); di = int(d_*SR); ri = int(r_*SR)
        si = max(0, n - ai - di - ri)
        result = []
        for i, v in enumerate(out):
            if   i < ai:        env = i / ai
            elif i < ai+di:     env = 1.0 - (1.0-s_)*(i-ai)/di
            elif i < ai+di+si:  env = s_
            else:
                pos = i - ai - di - si
                env = s_ * (1.0 - pos/ri) if ri else 0.0
            result.append(v * max(0.0, env))
        return result

    def arp_bar(pattern):
        parts = []
        for note in pattern * 4:
            parts.append(tri_note(note, s16))
        return concat(*parts)

    arp_seq = concat(
        arp_bar(['E3','G3','B3','G3']),       # Em
        arp_bar(['E3','G3','B3','G3']),
        arp_bar(['A2','C3','E3','C3']),       # Am
        arp_bar(['A2','C3','E3','C3']),
        arp_bar(['C3','E3','G3','E3']),       # C
        arp_bar(['C3','E3','G3','E3']),
        arp_bar(['B2','D#3','F#3','D#3']),    # B (화성단음계 D#)
        arp_bar(['B2','D#3','F#3','D#3']),
    )

    # ─── 윈드차임: 코드 전환 마디에 스윕 ────────────────────────────
    loop_n    = len(bass_seq)
    chime_buf = [0.0] * loop_n

    def add_chime(t_sec, freq, amp=0.10):
        decay = 0.75
        ring  = int(SR * 3.0)
        start = int(SR * t_sec)
        for i in range(ring):
            pos = start + i
            if pos >= loop_n: break
            t   = i / SR
            env = amp * math.exp(-decay * t)
            v   = (1.00 * math.sin(2*math.pi * freq      * t)
                 + 0.50 * math.sin(2*math.pi * freq*2    * t)
                 + 0.28 * math.sin(2*math.pi * freq*3    * t)
                 + 0.14 * math.sin(2*math.pi * freq*4    * t)
                 + 0.07 * math.sin(2*math.pi * freq*2.76 * t))
            chime_buf[pos] += env * v

    def sweep(bar_idx, notes_asc, descending, amp=0.10):
        ordered = list(reversed(notes_asc)) if descending else notes_asc
        n       = len(ordered)
        t_start = bar_idx * bar
        spacing = bar / n
        for k, freq in enumerate(ordered):
            add_chime(t_start + k * spacing, freq, amp=amp)

    Em_sc = [NOTE['E4'], NOTE['G4'], NOTE['B4'], NOTE['E5'], NOTE['G5'], NOTE['A5']]
    Am_sc = [NOTE['A4'], NOTE['C5'], NOTE['E5'], NOTE['A5']]
    C_sc  = [NOTE['C5'], NOTE['E5'], NOTE['G5'], NOTE['A5']]
    B_sc  = [NOTE['B4'], NOTE['D#5'], NOTE['F#5'], NOTE['A5']]

    sweep(0, Em_sc, descending=True)
    sweep(2, Am_sc, descending=False)
    sweep(4, C_sc,  descending=True)
    sweep(6, B_sc,  descending=False)

    # ─── 3루프 조립 ──────────────────────────────────────────────────
    bass_full     = bass_seq     * 3
    bass_oct_full = bass_oct_seq * 3
    mel_full      = mel_seq      * 3
    arp_full      = arp_seq      * 3
    chime_full    = chime_buf    * 3
    # 8마디×4비트=32비트/루프, ×3루프=96비트
    thump_full = concat(*[sub_thump(q) for _ in range(32)]) * 3

    length = max(len(bass_full), len(bass_oct_full), len(mel_full), len(arp_full), len(chime_full), len(thump_full))
    def pad(t_): return t_ + [0.0] * (length - len(t_))

    b = pad(bass_full); bo = pad(bass_oct_full)
    m = pad(mel_full);  a  = pad(arp_full); c = pad(chime_full); th = pad(thump_full)

    # 가중 믹스: bass 38 / bass_oct 17 / melody 28 / arp 11 / chime 6 / thump 8
    mixed = [b[i]*0.38 + bo[i]*0.17 + m[i]*0.28 + a[i]*0.11 + c[i]*0.06 + th[i]*0.08 for i in range(length)]
    peak  = max(abs(v) for v in mixed) if mixed else 1.0
    scale = 1.035 / peak if peak > 0 else 1.0  # +15% 음량
    mixed = [v * scale for v in mixed]

    # 루프 크로스페이드 50ms
    fade_n = int(SR * 0.05)
    out = list(mixed)
    for i in range(fade_n):
        t_ = i / fade_n
        out[-fade_n + i] = mixed[-fade_n + i]*(1.0-t_) + mixed[i]*t_

    save('bgm_nova.wav', out)


if __name__ == '__main__':
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    print('[gen_bgm_nova] E단조 Nova BGM 생성 중...\n')
    gen()
    print('\n완료!')
