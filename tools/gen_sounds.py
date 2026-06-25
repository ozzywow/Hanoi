"""
Hanoi Olympic - 사운드 생성 스크립트
표준 라이브러리(wave, math, struct)만 사용.
실행하면 Resources/Sound/ 에 WAV 파일 생성.
"""
import wave, struct, math, os, random

SR = 44100
OUTPUT_DIR = os.path.join(os.path.dirname(__file__), '..', 'Resources', 'Sound')

# 음표 주파수표
NOTE = {
    'B3':246.94,'C4':261.63,'D4':293.66,'E4':329.63,'F4':349.23,
    'G4':392.00,'A4':440.00,'B4':493.88,
    'C5':523.25,'D5':587.33,'E5':659.25,'F5':698.46,
    'G5':783.99,'A5':880.00,'B5':987.77,'C6':1046.50,
    'C3':130.81,'D3':146.83,'E3':164.81,'F3':174.61,
    'G3':196.00,'A3':220.00,'B3b':233.08,
}

# ── 기본 파형 생성 ──────────────────────────────────────────

def sine(freq, dur, amp=0.5):
    n = int(SR * dur)
    return [amp * math.sin(2 * math.pi * freq * i / SR) for i in range(n)]

def square(freq, dur, amp=0.4, duty=0.5):
    n = int(SR * dur)
    period = SR / freq
    return [amp if (i % period) < duty * period else -amp for i in range(n)]

def chirp(f0, f1, dur, amp=0.5):
    """선형 주파수 스윕 (사인)"""
    n = int(SR * dur)
    k = (f1 - f0) / dur
    return [amp * math.sin(2 * math.pi * (f0 + 0.5 * k * (i / SR)) * (i / SR)) for i in range(n)]

def silence(dur):
    return [0.0] * int(SR * dur)

# ── 엔벨로프 (ADSR) ─────────────────────────────────────────

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

# ── 믹싱 & 연결 ─────────────────────────────────────────────

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

# ── Biquad IIR 필터 (Audio EQ Cookbook 기반) ────────────────

def biquad(samples, ftype, freq, Q=0.707):
    """
    ftype: 'lp' | 'hp' | 'bp'
    freq: 컷오프/센터 주파수 (Hz)
    Q: 품질 계수
    """
    w0  = 2 * math.pi * freq / SR
    cw  = math.cos(w0)
    sw  = math.sin(w0)
    alpha = sw / (2 * Q)

    if ftype == 'hp':
        b0, b1, b2 = (1+cw)/2, -(1+cw), (1+cw)/2
    elif ftype == 'lp':
        b0, b1, b2 = (1-cw)/2,  (1-cw), (1-cw)/2
    else:  # 'bp'
        b0, b1, b2 = sw/2, 0.0, -sw/2

    a0 = 1 + alpha
    a1, a2 = -2*cw, 1 - alpha

    b0, b1, b2 = b0/a0, b1/a0, b2/a0
    a1, a2     = a1/a0, a2/a0

    out = []
    x1 = x2 = y1 = y2 = 0.0
    for x in samples:
        y = b0*x + b1*x1 + b2*x2 - a1*y1 - a2*y2
        x2, x1 = x1, x
        y2, y1 = y1, y
        out.append(y)
    return out


def keyboard_click(res_freq=1400, thud_amp=0.10, upclick=True, amp=0.88):
    """
    기계식 키보드 딸깍 합성 - 충격파(impulsive) 방식

    실제 키보드 클릭의 핵심:
      - 에너지의 90%가 0~3ms 안에 끝남
      - 고주파 transient (2~6kHz) 가 "딱" 느낌을 결정
      - 저음은 거의 없어야 함
    """
    TOTAL = int(SR * 0.050)   # 50ms
    out   = [0.0] * TOTAL

    # 1. 충격 transient (0~2ms) ───────────────────────────
    # 미분 필터(y = x - x_prev)로 급격한 성분만 추출 → 가장 "딱" 스러운 소리
    burst_n = int(SR * 0.002)
    raw = [random.gauss(0, 1) for _ in range(burst_n)]
    x_prev = 0.0
    for i, x in enumerate(raw):
        diff = x - x_prev          # 1차 미분 → 고주파 강조
        x_prev = x
        env = math.exp(-i / (SR * 0.0008))   # 0.8ms 감쇠
        out[i] += diff * env * 2.2

    # 2. 고주파 HP 노이즈 (0~4ms) ─────────────────────────
    # 3kHz+ 만 통과, 완전히 3ms 안에 소멸
    click_n = int(SR * 0.004)
    raw2 = [random.gauss(0, 1) for _ in range(click_n)]
    hp = biquad(raw2, 'hp', 3000, Q=1.2)
    for i in range(click_n):
        env = math.exp(-i / (SR * 0.001))    # 1ms 감쇠
        out[i] += hp[i] * 0.70 * env

    # 3. 모달 공진 (플라스틱 하우징, 다중 주파수) ─────────
    # 실제 키보드 하우징은 여러 주파수에서 동시에 진동
    modal = [
        (res_freq,       0.45, 0.003),   # 주 공진
        (res_freq * 1.6, 0.22, 0.002),   # 2배음 근처
        (res_freq * 0.6, 0.18, 0.004),   # 저배음
    ]
    for f, a, tc in modal:
        n = min(TOTAL, int(SR * tc * 4))
        for i in range(n):
            out[i] += a * math.sin(2 * math.pi * f * i / SR) * math.exp(-i / (SR * tc))

    # 4. 바텀아웃 (3ms 딜레이, 극소량) ───────────────────
    d = int(SR * 0.003)
    thud_n = min(TOTAL - d, int(2.5 / 280 * SR))   # 280Hz 2.5사이클만
    for i in range(thud_n):
        out[d + i] += thud_amp * math.sin(2 * math.pi * 280 * i / SR) * math.exp(-i / (SR * 0.003))

    # 5. 업클릭 (20ms, MX Blue 특유의 두 번째 클릭) ──────
    if upclick:
        d_up = int(SR * 0.020)
        for i in range(int(SR * 0.001)):   # 1ms 짧은 버스트
            if d_up + i < TOTAL:
                out[d_up + i] += random.gauss(0, 1) * 0.30 * math.exp(-i / (SR * 0.0004))

    # 정규화
    peak = max(abs(v) for v in out)
    if peak > 0:
        out = [v / peak * amp for v in out]
    return out


# ── WAV 출력 ────────────────────────────────────────────────

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

# ════════════════════════════════════════════════════════════
# 이펙트 사운드
# ════════════════════════════════════════════════════════════

def gen_fx0070():
    """버튼 클릭 - 기계식 키보드 경쾌한 클릭"""
    save('FX0070.wav', keyboard_click(res_freq=1400, thud_amp=0.12, upclick=True))

def gen_fx0108():
    """UI 버튼 클릭 - 기계식 키보드 (약간 낮은 공진, 업클릭 없음)"""
    save('FX0108.wav', keyboard_click(res_freq=1100, thud_amp=0.10, upclick=False, amp=0.85))

def gen_fx0066():
    """씬 전환 스윕 - 고→저 스윕"""
    s = adsr(chirp(900, 250, 0.20, 0.55), a=0.01, d=0.06, s=0.4, r=0.08)
    save('FX0066.wav', s)

def gen_fx0145():
    """카운트다운 시작 알림 - 두 번 상승 비프"""
    b1 = adsr(sine(880, 0.08, 0.6), a=0.004, d=0.03, s=0.1, r=0.04)
    b2 = adsr(sine(1100, 0.11, 0.7), a=0.004, d=0.05, s=0.1, r=0.05)
    save('FX0145.wav', concat(b1, silence(0.06), b2))

def gen_move_ok():
    """디스크 이동 성공 - 경쾌한 팝"""
    s = mix(
        adsr(sine(1200, 0.08, 0.6), a=0.002, d=0.04, s=0.0, r=0.03),
        adsr(sine(600,  0.08, 0.3), a=0.002, d=0.04, s=0.0, r=0.03),
    )
    save('move_ok.wav', s)

def gen_cancel():
    """이동 불가 / 취소 - 낮은 버저"""
    n = int(SR * 0.16)
    # 약간 디튜닝으로 거칠게
    raw = [0.4 * math.sin(2*math.pi*200*i/SR) + 0.22 * math.sin(2*math.pi*209*i/SR)
           for i in range(n)]
    s = adsr(raw, a=0.008, d=0.04, s=0.45, r=0.07)
    save('Cancel.wav', s)

def gen_select():
    """기둥 선택 가능 - 상승 스윕"""
    s = adsr(chirp(550, 900, 0.11, 0.55), a=0.005, d=0.03, s=0.5, r=0.04)
    save('select.wav', s)

def gen_deselect():
    """기둥 선택 불가 - 하강 스윕"""
    s = adsr(chirp(900, 550, 0.11, 0.50), a=0.005, d=0.02, s=0.5, r=0.04)
    save('deselect.wav', s)

def gen_levelup():
    """레벨 클리어 - C장조 아르페지오 + 화음"""
    arp_notes = ['C4', 'E4', 'G4', 'C5']
    parts = []
    for n in arp_notes:
        parts.append(adsr(sine(NOTE[n], 0.13, 0.65), a=0.008, d=0.05, s=0.6, r=0.05))
        parts.append(silence(0.015))
    # 마지막 화음
    chord = mix(
        adsr(sine(NOTE['C4'], 0.5, 0.32), a=0.01, d=0.08, s=0.55, r=0.22),
        adsr(sine(NOTE['E4'], 0.5, 0.28), a=0.01, d=0.08, s=0.55, r=0.22),
        adsr(sine(NOTE['G4'], 0.5, 0.28), a=0.01, d=0.08, s=0.55, r=0.22),
        adsr(sine(NOTE['C5'], 0.5, 0.38), a=0.01, d=0.08, s=0.55, r=0.22),
    )
    save('levelup.wav', concat(*parts, chord))

def gen_go():
    """GO! - 레이싱 스타트 고정음 삐---- (A4, 1.0s, 묵직한 하위 옥타브 추가)"""
    f = NOTE['A4']   # 440 Hz
    n = int(SR * 0.7)
    # 메인 톤: 사인 + 약한 3배음
    main = [math.sin(2*math.pi*f*i/SR) + 0.18*math.sin(2*math.pi*f*3*i/SR)
            for i in range(n)]
    main = adsr(main, a=0.006, d=0.0, s=1.0, r=0.01)
    # 하위 옥타브 (220Hz): 묵직한 바디감
    sub = [math.sin(2*math.pi*(f*0.5)*i/SR) for i in range(n)]
    sub = adsr(sub, a=0.010, d=0.0, s=1.0, r=0.01)
    # 하하위 옥타브 (110Hz): 한 옥타브 더 아래
    sub2 = [math.sin(2*math.pi*(f*0.25)*i/SR) for i in range(n)]
    sub2 = adsr(sub2, a=0.012, d=0.0, s=1.0, r=0.01)
    mixed = mix(main, [v * 0.60 for v in sub], [v * 0.45 for v in sub2])
    save('efs_go.wav', mixed)

def gen_count_sec():
    """카운트다운 틱 - 날카로운 클릭"""
    s = adsr(sine(880, 0.07, 0.72), a=0.002, d=0.02, s=0.0, r=0.04)
    save('count_sec.wav', s)

def gen_drop_coin():
    """코인 드롭 - 지수 주파수 감쇠"""
    n = int(SR * 0.28)
    raw = [0.62 * math.sin(2 * math.pi * 1400 * math.exp(-7 * i / SR) * (i / SR))
           for i in range(n)]
    s = adsr(raw, a=0.002, d=0.07, s=0.35, r=0.15)
    save('drop_coin.wav', s)

# ════════════════════════════════════════════════════════════
# BGM - 8비트 Chiptune (C장조, BPM 128)
# ════════════════════════════════════════════════════════════

def gen_bgm2():
    BPM = 128
    q  = 60 / BPM          # 4분음표
    h  = q * 2             # 2분음표
    e  = q / 2             # 8분음표
    s  = q / 4             # 16분음표

    def sq(note, dur, amp=0.35, duty=0.25):
        return adsr(square(NOTE[note], dur, amp, duty), a=0.005, d=0.02, s=0.75, r=0.04)

    def sq_bass(note, dur, amp=0.28, duty=0.5):
        return adsr(square(NOTE[note], dur, amp, duty), a=0.005, d=0.03, s=0.7, r=0.04)

    def tri(note, dur, amp=0.22):
        """삼각파 근사 (홀수 배음 합성)"""
        n = int(SR * dur)
        freq = NOTE[note]
        raw = []
        for i in range(n):
            t = i / SR
            v = sum(
                ((-1)**((k-1)//2)) * math.sin(2*math.pi*freq*k*t) / (k*k)
                for k in range(1, 9, 2)
            ) * (8 / math.pi**2)
            raw.append(amp * v)
        return adsr(raw, a=0.004, d=0.01, s=0.8, r=0.04)

    # ── 멜로디 (메인 테마, 16마디) ─────────────────────────
    # A섹션 (8마디): 밝고 경쾌
    mel_A = concat(
        # Bar 1
        sq('E5',e), sq('G5',e), sq('A5',e), sq('G5',e),
        # Bar 2
        sq('E5',q), sq('C5',q),
        # Bar 3
        sq('D5',e), sq('F5',e), sq('G5',e), sq('F5',e),
        # Bar 4
        sq('D5',q), sq('B4',q),
        # Bar 5
        sq('C5',e), sq('E5',e), sq('G5',e), sq('E5',e),
        # Bar 6
        sq('C5',e), sq('D5',e), sq('E5',q),
        # Bar 7
        sq('G5',e), sq('F5',e), sq('E5',e), sq('D5',e),
        # Bar 8
        sq('C5',h),
    )
    # B섹션 (8마디): 조금 다른 변주
    mel_B = concat(
        # Bar 9
        sq('A5',e), sq('G5',e), sq('E5',e), sq('G5',e),
        # Bar 10
        sq('A5',q), sq('E5',q),
        # Bar 11
        sq('G5',e), sq('F5',e), sq('D5',e), sq('F5',e),
        # Bar 12
        sq('G5',q), sq('D5',q),
        # Bar 13
        sq('E5',e), sq('G5',e), sq('A5',e), sq('G5',e),
        # Bar 14
        sq('C6',e), sq('B5',e), sq('A5',q),
        # Bar 15
        sq('G5',e), sq('E5',e), sq('D5',e), sq('C5',e),
        # Bar 16
        sq('G4',e), sq('A4',e), sq('B4',e), sq('C5',e),
    )
    melody = concat(mel_A, mel_B)

    # ── 베이스 (루트 + 5도) ─────────────────────────────────
    # 16마디: C C G G C C G G  /  Am Am F F C C G C
    bass_prog = [
        # A섹션
        ('C3','G3'), ('C3','G3'), ('G3','D4'), ('G3','D4'),
        ('C3','G3'), ('C3','G3'), ('G3','D4'), ('C3','G3'),
        # B섹션
        ('A3','E4'), ('A3','E4'), ('F3','C4'), ('F3','C4'),
        ('C3','G3'), ('C3','G3'), ('G3','D4'), ('C3','G3'),
    ]
    bass_parts = []
    for root, fifth in bass_prog:
        bass_parts.append(sq_bass(root, e))
        bass_parts.append(sq_bass(fifth, e))
        bass_parts.append(sq_bass(root, e))
        bass_parts.append(sq_bass(fifth, e))
    bass = concat(*bass_parts)

    # ── 아르페지오 (삼각파) ─────────────────────────────────
    arp_patterns = [
        # A섹션 (C G C G C C G C)
        ['C4','E4','G4','E4'], ['C4','E4','G4','E4'],
        ['G3','B3','D4','B3'], ['G3','B3','D4','B3'],
        ['C4','E4','G4','E4'], ['C4','E4','G4','E4'],
        ['G3','B3','D4','B3'], ['C4','E4','G4','E4'],
        # B섹션
        ['A3','C4','E4','C4'], ['A3','C4','E4','C4'],
        ['F3','A3','C4','A3'], ['F3','A3','C4','A3'],
        ['C4','E4','G4','E4'], ['C4','E4','G4','E4'],
        ['G3','B3','D4','B3'], ['C4','E4','G4','E4'],
    ]
    arp_parts = []
    for pattern in arp_patterns:
        for note in pattern * 4:   # 4박자 = 16분음표 16개
            arp_parts.append(tri(note, s))
    arp = concat(*arp_parts)

    # ── 믹싱 ────────────────────────────────────────────────
    max_len = max(len(melody), len(bass), len(arp))
    def pad(t): return t + [0.0] * (max_len - len(t))
    mixed = mix(pad(melody), pad(bass), pad(arp))

    # 3회 반복 (~40초)
    looped = mixed * 3

    save('BGM2.wav', looped)
    print(f"  {'(BGM loop x3)':20s}  {len(looped)/SR:.1f}s total")


# ════════════════════════════════════════════════════════════

if __name__ == '__main__':
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    print(f"\n[gen_sounds] 출력 폴더: {os.path.abspath(OUTPUT_DIR)}\n")

    print("--- 이펙트 ---")
    gen_fx0070()
    gen_fx0108()
    gen_fx0066()
    gen_fx0145()
    gen_move_ok()
    gen_cancel()
    gen_select()
    gen_deselect()
    gen_levelup()
    gen_go()
    gen_count_sec()
    gen_drop_coin()

    print("\n--- BGM ---")
    gen_bgm2()

    print("\n완료!")
