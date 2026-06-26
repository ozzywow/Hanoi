"""
bgm_earth — D장조, BPM 80 (웅장하고 아름다운 심포니 / G선상의 아리아 기반 변형)
메인 = 16분음표 튜바 베이스 오스티나토(묵직·리드미컬·또렷) — 분위기를 끌고감
첫 박: 타이트한 저음 킥(드럼) 으로 묵직한 임팩트
멜로디: 오케스트라 앙상블(ORCH_TBL 디튠 다성부) — 노래하는 아치 (서브)
팀파니: 프레이즈 분기·절정에만 색채로 절제 (킥과 저역 분리)
G선상의 아리아 차용: 길게 노래하는 선율 + 하행 베이스(D-C#-B-A-G-F#-E-A) + 흐르는 내성 16분
큰 틀은 기존 구성(베이스/아르페지오/패드/차임) + 새 악기(오케스트라 합성, 팀파니) 추가
진행: D | A/C# | Bm | D/A | G | F#m | Em | A  (8마디 루프 × 2)
※ 끊임없이 흐르는 16분 내성이 느린 선율에도 추진력을 유지 → 늘어지지 않음
"""
import wave, struct, math, os

SR = 44100
OUTPUT_DIR = os.path.join(os.path.dirname(__file__), '..', 'Resources', 'Sound')

NOTE = {
    'D2':  73.42,
    'E2':  82.41,  'F#2': 92.50,  'G2':  98.00,  'A2': 110.00, 'B2': 123.47, 'C#3':138.59, 'D3': 146.83,
    'E3': 164.81,  'F#3':185.00,  'G3': 196.00,  'A3': 220.00, 'B3': 246.94, 'C#4':277.18,
    'D4': 293.66,  'E4': 329.63,  'F#4':369.99,  'G4': 392.00, 'A4': 440.00, 'B4': 493.88, 'C#5':554.37,
    'D5': 587.33,  'E5': 659.25,  'F#5':739.99,  'G5': 783.99, 'A5': 880.00, 'B5': 987.77, 'C#6':1108.73, 'D6':1174.66,
}

def save(filename, samples):
    path = os.path.join(OUTPUT_DIR, filename)
    frames = b''.join(struct.pack('<h', max(-32767, min(32767, int(v * 32767)))) for v in samples)
    with wave.open(path, 'w') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
        w.writeframes(frames)
    print(f"  {filename:20s}  {len(samples)/SR:.2f}s")

def concat(*parts):
    out = []
    for p in parts: out.extend(p)
    return out

# ─── 오케스트라 음색용 웨이브테이블 (현+호른 혼합 스펙트럼, 따뜻한 1/n 롤오프) ──
TABLE_SIZE = 2048
def _make_wavetable(harmonics):
    tbl = [0.0] * TABLE_SIZE
    for n, a in enumerate(harmonics, start=1):
        for i in range(TABLE_SIZE):
            tbl[i] += a * math.sin(2*math.pi*n*i / TABLE_SIZE)
    pk = max(abs(v) for v in tbl) or 1.0
    return [v / pk for v in tbl]
# 기음 강하고 배음은 부드럽게 감쇠 → 톱니파(신스)보다 현악/호른 섹션에 가까운 질감
ORCH_TBL = _make_wavetable([1.0, 0.60, 0.40, 0.26, 0.17, 0.11, 0.07, 0.045, 0.03])
# 밝고 매끈하게 솟아오르는 리드(반젤리스 CS-80 풍) — 찬가풍 메인 보강용
LEAD_TBL = _make_wavetable([1.0, 0.55, 0.40, 0.26, 0.16, 0.09, 0.05])

def gen_earth():
    BPM  = 80
    q    = 60 / BPM
    e    = q / 2
    s16  = q / 4
    h    = q * 2
    bar  = q * 4

    loop_bars = 8
    loop_n    = int(SR * bar * loop_bars)

    # ─── 금관 음색: 위상적분 톱니파 + 1극 LPF + 비브라토 ───────────────
    #     트럼펫 = 밝게(높은 cutoff), 튜바 = 둥글게(낮은 cutoff)
    def brass(freq, dur, amp=1.0, cutoff=4500.0, attack=0.045, release=0.09,
              sustain=0.86, vib_depth=0.004, vib_rate=5.4, gap=0.04):
        n_total = int(SR * dur)
        n       = max(1, int(SR * (dur - gap)))   # 레가토 사이 미세 분리
        sil     = n_total - n
        alpha   = (2*math.pi*cutoff/SR) / (1 + 2*math.pi*cutoff/SR)
        a_, d_, s_, r_ = attack, 0.07, sustain, release
        ai, di, ri = int(a_*SR), int(d_*SR), int(r_*SR)
        si = max(0, n - ai - di - ri)
        out   = [0.0] * n_total
        phase = 0.0
        y     = 0.0
        for i in range(n):
            t   = i / SR
            vib = 1.0 + vib_depth * math.sin(2*math.pi*vib_rate*t) * min(1.0, t/0.35)
            phase += 2*math.pi * freq * vib / SR
            saw = (phase / math.pi) % 2.0 - 1.0     # 톱니파(전 배음)
            y  += alpha * (saw - y)                 # 1극 저역통과
            if   i < ai:        env = i / ai
            elif i < ai+di:     env = 1.0 - (1.0-s_)*(i-ai)/di
            elif i < ai+di+si:  env = s_
            else:
                pos = i - ai - di - si
                env = s_ * (1.0 - pos/ri) if ri else 0.0
            out[i] = amp * max(0.0, env) * y
        return out

    def brass_seq(notes, cutoff, amp, **kw):
        """notes = [(note, dur_beats), ...]"""
        parts = []
        for nm, db in notes:
            parts.append(brass(NOTE[nm], db * q, amp=amp, cutoff=cutoff, **kw))
        return concat(*parts)

    # ─── 오케스트라 리드: 디튠 다성부 앙상블 + 보잉 어택 + 비브라토 ────
    #     웨이브테이블(ORCH_TBL)을 여러 보이스로 살짝 어긋나게 읽어 '섹션' 두께 형성
    def orch_note(freq, dur, amp=1.0, detunes=(-7.0, 0.0, 7.0), table=ORCH_TBL,
                  attack=0.07, decay=0.10, sustain=0.85, release=0.16,
                  vib_depth=0.006, vib_rate=5.5, gap=0.02):
        n_total = int(SR * dur)
        n       = max(1, int(SR * (dur - gap)))
        ai, di, ri = int(attack*SR), int(decay*SR), int(release*SR)
        si = max(0, n - ai - di - ri)
        ratios = [2 ** (c / 1200.0) for c in detunes]
        phases = [0.0] * len(detunes)
        nv     = len(detunes)
        out    = [0.0] * n_total
        for i in range(n):
            t   = i / SR
            vib = 1.0 + vib_depth * math.sin(2*math.pi*vib_rate*t) * min(1.0, t/0.30)
            smp = 0.0
            for v in range(nv):
                phases[v] += freq * ratios[v] * vib * TABLE_SIZE / SR
                idx  = phases[v] % TABLE_SIZE
                i0   = int(idx); frac = idx - i0
                i1   = i0 + 1 if i0 + 1 < TABLE_SIZE else 0
                smp += table[i0] * (1.0 - frac) + table[i1] * frac
            smp /= nv
            if   i < ai:        env = i / ai
            elif i < ai+di:     env = 1.0 - (1.0-sustain)*(i-ai)/di
            elif i < ai+di+si:  env = sustain
            else:
                pos = i - ai - di - si
                env = sustain * (1.0 - pos/ri) if ri else 0.0
            out[i] = amp * max(0.0, env) * smp
        return out

    def orch_seq(notes, amp, **kw):
        parts = []
        for nm, db in notes:
            parts.append(orch_note(NOTE[nm], db * q, amp=amp, **kw))
        return concat(*parts)

    # ═══ 오케스트라 앙상블 멜로디 (G선상의 아리아 풍 — 노래하는 아치) ═══
    melody = [
        ('A5',2),('G5',1),('F#5',1),               # 1  D
        ('G5',2),('F#5',1),('E5',1),               # 2  A/C#
        ('F#5',1.5),('E5',0.5),('D5',1),('B4',1),  # 3  Bm
        ('A4',2),('D5',2),                          # 4  D/A
        ('B4',2),('C#5',1),('D5',1),               # 5  G
        ('E5',2),('F#5',1),('A5',1),               # 6  F#m  (절정)
        ('G5',2),('F#5',1),('E5',1),               # 7  Em
        ('D5',2),('C#5',1),('A4',1),               # 8  A   (해결)
    ]
    harmony = [   # 멜로디 3도 아래 (섹션 화음)
        ('F#5',2),('E5',1),('D5',1),
        ('E5',2),('D5',1),('C#5',1),
        ('D5',1.5),('C#5',0.5),('B4',1),('G4',1),
        ('F#4',2),('B4',2),
        ('G4',2),('A4',1),('B4',1),
        ('C#5',2),('D5',1),('F#5',1),
        ('E5',2),('D5',1),('C#5',1),
        ('B4',2),('A4',1),('F#4',1),
    ]
    mel_seq  = orch_seq(melody,  amp=1.0,  detunes=(-7.0, 0.0, 7.0),
                        attack=0.07, sustain=0.86, release=0.16, vib_depth=0.006, gap=0.018)
    harm_seq = orch_seq(harmony, amp=0.92, detunes=(-6.0, 6.0),
                        attack=0.09, sustain=0.82, release=0.18, vib_depth=0.005, gap=0.02)
    # ─── 반젤리스풍 리드: 메인 멜로디를 밝게 겹쳐 찬가처럼 솟아오르게(보강) ─
    #     느린 어택(부풀어 오름) + 디튠 앙상블 + 표현적 비브라토
    lead_seq = orch_seq(melody, amp=1.0, table=LEAD_TBL, detunes=(-9.0, 0.0, 9.0),
                        attack=0.13, decay=0.12, sustain=0.92, release=0.26,
                        vib_depth=0.009, vib_rate=4.8, gap=0.02)

    # ═══ 튜바 베이스 (메인) — 16분음표 오스티나토, 묵직·리드미컬·또렷 ══
    bass_roots = ['D3','C#3','B2','A2','G2','F#2','E2','A2']   # G선상 하행 베이스
    # 16스텝 악센트: 박머리는 강하게, 약박도 충분히 들리게 → 16분이 또렷이 구분됨
    ACCENT = [1.00, 0.60, 0.74, 0.60,
              0.86, 0.60, 0.74, 0.60,
              0.90, 0.60, 0.74, 0.60,
              0.84, 0.62, 0.76, 0.66]
    SIL16 = [0.0] * int(SR * s16)

    # 짧고 분리된 스타카토 + 밝은 cutoff(또렷한 어택) — 16개 음을 또박또박
    def bass16_bar(root):
        parts = []
        for k in range(16):
            parts.append(brass(NOTE[root], s16, amp=ACCENT[k], cutoff=1050,
                               attack=0.003, release=0.018, sustain=0.13,
                               vib_depth=0.0, gap=0.048))
        return concat(*parts)
    tuba_seq = concat(*[bass16_bar(r) for r in bass_roots])

    # 옥타브 어택(freq×2): 모든 16분에 가볍게 — 각 음의 윤곽/어택을 또렷하게
    def bass16_oct_bar(root):
        parts = []
        for k in range(16):
            a = ACCENT[k] * (0.40 if k in (0, 4, 8, 12) else 0.28)
            parts.append(brass(NOTE[root]*2, s16, amp=a, cutoff=1900,
                               attack=0.003, release=0.018, sustain=0.14,
                               vib_depth=0.0, gap=0.050))
        return concat(*parts)
    tuba_oct_seq = concat(*[bass16_oct_bar(r) for r in bass_roots])

    # ─── 묵직한 킥: 매 마디 첫 박에 타이트한 저음 킥(드럼) — 임팩트 ────
    kick_buf = [0.0] * loop_n
    def add_kick(t_sec, amp=0.95):
        dur = 0.20; n = int(SR * dur); start = int(SR * t_sec)
        phase = 0.0
        for i in range(n):
            pos = start + i
            if pos >= loop_n: break
            t = i / SR
            f = 48.0 + 120.0 * math.exp(-t * 48.0)   # 168→48Hz 빠른 피치드롭 = 킥 펀치
            phase += 2*math.pi * f / SR
            env  = math.exp(-t * 26.0) * min(1.0, t/0.0008)
            click = 0.22 * math.exp(-t * 350.0) * (((math.sin((i+1)*78.233)*43758.5453) % 1.0)*2 - 1)
            kick_buf[pos] += amp * (math.sin(phase) * env + click)
    for b in range(8):
        add_kick(b * bar, amp=0.95)        # 매 마디 첫 박 묵직한 킥

    # ─── 서브베이스: 맨 밑에 가장 낮은 음을 길게 지속(둥근 저음 베드) ──
    #     16분 베이스 루트의 한 옥타브 아래를 마디 내내 깔아 바닥을 채움
    def sub_bar(root):
        return brass(NOTE[root] / 2.0, bar, amp=1.0, cutoff=160,
                     attack=0.02, release=0.22, sustain=0.88,
                     vib_depth=0.0, gap=bar*0.03)
    sub_seq = concat(*[sub_bar(r) for r in bass_roots])

    # ─── 내성 아르페지오: 삼각파 16분음표 (흐르는 추진력) ─────────────
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
        arp_bar(['F#3','A3','D4','A3']),    # D
        arp_bar(['E3','A3','C#4','A3']),    # A
        arp_bar(['F#3','B3','D4','B3']),    # Bm
        arp_bar(['F#3','A3','D4','A3']),    # D/A
        arp_bar(['G3','B3','D4','B3']),     # G
        arp_bar(['F#3','A3','C#4','A3']),   # F#m
        arp_bar(['E3','G3','B3','G3']),     # Em
        arp_bar(['E3','A3','C#4','A3']),    # A
    )

    # ─── 스트링 패드: 멀티 배음 + 디튠 (느린 어택 = 현악 베드) ─────────
    def synth_chord(freqs, dur, amp=0.15, det_cents=7.0):
        n   = int(SR * dur)
        out = [0.0] * n
        det = 2 ** (det_cents / 1200)
        for freq in freqs:
            p1 = p2 = p2x = p3x = 0.0
            for i in range(n):
                p1  += 2*math.pi * freq       / SR
                p2  += 2*math.pi * (freq*det) / SR
                p2x += 2*math.pi * (freq*2)   / SR
                p3x += 2*math.pi * (freq*3)   / SR
                out[i] += amp * (0.32*math.sin(p1) + 0.32*math.sin(p2)
                                + 0.24*math.sin(p2x) + 0.14*math.sin(p3x))
        a_, d_, s_, r_ = 0.12, 0.20, 0.70, 0.35
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

    pad_chords = [
        ['D4','F#4','A4'],   # D
        ['C#4','E4','A4'],   # A
        ['B3','D4','F#4'],   # Bm
        ['D4','F#4','A4'],   # D/A
        ['G3','B3','D4'],    # G
        ['F#3','A3','C#4'],  # F#m
        ['E3','G3','B3'],    # Em
        ['C#4','E4','A4'],   # A
    ]
    pad_seq = concat(*[synth_chord([NOTE[x] for x in ch], bar) for ch in pad_chords])

    # ─── 팀파니: 튜닝된 막진동 (피치 드롭 + 깊은 감쇠) ────────────────
    timp_buf = [0.0] * loop_n
    def add_timpani(t_sec, freq, amp=0.5):
        dur   = 1.1
        n     = int(SR * dur)
        start = int(SR * t_sec)
        decay = 4.2
        phase = 0.0
        for i in range(n):
            pos = start + i
            if pos >= loop_n: break
            t  = i / SR
            f  = freq * (1.0 + 0.18 * math.exp(-32*t))   # 타격 직후 피치 살짝 하강
            phase += 2*math.pi * f / SR
            noise = ((math.sin((i+1)*12.9898) * 43758.5453) % 1.0) * 2 - 1
            env   = amp * math.exp(-decay*t) * (min(1.0, t/0.003))
            timp_buf[pos] += env * (math.sin(phase) + 0.14 * noise * math.exp(-40*t))

    # 킥이 매 마디 첫 박을 받치므로, 팀파니는 프레이즈 분기·절정에만 색채로 절제
    for b in (0, 4):                                   # 8마디 전·후반 진입
        add_timpani(b*bar, NOTE[bass_roots[b]] / 2.0, amp=0.48)
    for off in (3.0, 3.5):                             # 절정(6마디)·종지(8마디) 빌드업 롤
        add_timpani(5*bar + off*q, NOTE['A2']/2.0, amp=0.28 + 0.12*off)
        add_timpani(7*bar + off*q, NOTE['A2']/2.0, amp=0.28 + 0.12*off)

    # ─── 윈드차임(글로켄): 진입·절정 마디 반짝임 ─────────────────────
    chime_buf = [0.0] * loop_n
    def add_chime(t_sec, freq, amp=0.10):
        decay = 0.85; ring = int(SR*3.0); start = int(SR*t_sec)
        for i in range(ring):
            pos = start + i
            if pos >= loop_n: break
            t = i / SR
            env = amp * math.exp(-decay*t)
            v = (1.0*math.sin(2*math.pi*freq*t) + 0.5*math.sin(2*math.pi*freq*2*t)
                + 0.28*math.sin(2*math.pi*freq*3*t) + 0.14*math.sin(2*math.pi*freq*4*t))
            chime_buf[pos] += env * v
    def sweep(bar_idx, notes_asc, amp=0.10):
        spacing = bar / len(notes_asc)
        for k, fr in enumerate(notes_asc):
            add_chime(bar_idx*bar + k*spacing, fr, amp=amp)
    sweep(0, [NOTE['D5'],NOTE['F#5'],NOTE['A5'],NOTE['D6']], amp=0.09)   # 진입
    sweep(5, [NOTE['F#5'],NOTE['A5'],NOTE['C#6'],NOTE['D6']], amp=0.10)  # 절정

    # ═══ 믹스 (16분 베이스 메인) ══════════════════════════════════════
    layers = [
        (tuba_seq,     0.82),   # 튜바 16분 오스티나토 (메인 — 분위기를 끌고감, 약간↑)
        (tuba_oct_seq, 0.40),   # 옥타브 어택(모든 16분, 윤곽을 또렷하게, 약간↑)
        (sub_seq,      0.58),   # 맨 밑 서브베이스 (가장 낮은 음 길게 깔기, 더↑)
        (kick_buf,     0.95),   # 첫 박 묵직한 킥 (임팩트)
        (mel_seq,      0.44),   # 오케스트라 멜로디 (볼륨↓)
        (harm_seq,     0.22),   # 오케스트라 3도 화음 (볼륨↓)
        (lead_seq,     0.34),   # 반젤리스풍 솟아오르는 리드 (Chariots 보강)
        (arp_seq,      0.22),   # 흐르는 내성 16분
        (pad_seq,      0.85),   # 스트링 패드 (synth_chord 내부 amp 사용)
        (chime_buf,    0.78),
        (timp_buf,     0.70),   # 팀파니 (색채 — 킥과 저역 분리 위해 절제)
    ]
    length = max(len(s) for s, _ in layers)
    mixed = [0.0] * length
    for s, g in layers:
        for i in range(len(s)):
            mixed[i] += s[i] * g

    peak  = max(abs(v) for v in mixed) if mixed else 1.0
    scale = 0.97 / peak if peak > 0 else 1.0
    mixed = [v * scale for v in mixed]

    # 한 루프 길이에 맞춰 자르고(혹시 모를 초과분) 2회 반복 + 크로스페이드
    one = mixed[:loop_n] if len(mixed) >= loop_n else mixed + [0.0]*(loop_n-len(mixed))
    out = one * 2
    fade_n = int(SR * 0.06)
    for i in range(fade_n):
        t_ = i / fade_n
        out[-fade_n + i] = one[loop_n - fade_n + i]*(1.0-t_) + one[i]*t_

    save('bgm_earth.wav', out)


if __name__ == '__main__':
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    print('[gen_bgm_earth] D장조 금관 심포니 Earth BGM 생성...\n')
    gen_earth()
    print('\n완료!')
