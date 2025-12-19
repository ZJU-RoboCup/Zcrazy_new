  #!/usr/bin/env python3
"""
Generate Android mipmap icons from a source PNG (recommended 1024x1024 / 512x512 square).
Usage:
  python tools/gen_icons.py --src assets/zcrazy.png
It writes resized PNGs to android/res/mipmap-*/zcrazy.png
Requires: Pillow (pip install pillow)
"""
import argparse
from pathlib import Path
from PIL import Image

SIZES = {
    'mipmap-mdpi': 48,
    'mipmap-hdpi': 72,
    'mipmap-xhdpi': 96,
    'mipmap-xxhdpi': 144,
    'mipmap-xxxhdpi': 192,
}

ROOT = Path(__file__).resolve().parents[1]
RES  = ROOT / 'android' / 'res'

def gen(src: Path):
    img = Image.open(src).convert('RGBA')
    # ensure square by padding white
    size = max(img.width, img.height)
    canvas = Image.new('RGBA', (size, size), (255,255,255,0))
    canvas.paste(img, ((size - img.width)//2, (size - img.height)//2))
    for folder, px in SIZES.items():
        out_dir = RES / folder
        out_dir.mkdir(parents=True, exist_ok=True)
        out = out_dir / 'zcrazy.png'
        canvas.resize((px, px), Image.LANCZOS).save(out)
        print(f'wrote {out} ({px}x{px})')

if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('--src', required=True, help='source PNG (square recommended)')
    args = ap.parse_args()
    gen(Path(args.src))
