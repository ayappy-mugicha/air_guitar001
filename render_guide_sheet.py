"""Build the landscape booth HTML sheet from guide_copy."""

from __future__ import annotations

import shutil
import subprocess
import tempfile
import time
from html import escape
from pathlib import Path

from guide_copy import (
    GUIDE_SHEET_TITLE,
    GUIDE_STEPS,
    GUIDE_STROKE_ALT,
    GUIDE_WARNING,
)

ROOT = Path(__file__).resolve().parent
ASSETS = ROOT / "assets"
SHEET_PATH = ASSETS / "guide-sheet.html"
PDF_PATH = ASSETS / "guide.pdf"

SHEET_CSS = """
@import url("https://fonts.googleapis.com/css2?family=Familjen+Grotesk:wght@600;700&family=IBM+Plex+Mono:wght@600&family=Noto+Sans+JP:wght@500;700&display=swap");
@page { size: A4 landscape; margin: 10mm; }
html, body {
  margin: 0;
  padding: 0;
  background: #e6e2da;
  color: #1a1f2b;
  font-family: "Familjen Grotesk", "Noto Sans JP", "Hiragino Sans", sans-serif;
}
body {
  min-height: 190mm;
}
.sheet {
  box-sizing: border-box;
  min-height: 190mm;
  padding: 8mm 8mm 7mm;
  display: flex;
  flex-direction: column;
  gap: 6mm;
}
.sheet-top {
  display: flex;
  align-items: baseline;
  justify-content: space-between;
  gap: 8mm;
}
h1 {
  margin: 0;
  font-size: 22pt;
  font-weight: 700;
}
.warning {
  margin: 0;
  padding: 3mm 4mm;
  background: #24324a;
  color: #f4f6fa;
  border-radius: 8px;
  font-size: 12pt;
  font-weight: 600;
  white-space: nowrap;
}
.steps {
  list-style: none;
  margin: 0;
  padding: 0;
  display: grid;
  grid-template-columns: 1fr 1fr 1.7fr 1fr 1fr;
  gap: 4mm;
  flex: 1;
}
.step {
  background: #f4f1ea;
  border-radius: 10px;
  padding: 4mm;
  display: flex;
  flex-direction: column;
  gap: 3mm;
}
.num {
  font-family: "IBM Plex Mono", ui-monospace, monospace;
  font-size: 11pt;
  font-weight: 600;
  color: #d98a11;
}
h2 {
  margin: 0;
  font-size: 13pt;
  font-weight: 700;
}
.step p {
  margin: 0;
  font-size: 10.5pt;
  line-height: 1.45;
  font-weight: 500;
}
.step.is-stroke {
  background: #24324a;
  color: #f4f6fa;
}
.step.is-stroke .num { color: #ffd36a; }
.stroke-art {
  margin-top: auto;
}
.stroke-art svg {
  width: 100%;
  height: auto;
  display: block;
  border-radius: 8px;
}
"""


def _inline_stroke_svg() -> str:
    raw = (ASSETS / "guide-stroke.svg").read_text(encoding="utf-8")
    if raw.startswith("<?xml"):
        raw = raw.split("?>", 1)[-1]
    return f'<div class="stroke-art" role="img" aria-label="{escape(GUIDE_STROKE_ALT)}">{raw}</div>'


def render_sheet_html() -> str:
    items = []
    for index, step in enumerate(GUIDE_STEPS, start=1):
        media = ""
        classes = "step"
        if step["media"]:
            classes += " is-stroke"
            media = _inline_stroke_svg()
        items.append(
            "<li class=\"{classes}\">"
            "<span class=\"num\">{index}</span>"
            "<h2>{title}</h2>"
            "<p>{body}</p>"
            "{media}"
            "</li>".format(
                classes=classes,
                index=index,
                title=escape(step["title"]),
                body=escape(step["body"]),
                media=media,
            )
        )
    return (
        "<!DOCTYPE html>\n"
        '<html lang="ja">\n'
        "<head>\n"
        '<meta charset="utf-8">\n'
        f"<title>{escape(GUIDE_SHEET_TITLE)}</title>\n"
        "<style>\n"
        f"{SHEET_CSS.strip()}\n"
        "</style>\n"
        "</head>\n"
        "<body>\n"
        '<article class="sheet">\n'
        '<header class="sheet-top">\n'
        f"<h1>{escape(GUIDE_SHEET_TITLE)}</h1>\n"
        f'<p class="warning">{escape(GUIDE_WARNING)}</p>\n'
        "</header>\n"
        f'<ol class="steps">{"".join(items)}</ol>\n'
        "</article>\n"
        "</body>\n"
        "</html>\n"
    )


def write_sheet() -> Path:
    SHEET_PATH.write_text(render_sheet_html(), encoding="utf-8")
    return SHEET_PATH


def _chrome_bin() -> str | None:
    for name in ("google-chrome", "google-chrome-stable", "chromium", "chromium-browser"):
        found = shutil.which(name)
        if found:
            return found
    return None


def write_pdf(sheet_path: Path = SHEET_PATH, pdf_path: Path = PDF_PATH) -> Path:
    chrome = _chrome_bin()
    if not chrome:
        raise SystemExit("Chrome/Chromium が見つからないので PDF を書けません")
    if pdf_path.exists():
        pdf_path.unlink()
    profile = tempfile.mkdtemp(prefix="air-guitar-guide-chrome-")
    cmd = [
        chrome,
        "--headless",
        "--disable-gpu",
        "--no-sandbox",
        "--no-pdf-header-footer",
        "--allow-file-access-from-files",
        f"--user-data-dir={profile}",
        f"--print-to-pdf={pdf_path}",
        sheet_path.resolve().as_uri(),
    ]
    proc = subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    try:
        deadline = time.time() + 30
        last_size = -1
        stable = 0
        while time.time() < deadline:
            if pdf_path.exists():
                size = pdf_path.stat().st_size
                if size > 1000 and size == last_size:
                    stable += 1
                    if stable >= 4:
                        break
                else:
                    stable = 0
                last_size = size
            time.sleep(0.15)
        else:
            raise TimeoutError("PDF の書き出しが時間切れになりました")
    finally:
        proc.kill()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.terminate()
        shutil.rmtree(profile, ignore_errors=True)
    return pdf_path


if __name__ == "__main__":
    path = write_sheet()
    print(f"wrote {path}")
    pdf = write_pdf()
    print(f"wrote {pdf}")
