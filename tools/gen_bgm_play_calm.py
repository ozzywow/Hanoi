"""
BGM Play Scene - D장조 So Cute 기반 8비트 칩튠
스펙트럼 분석 기반 (9초~): BPM 129, D→F#m→G→Bb→A
베이스  : compound sine D2+D3 옥타브, 부드럽고 묵직
신디    : 원곡 피크 D4/F#4/A4/D5 + 디튠 shimmer + 2·3배음
윈드차임: 고음역 벨 사운드, 별이 쏟아지는 효과
믹스    : bass 60% / synth 30% / chime 10%
"""
import wave, struct, math, os

SR = 44100
OUTPUT_DIR = os.path.join(os.path.dirname(__file__), '..', 'Resources', 'Sound')

NOTE = {
    'D2':  73.42,  'F#2': 92.50,  'G2':  98.00,
    'A2':  110.00, 'Bb2': 116.54,
    'D3':  146.83, 'F#3': 185.00, 'G3':  196.00,
    'A3':  220.00, 'Bb3': 233.08, 'B3':  246.94,
    'C#4': 277.18, 'D4':  293.66, 'E4':  329.63, 'F4':  349.23,
    'F#4': 369.99, 'G4':  392.00, 'A4':  440.00, 'B4':  493.88,
    'C#5': 554.37, 'D5':  587.33, 'E5':  659.25,
    # 윈드차임용 고음역
    'Bb4': 466.16, 'F5':  698.46,
    'F#5': 739.99, 'G5':  783.99, 'A5':  880.00,
    'Bb5': 932.33, 'B5':  987.77, 'C#6': 1108.73,
    'D6':  1174.66,'E6':  1318.51,'F6':  1396.91,
}

def concat(*parts):
    out = []
    for p in parts:
        out.extend(p)
    return out

def sub_thump(dur_s, amp=0.30):
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

def save(filename, samples):
    path = os.path.join(OUTPUT_DIR, filename)
    frames = b''.join(
        struct.pack('<h', max(-32767, min(32767, int(v * 32767)))) for v in samples
    )
    with wave.open(path, 'w') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
        w.writeframes(frames)
    print(f"  {filename}  {len(samples)/SR:.2f}s")


def gen():
    BPM = 129
    q   = 60 / BPM          # 4분음표 ≈ 0.4651s
    e   = q / 2             # 8분음표 ≈ 0.2326s
    s16 = q / 4             # 16분음표 ≈ 0.1163s
    h   = q * 2             # 반음표
    bar = q * 4             # 1마디

    # ─── 베이스: compound sine, 부드럽고 묵직 ────────────────────────
    def bass_hit(freq_low, amp=0.82):
        freq_hi   = freq_low * 2
        note_n    = int(SR * e * 0.88)
        silence_n = int(SR * e) - note_n
        out = [0.0] * note_n
        for i in range(note_n):
            out[i] = (amp        * math.sin(2*math.pi * freq_low * i / SR)
                    + amp * 0.40 * math.sin(2*math.pi * freq_hi  * i / SR))
        a_, d_, s_, r_ = 0.020, 0.040, 0.82, 0.13
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

    def bass_oct_hit(freq_low, amp=0.58, mult=2.0):
        """옥타브 위 베이스 — 핸드폰 스피커 보강 (기본 freq×2, 충돌 시 freq×1.5)"""
        freq_oct  = freq_low * mult
        note_n    = int(SR * e * 0.88)
        silence_n = int(SR * e) - note_n
        out = [0.0] * note_n
        for i in range(note_n):
            out[i] = (amp        * math.sin(2*math.pi * freq_oct   * i / SR)
                    + amp * 0.30 * math.sin(2*math.pi * freq_oct*2 * i / SR))
        a_, d_, s_, r_ = 0.015, 0.035, 0.78, 0.14
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

    bass_seq = concat(
        bass_bar('D2'),  bass_bar('D2'),
        bass_bar('F#2'), bass_bar('F#2'),
        bass_bar('G2'),  bass_bar('G2'),
        bass_bar('Bb2'),
        bass_bar('A2'),
    )

    bass_oct_seq = concat(
        bass_oct_bar('D2'),  bass_oct_bar('D2'),
        bass_oct_bar('F#2'), bass_oct_bar('F#2'),
        bass_oct_bar('G2'),  bass_oct_bar('G2'),
        bass_oct_bar('Bb2', mult=1.5),  # Bb3 충돌 방지 → F3(174Hz)
        bass_oct_bar('A2',  mult=1.5),  # A3  충돌 방지 → E3(165Hz)
    )

    # ─── 신디: 원곡 스펙트럼 기반, 2·3배음으로 밝게 ──────────────────
    def synth_chord(freqs, dur, amp=0.20, det_cents=5.0):
        n   = int(SR * dur)
        out = [0.0] * n
        det = 2 ** (det_cents / 1200)
        for freq in freqs:
            p1 = p2 = p2x = p3x = p4x = p5x = 0.0
            for i in range(n):
                p1  += 2*math.pi * freq       / SR
                p2  += 2*math.pi * (freq*det) / SR   # 디튠
                p2x += 2*math.pi * (freq*2)   / SR   # 2배음(옥타브)
                p3x += 2*math.pi * (freq*3)   / SR   # 3배음
                p4x += 2*math.pi * (freq*4)   / SR   # 4배음 (쨍!)
                p5x += 2*math.pi * (freq*5)   / SR   # 5배음 (쨍!!)
                out[i] += amp * (0.28*math.sin(p1)    # 기본음 줄임
                                +0.28*math.sin(p2)    # 디튠
                                +0.28*math.sin(p2x)   # 옥타브
                                +0.18*math.sin(p3x)   # 3배음 ↑
                                +0.12*math.sin(p4x)   # 4배음 (쨍)
                                +0.07*math.sin(p5x))  # 5배음 (쨍)
        a_, d_, s_, r_ = 0.006, 0.11, 0.62, 0.30
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

    def synth_bar(freqs):
        return concat(synth_chord(freqs, h), synth_chord(freqs, h))

    D_ch   = [NOTE['D4'],  NOTE['F#4'], NOTE['A4'],  NOTE['D5']]
    Fsm_ch = [NOTE['F#4'], NOTE['A4'],  NOTE['C#5']]
    GM7_ch = [NOTE['G4'],  NOTE['B4'],  NOTE['D5']]
    Bb_ch  = [NOTE['Bb3'], NOTE['D4'],  NOTE['F4']]
    A_ch   = [NOTE['A3'],  NOTE['C#4'], NOTE['E4']]

    synth_seq = concat(
        synth_bar(D_ch),   synth_bar(D_ch),
        synth_bar(Fsm_ch), synth_bar(Fsm_ch),
        synth_bar(GM7_ch), synth_bar(GM7_ch),
        synth_bar(Bb_ch),
        synth_bar(A_ch),
    )

    # ─── 윈드차임: 코드별 음계 스윕 — 별이 쏟아지는 효과 ───────────
    # 짝수 마디: 고음→저음 (샤랄랄라↓)
    # 홀수 마디: 저음→고음 (샤랄랄라↑)
    # 빠른 어택 + 지수 감쇠 + 비조화 2.5배음 → 윈드차임 금속 질감
    loop_n    = len(bass_seq)
    chime_buf = [0.0] * loop_n

    def add_chime(t_sec, freq, amp=0.10):
        # 실제 윈드차임 분석 결과 반영:
        #   감쇠: decay=0.80 → 1초 후 -6.9dB, 2초 후 -12dB (측정값과 일치)
        #   링아웃: 3.5초 (느리게 울리며 사라짐)
        #   배음: 기본음+2·3·4배음+비조화(2.76x) → 금속 튜브 질감
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
                 + 0.07 * math.sin(2*math.pi * freq * 2.76 * t))  # 비조화 배음
            chime_buf[pos] += env * v

    def sweep(bar_idx, notes_asc, descending, amp=0.10):
        """한 마디를 꽉 채우는 스윕: 간격 = bar / n 으로 자동 계산
           notes_asc: 낮은음→높은음 순 리스트
           descending=True → 높→낮, False → 낮→높"""
        ordered = list(reversed(notes_asc)) if descending else notes_asc
        n       = len(ordered)
        t_start = bar_idx * bar
        spacing = bar / n          # n개가 마디 전체에 고르게 분포
        for k, freq in enumerate(ordered):
            add_chime(t_start + k * spacing, freq, amp=amp)

    # 코드별 D장조 음계 — 넓은 음역(A4~D6)으로 확장해 충분한 음표 수 확보
    D_sc   = [NOTE['A4'],  NOTE['B4'],  NOTE['C#5'], NOTE['D5'],
              NOTE['E5'],  NOTE['F#5'], NOTE['G5'],  NOTE['A5'],
              NOTE['B5'],  NOTE['C#6'], NOTE['D6']]              # 11음 (D장조 A4~D6)

    Fsm_sc = [NOTE['F#4'], NOTE['G4'],  NOTE['A4'],  NOTE['B4'],
              NOTE['C#5'], NOTE['D5'],  NOTE['E5'],  NOTE['F#5'],
              NOTE['G5'],  NOTE['A5'],  NOTE['B5'],  NOTE['C#6']] # 12음 (F#부터 C#6)

    G_sc   = [NOTE['G4'],  NOTE['A4'],  NOTE['B4'],  NOTE['D5'],
              NOTE['E5'],  NOTE['F#5'], NOTE['G5'],  NOTE['A5'],
              NOTE['B5'],  NOTE['D6']]                            # 10음 (G장조 계열)

    Bb_sc  = [NOTE['Bb4'], NOTE['D5'],  NOTE['F5'],  NOTE['Bb5'],
              NOTE['D6'],  NOTE['F6']]                            # 6음 (BbM7 아르페지오)

    A_sc   = [NOTE['A4'],  NOTE['B4'],  NOTE['C#5'], NOTE['E5'],
              NOTE['F#5'], NOTE['A5'],  NOTE['C#6'], NOTE['E6']] # 8음 (A장조)

    # 전 마디 스윕, 방향 교대 (짝수↓ 홀수↑), 코드별 음계 적용
    sweep(0, D_sc,   descending=True)    # D    마디1: ↓
    sweep(1, D_sc,   descending=False)   # D    마디2: ↑
    sweep(2, Fsm_sc, descending=True)    # F#m  마디3: ↓
    sweep(3, Fsm_sc, descending=False)   # F#m  마디4: ↑
    sweep(4, G_sc,   descending=True)    # GM7  마디5: ↓
    sweep(5, G_sc,   descending=False)   # GM7  마디6: ↑
    sweep(6, Bb_sc,  descending=True)    # BbM7 마디7: ↓
    sweep(7, A_sc,   descending=False)   # A    마디8: ↑

    # ─── 3루프 조립 ──────────────────────────────────────────────────
    bass_full     = bass_seq     * 3
    bass_oct_full = bass_oct_seq * 3
    synth_full    = synth_seq    * 3
    chime_full    = chime_buf    * 3
    # 8마디×4비트=32비트/루프, ×3루프=96비트
    thump_full = concat(*[sub_thump(q) for _ in range(32)]) * 3

    length = max(len(bass_full), len(bass_oct_full), len(synth_full), len(chime_full), len(thump_full))
    def pad(t_): return t_ + [0.0] * (length - len(t_))

    b = pad(bass_full); bo = pad(bass_oct_full)
    s = pad(synth_full); c = pad(chime_full); th = pad(thump_full)

    # 가중 믹스: bass 44 / bass_oct 20 / synth 17 / chime 4 / thump 12
    mixed = [b[i]*0.44 + bo[i]*0.20 + s[i]*0.17 + c[i]*0.04 + th[i]*0.12 for i in range(length)]
    peak  = max(abs(v) for v in mixed) if mixed else 1.0
    scale = 1.125 / peak if peak > 0 else 1.0  # +25% 음량 (bgm_space 전용)
    mixed = [v * scale for v in mixed]

    # 루프 크로스페이드 50ms
    fade_n = int(SR * 0.05)
    out = list(mixed)
    for i in range(fade_n):
        t_ = i / fade_n
        out[-fade_n + i] = mixed[-fade_n + i]*(1.0-t_) + mixed[i]*t_

    save('bgm_space.wav', out)


if __name__ == '__main__':
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    print('[gen_bgm_play_calm] BGM 생성 중...\n')
    gen()
    print('\n완료!')
