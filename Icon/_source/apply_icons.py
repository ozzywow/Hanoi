import glob, os
from struct import unpack
from PIL import Image

OUT = r"C:\Users\ozzywow\AppData\Local\Temp\claude\c--Users-ozzywow-Rebuild-Hanoi\77362d4e-c308-4a07-9002-4bca5451f045\scratchpad"
ROOT = r"c:\Users\ozzywow\Rebuild\Hanoi"
master = Image.open(f"{OUT}\\master_1024.png").convert("RGBA")

def png_size(p):
    d = open(p, "rb").read(24)
    return unpack(">II", d[16:24])

targets = []
targets += glob.glob(os.path.join(ROOT, "Icon", "Icon*.png"))
targets += glob.glob(os.path.join(ROOT, "Resources", "Icon*.png"))
targets += glob.glob(os.path.join(ROOT, "proj.android", "app", "res", "mipmap-*", "ic_launcher.png"))

for p in targets:
    w, h = png_size(p)
    im = master.resize((w, h), Image.LANCZOS)
    # 1024 App Store icon must have no alpha; flatten all to RGB (art is full-bleed opaque)
    im.convert("RGB").save(p)
    print(f"{w}x{h}  {os.path.relpath(p, ROOT)}")

print("TOTAL", len(targets))
