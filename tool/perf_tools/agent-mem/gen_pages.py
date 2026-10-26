#!/usr/bin/env python3
# Writes the synthetic page set used by bench.py: text-, image-, animated
# GIF- and effect-heavy pages, so each saving shows up on the page type it
# targets.
import os
import sys

from PIL import Image, ImageDraw

WORDS = ("agent browser layout paint memory engine style frame text line box "
         "image decode shadow layer tile font glyph advance").split()


def text_page():
    paras = []
    for i in range(400):
        words = [WORDS[(i * 7 + j * 3) % len(WORDS)] for j in range(60)]
        paras.append("<p>%d %s</p>" % (i, " ".join(words)))
    return ("<!DOCTYPE html><title>text</title><body style='font:16px sans-serif'>"
            "<h1>Text heavy</h1>%s</body>" % "".join(paras))


def image_page(out):
    tags = []
    for i in range(40):
        name = "img%02d.%s" % (i, "jpg" if i % 2 else "png")
        img = Image.new("RGB", (800, 600), (i * 6 % 256, 80, 160))
        d = ImageDraw.Draw(img)
        for k in range(0, 800, 40):
            d.line((k, 0, 800 - k, 600), fill=(255, k % 256, i * 5 % 256), width=3)
        img.save(os.path.join(out, name))
        tags.append("<img src='%s' alt='image %d'>" % (name, i))
    return ("<!DOCTYPE html><title>images</title><body>"
            "<h1>Image heavy</h1>%s</body>" % "".join(tags))


def gif_page(out):
    tags = []
    for i in range(10):
        name = "anim%02d.gif" % i
        frames = []
        for f in range(8):
            img = Image.new("P", (800, 600), (i * 20 + f) % 256)
            ImageDraw.Draw(img).rectangle((f * 90, 0, f * 90 + 80, 600), fill=255)
            frames.append(img)
        frames[0].save(os.path.join(out, name), save_all=True,
                       append_images=frames[1:], duration=50, loop=0)
        tags.append("<img src='%s' alt='animation %d'>" % (name, i))
    return ("<!DOCTYPE html><title>gifs</title><body>"
            "<h1>Animated GIFs</h1>%s</body>" % "".join(tags))


def effects_page():
    cards = []
    for i in range(120):
        cards.append("<div class=c style='animation-delay:-%dms'>card %d<button>open %d</button></div>"
                     % (i * 37, i, i))
    return ("<!DOCTYPE html><title>effects</title><style>"
            ".c{display:inline-block;width:200px;height:120px;margin:12px;"
            "border-radius:12px;box-shadow:0 8px 24px rgba(0,0,0,.4),inset 0 0 16px #08f;"
            "background:linear-gradient(#fff,#ddd);animation:f 2s infinite alternate}"
            "@keyframes f{from{opacity:.5;transform:translateY(0)}"
            "to{opacity:1;transform:translateY(6px)}}</style>"
            "<body><h1>Effects heavy</h1>%s</body>" % "".join(cards))


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "pages"
    os.makedirs(out, exist_ok=True)
    pages = {"text.html": text_page(), "images.html": image_page(out),
             "gifs.html": gif_page(out), "effects.html": effects_page()}
    for name, html in pages.items():
        with open(os.path.join(out, name), "w") as f:
            f.write(html)
        print(os.path.abspath(os.path.join(out, name)))


if __name__ == "__main__":
    main()
