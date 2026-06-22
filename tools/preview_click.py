"""
키보드 클릭 사운드 미리듣기 & 튜닝 도구
실행: python tools/preview_click.py

파라미터를 바꾸고 재실행하면 즉시 들을 수 있음.
"""
import sys, os, time
sys.path.insert(0, os.path.dirname(__file__))
import gen_sounds
import winsound

SOUND_DIR = os.path.abspath(gen_sounds.OUTPUT_DIR)

def play(filename, repeat=1, gap=0.25):
    path = os.path.join(SOUND_DIR, filename)
    for _ in range(repeat):
        winsound.PlaySound(path, winsound.SND_FILENAME)
        time.sleep(gap)

# ── 여기서 파라미터 조정 ──────────────────────────────────────
#
#   res_freq  : 하우징 공진 주파수 (Hz)
#               높을수록 밝고 경쾌 (1000~2000 권장)
#               낮을수록 묵직 (600~900)
#
#   thud_amp  : 저음 탁음 세기 (0.0 ~ 0.5)
#               0에 가까울수록 경쾌, 클수록 둔탁
#
#   upclick   : True → 22ms 뒤 반동 클릭 추가 (MX Blue 느낌)
#               False → 누를 때만 소리
#
TUNE = dict(
    res_freq = 1400,
    thud_amp = 0.12,
    upclick  = True,
)
# ─────────────────────────────────────────────────────────────

if __name__ == '__main__':
    os.makedirs(SOUND_DIR, exist_ok=True)

    print(f"파라미터: {TUNE}")
    print("생성 중...")

    samples = gen_sounds.keyboard_click(**TUNE)
    gen_sounds.save('FX0070.wav', samples)

    print("\n[FX0070] 3회 재생 (간격 0.35s)")
    play('FX0070.wav', repeat=3, gap=0.35)

    time.sleep(0.6)

    # FX0108도 같이 확인 (약간 낮은 공진)
    tune2 = dict(res_freq=TUNE['res_freq'] - 300,
                 thud_amp=TUNE['thud_amp'],
                 upclick=False, amp=0.85)
    samples2 = gen_sounds.keyboard_click(**tune2)
    gen_sounds.save('FX0108.wav', samples2)

    print("[FX0108] 3회 재생 (간격 0.35s)")
    play('FX0108.wav', repeat=3, gap=0.35)

    print("\n완료. TUNE 값을 바꾸고 다시 실행하세요.")
