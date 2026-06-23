"""
bgm_cosmos — D단조, BPM 108
bgm_space(신디+윈드차임) + bgm_universe(아르페지오) 블렌드
우주적, 신비로운 느낌
진행: Dm | Bb | Gm | Am  (4코드 × 2마디 = 8마디 루프)
"""
import wave, struct, math, os

SR = 44100
OUTPUT_DIR = os.path.join(os.path.dirname(__file__), '..', 'Resources', 'Sound')

NOTE = {
    'D2':  73.42,  'G2':  98.00,  'A2': 110.00,  'Bb2': 116.54, 'C3': 130.81,
    'D3': 146.83,  'E3': 164.81,  'F3': 174.61,  'G3':  196.00, 'A3': 220.00,
    'Bb3':233.08,  'C4': 261.63,  'D4': 293.66,  'E4':  329.63, 'F4': 349.23,
    'G4': 392.00,  'A4': 440.00,  'Bb4':466.16,  'C5':  523.25, 'D5': 587.33,
    'E5': 659.25,  'F5': 698.46,  'G5': 783.99,  'A5':  880.00, 'Bb5':932.33,
    'C6':1046.50,  'D6':1174.66,
}

def save(filename, samples):
    path = os.path.join(OUTPUT_DIR, filename)
    frames = b''.join(struct.pack('<h', max(-32767, min(32767, int(v * 32767)))) for v in samples)
    with wave.open(path, 'w') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(frames)
    print(f"  {filename:20s}  {len(samples)/SR:.2f}s")

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

def gen_cosmos():
    BPM  = 108
    q    = 60 / BPM
    h    = q * 2
    s16  = q / 4
    bar  = q * 4

    loop_bars = 8
    loop_n    = int(SR * bar * loop_bars)

    # ─── 베이스: compound sine (bgm_space 방식, D단조 루트) ──────────
    def bass_hit(freq_low, amp=0.80):
        note_n    = int(SR * q * 0.88)
        silence_n = int(SR * q) - note_n
        out = []
        for i in range(note_n):
            t = i / SR
            v = (amp        * math.sin(2*math.pi * freq_low   * t)
               + amp * 0.35 * math.sin(2*math.pi * freq_low*2 * t))
            a_, d_, s_, r_ = 0.025, 0.05, 0.80, 0.25
            ai, di, ri = int(a_*SR), int(d_*SR), int(r_*SR)
            si = max(0, note_n - ai - di - ri)
            if   i < ai:         env = i / ai
            elif i < ai+di:      env = 1.0 - (1.0-s_)*(i-ai)/di
            elif i < ai+di+si:   env = s_
            else:
                pos = i - ai - di - si
                env = s_ * (1.0 - pos/ri) if ri else 0.0
            out.append(v * max(0.0, env))
        return out + [0.0] * silence_n

    def bass_bar(root, fifth):
        return concat(
            bass_hit(NOTE[root]),
            bass_hit(NOTE[root]),
            bass_hit(NOTE[fifth], 0.65),
            bass_hit(NOTE[root], 0.75),
        )

    def bass_oct_hit(freq_low, amp=0.55):
        """옥타브 위 베이스 — 핸드폰 스피커 보강 (freq×2)"""
        freq_oct  = freq_low * 2
        note_n    = int(SR * q * 0.88)
        silence_n = int(SR * q) - note_n
        out = []
        for i in range(note_n):
            t = i / SR
            v = (amp        * math.sin(2*math.pi * freq_oct   * t)
               + amp * 0.30 * math.sin(2*math.pi * freq_oct*2 * t))
            a_, d_, s_, r_ = 0.015, 0.04, 0.72, 0.18
            ai, di, ri = int(a_*SR), int(d_*SR), int(r_*SR)
            si = max(0, note_n - ai - di - ri)
            if   i < ai:         env = i / ai
            elif i < ai+di:      env = 1.0 - (1.0-s_)*(i-ai)/di
            elif i < ai+di+si:   env = s_
            else:
                pos = i - ai - di - si
                env = s_ * (1.0 - pos/ri) if ri else 0.0
            out.append(v * max(0.0, env))
        return out + [0.0] * silence_n

    def bass_oct_bar(root, fifth):
        return concat(
            bass_oct_hit(NOTE[root]),
            bass_oct_hit(NOTE[root]),
            bass_oct_hit(NOTE[fifth], 0.38),
            bass_oct_hit(NOTE[root], 0.46),
        )

    bass_seq = concat(
        bass_bar('D2', 'A2'),    # Dm ×2
        bass_bar('D2', 'A2'),
        bass_bar('Bb2', 'F3'),   # Bb ×2
        bass_bar('Bb2', 'F3'),
        bass_bar('G2', 'D3'),    # Gm ×2
        bass_bar('G2', 'D3'),
        bass_bar('A2', 'E3'),    # Am ×2
        bass_bar('A2', 'E3'),
    )

    bass_oct_seq = concat(
        bass_oct_bar('D2', 'A2'),
        bass_oct_bar('D2', 'A2'),
        bass_oct_bar('Bb2', 'F3'),
        bass_oct_bar('Bb2', 'F3'),
        bass_oct_bar('G2', 'D3'),
        bass_oct_bar('G2', 'D3'),
        bass_oct_bar('A2', 'E3'),
        bass_oct_bar('A2', 'E3'),
    )

    # ─── 아르페지오: 삼각파 (bgm_universe 방식, D단조 보이싱) ────────
    def tri_note(freq, dur, amp=0.22):
        n      = int(SR * dur)
        period = SR / freq
        out    = [amp * (2 * abs(2 * ((i % period) / period) - 1) - 1) for i in range(n)]
        a_, d_, s_, r_ = 0.004, 0.015, 0.75, 0.05
        ai, di, ri = int(a_*SR), int(d_*SR), int(r_*SR)
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
            parts.append(tri_note(NOTE[note], s16))
        return concat(*parts)

    arp_seq = concat(
        arp_bar(['D3', 'F3', 'A3', 'F3']),    # Dm
        arp_bar(['D3', 'F3', 'A3', 'F3']),
        arp_bar(['Bb2', 'F3', 'Bb3', 'F3']),  # Bb
        arp_bar(['Bb2', 'F3', 'Bb3', 'F3']),
        arp_bar(['G2',  'D3', 'G3',  'D3']),  # Gm
        arp_bar(['G2',  'D3', 'G3',  'D3']),
        arp_bar(['A2',  'E3', 'A3',  'E3']),  # Am
        arp_bar(['A2',  'E3', 'A3',  'E3']),
    )

    # ─── 신디 패드: 멀티 배음 + 디튠 (bgm_space 방식, 느린 어택) ────
    def synth_chord(freqs, dur, amp=0.18, det_cents=6.0):
        n   = int(SR * dur)
        out = [0.0] * n
        det = 2 ** (det_cents / 1200)
        for freq in freqs:
            p1 = p2 = p2x = p3x = p4x = p5x = 0.0
            for i in range(n):
                p1  += 2*math.pi * freq       / SR
                p2  += 2*math.pi * (freq*det) / SR
                p2x += 2*math.pi * (freq*2)   / SR
                p3x += 2*math.pi * (freq*3)   / SR
                p4x += 2*math.pi * (freq*4)   / SR
                p5x += 2*math.pi * (freq*5)   / SR
                out[i] += amp * (0.28*math.sin(p1)
                                +0.28*math.sin(p2)
                                +0.28*math.sin(p2x)
                                +0.18*math.sin(p3x)
                                +0.12*math.sin(p4x)
                                +0.07*math.sin(p5x))
        a_, d_, s_, r_ = 0.030, 0.15, 0.65, 0.40   # 느린 어택 → 패드 질감
        ai, di, ri = int(a_*SR), int(d_*SR), int(r_*SR)
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

    Dm_ch  = [NOTE['D4'],  NOTE['F4'],  NOTE['A4'],  NOTE['D5']]
    Bb_ch  = [NOTE['Bb3'], NOTE['D4'],  NOTE['F4'],  NOTE['Bb4']]
    Gm_ch  = [NOTE['G3'],  NOTE['Bb3'], NOTE['D4'],  NOTE['G4']]
    Am_ch  = [NOTE['A3'],  NOTE['C4'],  NOTE['E4'],  NOTE['A4']]

    def synth_bar(freqs):
        return concat(synth_chord(freqs, h), synth_chord(freqs, h))

    synth_seq = concat(
        synth_bar(Dm_ch), synth_bar(Dm_ch),
        synth_bar(Bb_ch), synth_bar(Bb_ch),
        synth_bar(Gm_ch), synth_bar(Gm_ch),
        synth_bar(Am_ch), synth_bar(Am_ch),
    )

    # ─── 윈드차임: 스파스 스윕 (bgm_space 방식, 코드 전환 마디만) ────
    chime_buf = [0.0] * loop_n

    def add_chime(t_sec, freq, amp=0.12):
        decay = 0.80
        ring  = int(SR * 3.5)
        start = int(SR * t_sec)
        for i in range(ring):
            pos = start + i
            if pos >= loop_n: break
            t   = i / SR
            env = amp * math.exp(-decay * t)
            v   = (1.00 * math.sin(2*math.pi * freq       * t)
                 + 0.50 * math.sin(2*math.pi * freq * 2    * t)
                 + 0.28 * math.sin(2*math.pi * freq * 3    * t)
                 + 0.14 * math.sin(2*math.pi * freq * 4    * t)
                 + 0.07 * math.sin(2*math.pi * freq * 2.76 * t))
            chime_buf[pos] += env * v

    def sweep(bar_idx, notes_asc, descending, amp=0.12):
        ordered = list(reversed(notes_asc)) if descending else notes_asc
        n       = len(ordered)
        t_start = bar_idx * bar
        spacing = bar / n
        for k, freq in enumerate(ordered):
            add_chime(t_start + k * spacing, freq, amp=amp)

    Dm_sc = [NOTE['D5'],  NOTE['F5'],  NOTE['A5'],  NOTE['C6'],  NOTE['D6']]
    Bb_sc = [NOTE['Bb4'], NOTE['D5'],  NOTE['F5'],  NOTE['Bb5']]
    Gm_sc = [NOTE['G4'],  NOTE['Bb4'], NOTE['D5'],  NOTE['G5']]
    Am_sc = [NOTE['A4'],  NOTE['C5'],  NOTE['E5'],  NOTE['A5']]

    sweep(0, Dm_sc, descending=True)    # Dm  마디0 ↓
    sweep(2, Bb_sc, descending=False)   # Bb  마디2 ↑
    sweep(4, Gm_sc, descending=True)    # Gm  마디4 ↓
    sweep(6, Am_sc, descending=False)   # Am  마디6 ↑

    # ─── 3루프 조립 ─────────────────────────────────────────────────
    bass_full     = bass_seq     * 3
    bass_oct_full = bass_oct_seq * 3
    arp_full      = arp_seq      * 3
    synth_full    = synth_seq    * 3
    chime_full    = chime_buf    * 3
    # 8마디×4비트=32비트/루프, ×3루프=96비트
    thump_full = concat(*[sub_thump(q) for _ in range(32)]) * 3

    length = max(len(bass_full), len(bass_oct_full), len(arp_full), len(synth_full), len(chime_full), len(thump_full))
    def pad(t_): return t_ + [0.0] * (length - len(t_))

    b = pad(bass_full); bo = pad(bass_oct_full)
    a = pad(arp_full); s = pad(synth_full); c = pad(chime_full); th = pad(thump_full)

    # bass 42 / bass_oct 22 / arp 16 / synth 16 / chime 4 / thump 10
    mixed = [b[i]*0.42 + bo[i]*0.22 + a[i]*0.16 + s[i]*0.16 + c[i]*0.04 + th[i]*0.10 for i in range(length)]
    peak  = max(abs(v) for v in mixed) if mixed else 1.0
    scale = 1.035 / peak if peak > 0 else 1.0  # +15% 음량
    mixed = [v * scale for v in mixed]

    # 루프 크로스페이드
    fade_n = int(SR * 0.05)
    out = list(mixed)
    for i in range(fade_n):
        t_ = i / fade_n
        out[-fade_n + i] = mixed[-fade_n + i]*(1.0-t_) + mixed[i]*t_

    save('bgm_cosmos.wav', out)


if __name__ == '__main__':
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    print('[gen_bgm_cosmos] D단조 우주 BGM 생성...\n')
    gen_cosmos()
    print('\n완료!')
