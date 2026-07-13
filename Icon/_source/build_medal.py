import math
from PIL import Image, ImageDraw, ImageFont, ImageFilter

SRC1024 = r"c:\Users\ozzywow\Rebuild\Hanoi\Icon\Icon1024.png"
OUT = r"C:\Users\ozzywow\AppData\Local\Temp\claude\c--Users-ozzywow-Rebuild-Hanoi\77362d4e-c308-4a07-9002-4bca5451f045\scratchpad"
BLACKF = r"C:\Windows\Fonts\ariblk.ttf"

def make_badge_layer(S):
    """Return an RGBA layer (S x S) with the medal badge, supersampled for smooth edges."""
    SS = 4
    W = S * SS
    lay = Image.new("RGBA", (W, W), (0, 0, 0, 0))
    d = ImageDraw.Draw(lay)

    cx, cy = 0.685 * W, 0.300 * W       # inset from top-right corner (safe from rounding)
    R = 0.180 * W                        # medal radius

    # ---- ribbon tails (behind medal) ----
    def ribbon(x_off, color, dark):
        top = (cx + x_off, cy + R * 0.2)
        w = R * 0.62
        bottom_y = cy + R * 1.75
        pts = [
            (top[0] - w * 0.5, top[1]),
            (top[0] + w * 0.5, top[1]),
            (top[0] + w * 0.5, bottom_y),
            (top[0], bottom_y - w * 0.45),          # V notch
            (top[0] - w * 0.5, bottom_y),
        ]
        d.polygon(pts, fill=color)
        # inner shade stripe
        d.line([(top[0] - w * 0.18, top[1]), (top[0] - w * 0.18, bottom_y - w * 0.3)],
               fill=dark, width=int(w * 0.10))
    ribbon(-R * 0.42, (40, 96, 200, 255), (26, 66, 150, 255))    # blue left
    ribbon(+R * 0.42, (196, 40, 52, 255), (150, 24, 34, 255))    # red right

    # ---- medal disc (concentric bevel) ----
    rings = [
        (R,          (140, 92, 12, 255)),    # dark outer rim
        (R * 0.92,   (255, 220, 110, 255)),  # bright gold
        (R * 0.80,   (214, 158, 40, 255)),   # mid gold
        (R * 0.66,   (245, 205, 95, 255)),   # inner face
    ]
    for rr, col in rings:
        d.ellipse([cx - rr, cy - rr, cx + rr, cy + rr], fill=col)

    # knurled rim ticks
    for a in range(0, 360, 12):
        ar = math.radians(a)
        r1, r2 = R * 0.985, R * 0.86
        x1, y1 = cx + r1 * math.cos(ar), cy + r1 * math.sin(ar)
        x2, y2 = cx + r2 * math.cos(ar), cy + r2 * math.sin(ar)
        d.line([(x1, y1), (x2, y2)], fill=(180, 122, 20, 200), width=int(R * 0.02))

    # ---- number "1" (embossed) ----
    f = ImageFont.truetype(BLACKF, int(R * 1.15))
    def centered(txt, ox, oy, fill):
        l, t, r, b = d.textbbox((0, 0), txt, font=f)
        d.text((cx - (r - l) / 2 - l + ox, cy - (b - t) / 2 - t + oy), txt, font=f, fill=fill)
    centered("1", R * 0.03, R * 0.03, (150, 96, 10, 255))   # dark drop
    centered("1", 0, 0, (95, 60, 5, 255))                    # base
    centered("1", -R * 0.015, -R * 0.015, (255, 244, 200, 255))  # light face

    # ---- gloss highlight (upper-left of medal) ----
    gloss = Image.new("RGBA", (W, W), (0, 0, 0, 0))
    gd = ImageDraw.Draw(gloss)
    gr = R * 0.62
    gx, gy = cx - R * 0.28, cy - R * 0.34
    gd.ellipse([gx - gr, gy - gr * 0.75, gx + gr, gy + gr * 0.75], fill=(255, 255, 255, 70))
    gloss = gloss.filter(ImageFilter.GaussianBlur(W * 0.02))
    # clip gloss to medal disc
    mask = Image.new("L", (W, W), 0)
    ImageDraw.Draw(mask).ellipse([cx - R * 0.92, cy - R * 0.92, cx + R * 0.92, cy + R * 0.92], fill=255)
    lay = Image.alpha_composite(lay, Image.composite(gloss, Image.new("RGBA", (W, W), (0, 0, 0, 0)), mask))

    # downscale to S
    badge = lay.resize((S, S), Image.LANCZOS)

    # ---- soft drop shadow (separate the badge from busy bg) ----
    alpha = badge.split()[3]
    shadow = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    shadow.putalpha(alpha.point(lambda a: int(a * 0.55)))
    shadow = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    sh_mask = alpha.point(lambda a: int(a * 0.5))
    black = Image.new("RGBA", (S, S), (0, 0, 0, 255))
    shadow = Image.composite(black, Image.new("RGBA", (S, S), (0, 0, 0, 0)), sh_mask)
    shadow = shadow.filter(ImageFilter.GaussianBlur(S * 0.012))
    off = int(S * 0.008)
    shifted = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    shifted.paste(shadow, (off, off))

    out = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    out = Image.alpha_composite(out, shifted)
    out = Image.alpha_composite(out, badge)
    return out

# build master 1024
base = Image.open(SRC1024).convert("RGBA")
badge = make_badge_layer(1024)
master = Image.alpha_composite(base, badge)
master.save(f"{OUT}\\master_1024.png")

# preview sheet at various sizes
sizes = [1024, 180, 96, 64, 48]
pad = 12
W = pad + sum(min(s, 256) + pad for s in sizes)
H = 256 + 40 + pad * 2
sheet = Image.new("RGB", (W, H), (30, 30, 34))
from PIL import ImageFont as IF
lf = IF.truetype(r"C:\Windows\Fonts\arialbd.ttf", 16)
x = pad
for s in sizes:
    im = master.resize((s, s), Image.LANCZOS)
    disp = min(s, 256)
    im2 = im.resize((disp, disp), Image.NEAREST if s <= 64 else Image.LANCZOS)
    sheet.paste(im2.convert("RGB"), (x, pad))
    ImageDraw.Draw(sheet).text((x + 2, pad + 258), f"{s}px", font=lf, fill=(230, 230, 235))
    x += disp + pad
sheet.save(f"{OUT}\\_master_sheet.png")
print("done")
