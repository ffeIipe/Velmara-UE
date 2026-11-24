"""
Velmara UI Preview & QA Sheet Generator — Phase 1
Renders 4 high-resolution visual review sheets for inspecting every extracted
and procedural texture (alpha transparency, 9-slice margins, palette tinting,
and live HUD + Settings mockup compositions).
"""

from __future__ import annotations

import json
import pathlib
import zipfile
from typing import List, Tuple

import numpy as np
from PIL import Image, ImageDraw, ImageFont

ROOT = pathlib.Path(r"i:\UnrealProjects\TesisUE")
RAW_DIR = ROOT / "UI_Raw"
PROC_DIR = ROOT / "UI_Processed"
PREVIEW_DIR = PROC_DIR / "_Previews"
ARTIFACT_DIR = pathlib.Path(r"C:\Users\felipe\.gemini\antigravity\brain\333bba52-7683-4122-859a-d0c75a5cb986")
SCRATCH_FONTS = ARTIFACT_DIR / "scratch" / "fonts"

# Velmara Design Tokens (Docs/UI_Velmara_PromptKit.md)
PALETTE = {
    "Void": (10, 8, 9),
    "Charcoal": (28, 26, 28),
    "Ash": (58, 54, 56),
    "Bone": (232, 224, 208),
    "BoneMuted": (163, 156, 142),
    "Blood": (142, 15, 20),
    "BloodBright": (194, 26, 31),
    "BloodDried": (74, 10, 13),
    "Shield": (184, 192, 196),
    "Spectral": (143, 134, 168),
    "Sunlight": (217, 164, 65),
}


def extract_fonts() -> Tuple[pathlib.Path, pathlib.Path, pathlib.Path]:
    SCRATCH_FONTS.mkdir(parents=True, exist_ok=True)
    cinzel_reg = SCRATCH_FONTS / "Cinzel-Regular.ttf"
    cinzel_semi = SCRATCH_FONTS / "Cinzel-SemiBold.ttf"
    forum_reg = SCRATCH_FONTS / "Forum-Regular.ttf"
    if not cinzel_reg.exists():
        with zipfile.ZipFile(RAW_DIR / "Cinzel (1).zip") as zf:
            cinzel_reg.write_bytes(zf.read("static/Cinzel-Regular.ttf"))
            cinzel_semi.write_bytes(zf.read("static/Cinzel-SemiBold.ttf"))
    if not forum_reg.exists():
        with zipfile.ZipFile(RAW_DIR / "Forum.zip") as zf:
            forum_reg.write_bytes(zf.read("Forum-Regular.ttf"))
    return cinzel_reg, cinzel_semi, forum_reg


def load_font(path: pathlib.Path, size: int) -> ImageFont.FreeTypeFont:
    return ImageFont.truetype(str(path), size=size)


def make_checkerboard(w: int, h: int, cell: int = 12, c1=(20, 18, 21), c2=(30, 27, 31)) -> Image.Image:
    yy, xx = np.mgrid[0:h, 0:w]
    pat = ((xx // cell) + (yy // cell)) % 2
    arr = np.empty((h, w, 3), dtype=np.uint8)
    arr[pat == 0] = c1
    arr[pat == 1] = c2
    return Image.fromarray(arr, mode="RGB")


def tint_rgba(im: Image.Image, rgb: Tuple[int, int, int], alpha_mul: float = 1.0) -> Image.Image:
    arr = np.array(im.convert("RGBA"), dtype=np.float32)
    arr[..., 0] = arr[..., 0] * (rgb[0] / 255.0)
    arr[..., 1] = arr[..., 1] * (rgb[1] / 255.0)
    arr[..., 2] = arr[..., 2] * (rgb[2] / 255.0)
    arr[..., 3] = np.clip(arr[..., 3] * alpha_mul, 0.0, 255.0)
    return Image.fromarray(np.round(arr).astype(np.uint8), mode="RGBA")


def paste_fit(
    canvas: Image.Image,
    sprite: Image.Image,
    box: Tuple[int, int, int, int],
    margin_guide: List[float] | None = None,
    tint: Tuple[int, int, int] | None = None,
    pad: int = 10,
) -> None:
    x0, y0, x1, y1 = box
    bw, bh = (x1 - x0) - 2 * pad, (y1 - y0) - 2 * pad
    src = tint_rgba(sprite, tint) if tint else sprite.convert("RGBA")
    sw, sh = src.size
    scale = min(bw / sw, bh / sh)
    nw, nh = max(1, int(round(sw * scale))), max(1, int(round(sh * scale)))
    # For very thin 1D gradients (h=8 or w=8), stretch along thin axis so gradient is clearly visible
    if sh <= 12 and sw >= 128:
        nw = bw
        nh = min(bh, 48)
    elif sw <= 12 and sh >= 128:
        nw = min(bw, 56)
        nh = bh
    resized = src.resize((nw, nh), Image.Resampling.LANCZOS)
    ox = x0 + pad + (bw - nw) // 2
    oy = y0 + pad + (bh - nh) // 2
    canvas.paste(resized, (ox, oy), resized)

    if margin_guide:
        ml, mt, mr, mb = margin_guide
        draw = ImageDraw.Draw(canvas, "RGBA")
        col = (217, 164, 65, 130)  # subtle Sunlight gold 9-slice guide
        if ml > 0:
            gx = ox + int(round(ml * nw))
            draw.line([(gx, oy), (gx, oy + nh)], fill=col, width=1)
        if mr > 0:
            gx = ox + nw - int(round(mr * nw))
            draw.line([(gx, oy), (gx, oy + nh)], fill=col, width=1)
        if mt > 0:
            gy = oy + int(round(mt * nh))
            draw.line([(ox, gy), (ox + nw, gy)], fill=col, width=1)
        if mb > 0:
            gy = oy + nh - int(round(mb * nh))
            draw.line([(ox, gy), (ox + nw, gy)], fill=col, width=1)


def nine_slice_render(
    sprite: Image.Image,
    target_w: int,
    target_h: int,
    margin: List[float],
    draw_as: str = "Border",
    tint: Tuple[int, int, int] = (255, 255, 255),
    alpha_mul: float = 1.0,
) -> Image.Image:
    """Exact CPU simulation of Unreal Slate 9-slice / 3-slice drawing."""
    src = tint_rgba(sprite, tint, alpha_mul)
    sw, sh = src.size
    ml, mt, mr, mb = margin
    sl = int(round(ml * sw))
    st = int(round(mt * sh))
    sr = int(round(mr * sw))
    sb = int(round(mb * sh))
    # Cap corners to half target size (2x assets rendered at 0.5x display scale for 1080p)
    scale_2x = 0.5
    dl = min(int(round(sl * scale_2x)), target_w // 2)
    dt = min(int(round(st * scale_2x)), target_h // 2)
    dr = min(int(round(sr * scale_2x)), target_w // 2)
    db = min(int(round(sb * scale_2x)), target_h // 2)

    out = Image.new("RGBA", (target_w, target_h), (0, 0, 0, 0))
    sx = [0, sl, sw - sr, sw]
    sy = [0, st, sh - sb, sh]
    dx = [0, dl, target_w - dr, target_w]
    dy = [0, dt, target_h - db, target_h]

    for row in range(3):
        for col in range(3):
            if draw_as == "Border" and row == 1 and col == 1:
                continue
            cw, ch = sx[col + 1] - sx[col], sy[row + 1] - sy[row]
            tw, th = dx[col + 1] - dx[col], dy[row + 1] - dy[row]
            if cw <= 0 or ch <= 0 or tw <= 0 or th <= 0:
                continue
            tile = src.crop((sx[col], sy[row], sx[col + 1], sy[row + 1])).resize((tw, th), Image.Resampling.LANCZOS)
            out.paste(tile, (dx[col], dy[row]), tile)
    return out


def draw_header(canvas: Image.Image, title: str, subtitle: str, f_title, f_sub) -> None:
    draw = ImageDraw.Draw(canvas)
    draw.rectangle([0, 0, canvas.width, 72], fill=(12, 10, 12))
    draw.line([(0, 72), (canvas.width, 72)], fill=(142, 15, 20), width=2)
    draw.text((28, 12), title, fill=PALETTE["Bone"], font=f_title)
    draw.text((28, 44), subtitle, fill=PALETTE["BoneMuted"], font=f_sub)


def draw_card(
    canvas: Image.Image,
    x: int,
    y: int,
    w: int,
    h: int,
    label: str,
    meta: str,
    f_lbl,
    f_meta,
) -> Tuple[int, int, int, int]:
    draw = ImageDraw.Draw(canvas)
    draw.rectangle([x, y, x + w, y + h], fill=(16, 14, 17), outline=(52, 46, 50), width=1)
    footer_h = 38
    chk = make_checkerboard(w - 2, h - footer_h - 2, cell=10)
    canvas.paste(chk, (x + 1, y + 1))
    draw.line([(x, y + h - footer_h), (x + w, y + h - footer_h)], fill=(44, 40, 44), width=1)
    draw.text((x + 8, y + h - footer_h + 4), label, fill=PALETTE["Bone"], font=f_lbl)
    draw.text((x + 8, y + h - footer_h + 20), meta, fill=PALETTE["BoneMuted"], font=f_meta)
    return (x + 1, y + 1, x + w - 1, y + h - footer_h - 1)


def save_preview(img: Image.Image, filename: str) -> None:
    PREVIEW_DIR.mkdir(parents=True, exist_ok=True)
    img.save(PREVIEW_DIR / filename, optimize=True)
    img.save(ARTIFACT_DIR / filename, optimize=True)


def build_sheet_01(manifest: dict, f_title, f_sub, f_lbl, f_meta) -> None:
    """All extracted raw assets from Asset_01..Asset_08."""
    extracted = [a for a in manifest["assets"] if not a["source"].startswith("Procedural")]
    # Separate wide vs compact assets for clean layout
    compact = [a for a in extracted if a["width"] / a["height"] < 2.6 and a["category"] != "Backgrounds"]
    wide = [a for a in extracted if a["width"] / a["height"] >= 2.6]
    bgs = [a for a in extracted if a["category"] == "Backgrounds"]

    W = 1920
    # 6 columns for compact (29 assets -> 5 rows)
    cols = 6
    margin_x = 24
    gap = 14
    card_w = (W - 2 * margin_x - (cols - 1) * gap) // cols
    card_h = 186
    rows_compact = (len(compact) + cols - 1) // cols

    # Wide items in 2 columns
    wide_cols = 2
    wide_w = (W - 2 * margin_x - gap) // wide_cols
    wide_h = 128
    rows_wide = (len(wide) + wide_cols - 1) // wide_cols

    bg_h = 260
    H = 92 + rows_compact * (card_h + gap) + 32 + rows_wide * (wide_h + gap) + 32 + bg_h + 36
    canvas = Image.new("RGB", (W, H), (10, 8, 9))
    draw_header(
        canvas,
        "VELMARA UI — SHEET 01: EXTRACTED & ALPHA-CLEANED ASSETS (Asset_01 – Asset_08)",
        f"{len(extracted)} assets | Pure #FFFFFF + straight alpha over transparency checkerboard | Ready for UE5 UserInterface2D",
        f_title, f_sub,
    )

    y = 92
    for i, a in enumerate(compact):
        r, c = divmod(i, cols)
        cx = margin_x + c * (card_w + gap)
        cy = y + r * (card_h + gap)
        box = draw_card(canvas, cx, cy, card_w, card_h, a["name"], f"{a['width']}x{a['height']}  |  {a['source'].split('_')[0]}_{a['source'].split('_')[1]}", f_lbl, f_meta)
        im = Image.open(PROC_DIR / a["rel_path"])
        paste_fit(canvas, im, box, tint=PALETTE["Bone"], pad=12)

    y += rows_compact * (card_h + gap) + 18
    draw = ImageDraw.Draw(canvas)
    draw.text((margin_x, y), "WIDE CRESTS, SEPARATORS & BLOOD BRUSH STROKES (Asset_02 & Asset_07)", fill=PALETTE["BloodBright"], font=f_sub)
    y += 26
    for i, a in enumerate(wide):
        r, c = divmod(i, wide_cols)
        cx = margin_x + c * (wide_w + gap)
        cy = y + r * (wide_h + gap)
        box = draw_card(canvas, cx, cy, wide_w, wide_h, a["name"], f"{a['width']}x{a['height']}  |  {a['category']}", f_lbl, f_meta)
        im = Image.open(PROC_DIR / a["rel_path"])
        tint = PALETTE["BloodBright"] if a["category"] == "Masks" else PALETTE["Bone"]
        paste_fit(canvas, im, box, tint=tint, pad=10)

    y += rows_wide * (wide_h + gap) + 18
    for a in bgs:
        box = draw_card(canvas, margin_x, y, 520, bg_h, a["name"], f"{a['width']}x{a['height']} | 16:9 letterbox-cleaned RGB", f_lbl, f_meta)
        im = Image.open(PROC_DIR / a["rel_path"]).convert("RGBA")
        paste_fit(canvas, im, box, pad=6)
        # Notes panel beside BG
        draw.text((margin_x + 548, y + 18), "CLEANUP SUMMARY (Asset_01 – Asset_08):", fill=PALETTE["Bone"], font=f_sub)
        notes_lines = [
            "• Background floor <= 12 clamped to exact Alpha=0 (eliminates AI near-black 1..5 haze).",
            "• All monochrome assets written as pure RGB (255,255,255) + straight alpha (zero bilinear dark fringe in UE).",
            "• Bar finials mirrored pixel-perfectly (_L / _R); Boss Crest & Separators centered on exact horizontal axis.",
            "• Combat/Menu icons normalized to 128x128 (82% safe area); Weapon icons at 256x256 for fine gothic detail.",
            "• Asset_08 Main Menu key art trimmed of 23px top/bottom letterbox bars to exact 16:9 (1368x768).",
        ]
        for k, line in enumerate(notes_lines):
            draw.text((margin_x + 548, y + 54 + k * 28), line, fill=PALETTE["BoneMuted"], font=f_meta)

    save_preview(canvas, "preview_01_extracted_assets.png")


def build_sheet_02(manifest: dict, f_title, f_sub, f_lbl, f_meta) -> None:
    """All procedural Frames & Square/Diamond Slots with 9-slice guides."""
    frames = [a for a in manifest["assets"] if a["category"] == "Frames"]
    W = 1920
    margin_x = 24
    gap = 16
    cols = 4
    card_w = (W - 2 * margin_x - (cols - 1) * gap) // cols
    card_h = 260
    rows = (len(frames) + cols - 1) // cols
    H = 92 + rows * (card_h + gap) + 28
    canvas = Image.new("RGB", (W, H), (10, 8, 9))
    draw_header(
        canvas,
        "VELMARA UI — SHEET 02: PROCEDURAL 9-SLICE / 3-SLICE FRAMES & SLOTS",
        f"{len(frames)} procedural assets (4x supersampled, bit-exact symmetry) | Gold guides = exact normalized UE Slate margins",
        f_title, f_sub,
    )

    y = 92
    for i, a in enumerate(frames):
        r, c = divmod(i, cols)
        cx = margin_x + c * (card_w + gap)
        cy = y + r * (card_h + gap)
        m_str = f"Margin={a['margin'][0]:.3f}/{a['margin'][1]:.3f}" if a["margin"] else "Fixed Image"
        box = draw_card(canvas, cx, cy, card_w, card_h, a["name"], f"{a['width']}x{a['height']} | {a['draw_as']} | {m_str}", f_lbl, f_meta)
        im = Image.open(PROC_DIR / a["rel_path"])
        tint = PALETTE["BloodBright"] if ("Selected" in a["name"] or "Sidebar_Active" in a["name"]) else PALETTE["Bone"]
        paste_fit(canvas, im, box, margin_guide=a["margin"], tint=tint, pad=14)

    save_preview(canvas, "preview_02_procedural_frames_slots.png")


def build_sheet_03(manifest: dict, f_title, f_sub, f_lbl, f_meta) -> None:
    """All procedural Bars, Controls, Small Ornaments/Markers, and Soft Gradients."""
    proc_bars = [a for a in manifest["assets"] if a["category"] == "Bars" and a["source"].startswith("Procedural")]
    proc_orns = [a for a in manifest["assets"] if a["category"] == "Ornaments" and a["source"].startswith("Procedural")]
    grads = [a for a in manifest["assets"] if a["category"] == "Gradients"]

    W = 1920
    margin_x = 24
    gap = 14

    # Section 1: Wide bars (2 cols)
    wide_bars = [a for a in proc_bars if a["width"] >= 512]
    small_bars = [a for a in proc_bars if a["width"] < 512]
    bw = (W - 2 * margin_x - gap) // 2
    bh = 112
    r_wb = (len(wide_bars) + 1) // 2

    # Section 2: Small bar parts + procedural markers/ornaments (6 cols)
    small_all = small_bars + proc_orns
    cols_s = 6
    sw = (W - 2 * margin_x - (cols_s - 1) * gap) // cols_s
    sh = 164
    r_s = (len(small_all) + cols_s - 1) // cols_s

    # Section 3: Gradients (5 cols)
    gw = (W - 2 * margin_x - 4 * gap) // 5
    gh = 176

    H = 96 + r_wb * (bh + gap) + 40 + r_s * (sh + gap) + 40 + gh + 36
    canvas = Image.new("RGB", (W, H), (10, 8, 9))
    draw_header(
        canvas,
        "VELMARA UI — SHEET 03: PROCEDURAL BARS, MASKS, MARKERS & HOVER GRADIENTS",
        f"{len(wide_bars)} bar/slider strips + {len(small_all)} pips/markers/ornaments + {len(grads)} smooth gradients | Gold guides = 3-slice margins",
        f_title, f_sub,
    )

    y = 92
    for i, a in enumerate(wide_bars):
        r, c = divmod(i, 2)
        cx = margin_x + c * (bw + gap)
        cy = y + r * (bh + gap)
        m_str = f"3-slice L/R={a['margin'][0]:.3f}" if a["margin"] else "Mask / Fill"
        box = draw_card(canvas, cx, cy, bw, bh, a["name"], f"{a['width']}x{a['height']} | {a['draw_as']} | {m_str}", f_lbl, f_meta)
        im = Image.open(PROC_DIR / a["rel_path"])
        if "Shield" in a["name"]:
            tint = PALETTE["Shield"]
        elif "FillMask" in a["name"]:
            tint = PALETTE["Blood"]
        else:
            tint = PALETTE["Bone"]
        paste_fit(canvas, im, box, margin_guide=a["margin"], tint=tint, pad=10)

    y += r_wb * (bh + gap) + 16
    draw = ImageDraw.Draw(canvas)
    draw.text((margin_x, y), "PIPS, THUMBS, HOLD RING, CHEVRONS, ROW STAR & TAB ORNAMENTS", fill=PALETTE["BloodBright"], font=f_sub)
    y += 24
    for i, a in enumerate(small_all):
        r, c = divmod(i, cols_s)
        cx = margin_x + c * (sw + gap)
        cy = y + r * (sh + gap)
        box = draw_card(canvas, cx, cy, sw, sh, a["name"], f"{a['width']}x{a['height']} | {a['category']}", f_lbl, f_meta)
        im = Image.open(PROC_DIR / a["rel_path"])
        tint = PALETTE["Spectral"] if "Pip_Diamond_Filled" in a["name"] else (PALETTE["BloodBright"] if "TabActive" in a["name"] else PALETTE["Bone"])
        paste_fit(canvas, im, box, tint=tint, pad=14)

    y += r_s * (sh + gap) + 16
    draw.text((margin_x, y), "PROCEDURAL SOFT ALPHA GRADIENTS (TINTED BLOOD / VOID OVER CHECKERBOARD)", fill=PALETTE["BloodBright"], font=f_sub)
    y += 24
    for i, a in enumerate(grads):
        cx = margin_x + i * (gw + gap)
        box = draw_card(canvas, cx, y, gw, gh, a["name"], f"{a['width']}x{a['height']} | Smooth float alpha", f_lbl, f_meta)
        im = Image.open(PROC_DIR / a["rel_path"])
        tint = PALETTE["BloodBright"] if "HFade" in a["name"] else PALETTE["Blood"]
        paste_fit(canvas, im, box, tint=tint, pad=14)

    save_preview(canvas, "preview_03_procedural_bars_markers_grads.png")


def build_sheet_04_live_composition(manifest: dict, f_cinzel, f_cinzel_bold, f_forum) -> None:
    """Live composition test using ONLY processed T_UI_* textures + Cinzel/Forum fonts."""
    by_name = {a["name"]: a for a in manifest["assets"]}

    def load_tex(name: str) -> Image.Image:
        return Image.open(PROC_DIR / by_name[name]["rel_path"]).convert("RGBA")

    W, H = 1920, 1080
    # Backdrop: darkened T_UI_Bg_MainMenu
    bg = Image.open(PROC_DIR / "Backgrounds" / "T_UI_Bg_MainMenu.png").convert("RGB").resize((W, H), Image.Resampling.LANCZOS)
    bg_arr = (np.array(bg, dtype=np.float32) * 0.42).astype(np.uint8)
    canvas = Image.fromarray(bg_arr, mode="RGB").convert("RGBA")
    draw = ImageDraw.Draw(canvas, "RGBA")

    f_h1 = load_font(f_cinzel, 34)
    f_h2 = load_font(f_cinzel, 22)
    f_tab = load_font(f_cinzel, 16)
    f_row = load_font(f_cinzel, 17)
    f_val = load_font(f_cinzel_bold, 17)
    f_body = load_font(f_forum, 22)
    f_small = load_font(f_cinzel, 13)
    f_boss = load_font(f_cinzel, 24)

    # =========================================================================
    # SECTION A (TOP HALF): SETTINGS / AJUSTES LIVE COMPOSITION (Mockup_Ajustes)
    # =========================================================================
    # Top Nav Bar
    tabs = ["EQUIPO", "INVENTARIO", "HABILIDADES", "MAPA", "AJUSTES", "DIARIO"]
    tab_div = tint_rgba(load_tex("T_UI_Orn_TabDivider"), PALETTE["BoneMuted"]).resize((16, 32), Image.Resampling.LANCZOS)
    tx = 290
    ty = 34
    draw.line([(210, ty + 40), (1540, ty + 40)], fill=(163, 156, 142, 100), width=1)
    for idx, t in enumerate(tabs):
        is_act = (t == "AJUSTES")
        col = PALETTE["BloodBright"] if is_act else PALETTE["BoneMuted"]
        tw_est = 165
        bbox = draw.textbbox((0, 0), t, font=f_tab)
        text_w = bbox[2] - bbox[0]
        draw.text((tx + (tw_est - text_w) // 2, ty + 8), t, fill=col, font=f_tab)
        if is_act:
            act_line = tint_rgba(load_tex("T_UI_Orn_TabActiveLine"), PALETTE["BloodBright"]).resize((190, 24), Image.Resampling.LANCZOS)
            canvas.alpha_composite(act_line, (tx + (tw_est - 190) // 2, ty + 28))
        if idx < len(tabs) - 1:
            canvas.alpha_composite(tab_div, (tx + tw_est + 10, ty + 2))
        tx += tw_est + 36

    # Top-Right Velmara Crescent Emblem
    emblem = tint_rgba(load_tex("T_UI_Orn_Crest_Velmara"), PALETTE["Bone"]).resize((44, 132), Image.Resampling.LANCZOS)
    canvas.alpha_composite(emblem, (1760, 24))

    # Left Sidebar ("AJUSTES" + Categories)
    v_line = tint_rgba(load_tex("T_UI_Orn_SidebarLine_V"), PALETTE["Bone"]).resize((24, 380), Image.Resampling.LANCZOS)
    canvas.alpha_composite(v_line, (42, 130))
    draw.text((78, 152), "A J U S T E S", fill=PALETTE["Bone"], font=f_h1)
    sep_s = tint_rgba(load_tex("T_UI_Orn_Sep_Star"), PALETTE["Bone"]).resize((330, 36), Image.Resampling.LANCZOS)
    canvas.alpha_composite(sep_s, (74, 190))

    cats = ["JUEGO", "PANTALLA", "SONIDO", "CONTROLES", "ACCESIBILIDAD"]
    diam_h = load_tex("T_UI_Marker_Diamond_Hollow").resize((24, 24), Image.Resampling.LANCZOS)
    cy = 236
    for i, c in enumerate(cats):
        if i == 0:
            grad_ltr = tint_rgba(load_tex("T_UI_Grad_HFade_LeftToRight"), PALETTE["Blood"], 0.72).resize((330, 52), Image.Resampling.LANCZOS)
            sb_frame = tint_rgba(load_tex("T_UI_Frame_Sidebar_Active"), PALETTE["BloodBright"], 0.95).resize((330, 52), Image.Resampling.LANCZOS)
            canvas.alpha_composite(grad_ltr, (72, cy))
            canvas.alpha_composite(sb_frame, (72, cy))
            canvas.alpha_composite(tint_rgba(diam_h, PALETTE["BloodBright"]), (92, cy + 14))
            draw.text((134, cy + 15), c, fill=PALETTE["Bone"], font=f_row)
        else:
            canvas.alpha_composite(tint_rgba(diam_h, PALETTE["BoneMuted"]), (92, cy + 14))
            draw.text((134, cy + 15), c, fill=PALETTE["BoneMuted"], font=f_row)
        cy += 60

    # Center Column: Settings Option Rows using 9-slice T_UI_Frame_OptionRow + T_UI_Orn_Chevron_L + T_UI_Orn_RowStar
    draw.text((488, 174), "J U E G O", fill=PALETTE["Bone"], font=f_h2)
    draw.line([(610, 188), (1120, 188)], fill=(163, 156, 142, 110), width=1)
    opt_rows = [
        ("DIFICULTAD", "NORMAL", False),
        ("AYUDA EN COMBATE", "DESACTIVADA", False),
        ("SUBTÍTULOS", "ACTIVADOS", True),
        ("VIBRACIÓN", "ACTIVADA", False),
    ]
    row_tex = load_tex("T_UI_Frame_OptionRow")
    row_m = by_name["T_UI_Frame_OptionRow"]["margin"]
    chev_l = load_tex("T_UI_Orn_Chevron_L").resize((24, 36), Image.Resampling.LANCZOS)
    row_star = load_tex("T_UI_Orn_RowStar").resize((24, 24), Image.Resampling.LANCZOS)
    ry = 210
    row_w, row_h = 636, 58
    for lbl, val, is_hov in opt_rows:
        # Translucent Void panel fill
        draw.rectangle([488, ry, 488 + row_w, ry + row_h], fill=(10, 8, 9, 175))
        if is_hov:
            hov_g = tint_rgba(load_tex("T_UI_Grad_HFade_Center"), PALETTE["Blood"], 0.78).resize((row_w - 4, row_h - 4), Image.Resampling.LANCZOS)
            canvas.alpha_composite(hov_g, (490, ry + 2))
        frame_tint = PALETTE["BloodBright"] if is_hov else PALETTE["BoneMuted"]
        rf = nine_slice_render(row_tex, row_w, row_h, row_m, "Border", frame_tint, 0.95 if is_hov else 0.72)
        canvas.alpha_composite(rf, (488, ry))
        # Inner left chevron for value selector + bottom star centered under value
        val_x_center = 488 + 472
        canvas.alpha_composite(tint_rgba(chev_l, frame_tint), (488 + 340, ry + 11))
        canvas.alpha_composite(tint_rgba(row_star, PALETTE["Bone"]), (val_x_center - 12, ry + row_h - 14))
        draw.text((536, ry + 19), lbl, fill=PALETTE["Bone"] if is_hov else PALETTE["BoneMuted"], font=f_row)
        vb = draw.textbbox((0, 0), val, font=f_val)
        draw.text((val_x_center - (vb[2] - vb[0]) // 2, ry + 19), val, fill=PALETTE["Bone"], font=f_val)
        ry += row_h + 12

    # Right Description Panel with 9-slice T_UI_Frame_Brackets_Panel
    px, py, pw, ph = 1190, 170, 560, 340
    draw.rectangle([px + 4, py + 4, px + pw - 4, py + ph - 4], fill=(10, 8, 9, 175))
    bp = nine_slice_render(
        load_tex("T_UI_Frame_Brackets_Panel"), pw, ph,
        by_name["T_UI_Frame_Brackets_Panel"]["margin"], "Border", PALETTE["Bone"], 0.92
    )
    canvas.alpha_composite(bp, (px, py))
    draw.text((px + 40, py + 34), "S U B T Í T U L O S", fill=PALETTE["Bone"], font=f_h2)
    sep_d = tint_rgba(load_tex("T_UI_Orn_Sep_Diamond"), PALETTE["Bone"]).resize((480, 22), Image.Resampling.LANCZOS)
    canvas.alpha_composite(sep_d, (px + 40, py + 64))
    draw.text(
        (px + 40, py + 104),
        "Activa o desactiva los subtítulos durante el juego,\nlas cinemáticas y los diálogos. Algunas líneas\npueden contener información importante.",
        fill=PALETTE["Bone"], font=f_body, spacing=8,
    )

    # =========================================================================
    # SECTION B (BOTTOM HALF): HUD M01 + BOSS M02 + INVENTORY SLOTS LIVE TEST
    # =========================================================================
    draw.line([(48, 560), (1872, 560)], fill=(142, 15, 20, 180), width=2)
    draw.text((64, 574), "LIVE HUD M01 & BOSS M02 COMPOSITION TEST (ALL GENERATED T_UI_* ASSETS)", fill=PALETTE["BoneMuted"], font=f_small)

    # --- Player Vitals (Bottom-Left Top) ---
    vx, vy = 84, 636
    # Shield Bar (65%)
    sh_w, sh_h = 420, 18
    sh_bg = tint_rgba(load_tex("T_UI_Bar_Shield_FillMask"), PALETTE["Charcoal"], 0.85).resize((sh_w, sh_h), Image.Resampling.LANCZOS)
    sh_fill_full = tint_rgba(load_tex("T_UI_Bar_Shield_FillMask"), PALETTE["Shield"], 0.95).resize((sh_w, sh_h), Image.Resampling.LANCZOS)
    sh_fill = sh_fill_full.crop((0, 0, int(sh_w * 0.65), sh_h))
    sh_frame = nine_slice_render(load_tex("T_UI_Bar_Shield_Frame"), sh_w, sh_h, by_name["T_UI_Bar_Shield_Frame"]["margin"], "Border", PALETTE["Bone"], 0.85)
    canvas.alpha_composite(sh_bg, (vx, vy))
    canvas.alpha_composite(sh_fill, (vx, vy))
    canvas.alpha_composite(sh_frame, (vx, vy))

    # Health Bar (70% Blood + 85% BloodDried damage trail) + Asset_01 Finials
    hy = vy + 24
    hb_w, hb_h = 460, 32
    hb_bg = tint_rgba(load_tex("T_UI_Bar_Health_FillMask"), PALETTE["Charcoal"], 0.9).resize((hb_w, hb_h), Image.Resampling.LANCZOS)
    hb_trail = tint_rgba(load_tex("T_UI_Bar_Health_FillMask"), PALETTE["BloodDried"], 1.0).resize((hb_w, hb_h), Image.Resampling.LANCZOS).crop((0, 0, int(hb_w * 0.84), hb_h))
    hb_fill = tint_rgba(load_tex("T_UI_Bar_Health_FillMask"), PALETTE["Blood"], 1.0).resize((hb_w, hb_h), Image.Resampling.LANCZOS).crop((0, 0, int(hb_w * 0.68), hb_h))
    hb_frame = nine_slice_render(load_tex("T_UI_Bar_Health_Frame"), hb_w, hb_h, by_name["T_UI_Bar_Health_Frame"]["margin"], "Border", PALETTE["Bone"], 0.95)
    canvas.alpha_composite(hb_bg, (vx, hy))
    canvas.alpha_composite(hb_trail, (vx, hy))
    canvas.alpha_composite(hb_fill, (vx, hy))
    canvas.alpha_composite(hb_frame, (vx, hy))
    fin_l = tint_rgba(load_tex("T_UI_Bar_Finial_Simple_L"), PALETTE["Bone"]).resize((60, 40), Image.Resampling.LANCZOS)
    fin_r = tint_rgba(load_tex("T_UI_Bar_Finial_Simple_R"), PALETTE["Bone"]).resize((60, 40), Image.Resampling.LANCZOS)
    canvas.alpha_composite(fin_l, (vx - 46, hy - 4))
    canvas.alpha_composite(fin_r, (vx + hb_w - 14, hy - 4))

    # 5 Spectral Energy Diamond Pips (3 filled Spectral, 2 hollow Ash)
    pip_f = tint_rgba(load_tex("T_UI_Pip_Diamond_Filled"), PALETTE["Spectral"]).resize((24, 24), Image.Resampling.LANCZOS)
    pip_h = tint_rgba(load_tex("T_UI_Pip_Diamond_Hollow"), PALETTE["BoneMuted"]).resize((24, 24), Image.Resampling.LANCZOS)
    for k in range(5):
        canvas.alpha_composite(pip_f if k < 3 else pip_h, (vx + 12 + k * 30, hy + 40))

    # --- 4 Diamond HUD Skill Slots (Bottom-Left) ---
    skill_icons = ["T_UI_Icon_Stat_Damage", "T_UI_Icon_Stat_Speed", "T_UI_Icon_Stat_Spectral", "T_UI_Icon_Item_Vial"]
    keys = ["Q", "E", "R", "F"]
    ds_mask = tint_rgba(load_tex("T_UI_Slot_Diamond_FillMask"), PALETTE["Void"], 0.78).resize((96, 96), Image.Resampling.LANCZOS)
    ds_norm = tint_rgba(load_tex("T_UI_Slot_Diamond_Normal"), PALETTE["Bone"], 0.85).resize((96, 96), Image.Resampling.LANCZOS)
    ds_sel = tint_rgba(load_tex("T_UI_Slot_Diamond_Selected"), PALETTE["BloodBright"], 0.95).resize((96, 96), Image.Resampling.LANCZOS)
    for k, (iname, key_lbl) in enumerate(zip(skill_icons, keys)):
        sx = 84 + k * 108
        sy = 780
        canvas.alpha_composite(ds_mask, (sx, sy))
        canvas.alpha_composite(ds_sel if k == 0 else ds_norm, (sx, sy))
        ic = tint_rgba(load_tex(iname), PALETTE["Bone"]).resize((44, 44), Image.Resampling.LANCZOS)
        canvas.alpha_composite(ic, (sx + 26, sy + 26))
        draw.text((sx + 42, sy + 100), key_lbl, fill=PALETTE["BoneMuted"], font=f_small)

    # --- Square Inventory Slots Demo (Middle-Bottom) ---
    inv_icons = ["T_UI_Icon_Weapon_Longsword", "T_UI_Icon_Weapon_DualBlades", "T_UI_Icon_Weapon_Flintlock", "T_UI_Icon_Weapon_Scythe"]
    sq_mask = tint_rgba(load_tex("T_UI_Slot_Square_FillMask"), PALETTE["Charcoal"], 0.82).resize((96, 96), Image.Resampling.LANCZOS)
    sq_norm = tint_rgba(load_tex("T_UI_Slot_Square_Normal"), PALETTE["BoneMuted"], 0.9).resize((96, 96), Image.Resampling.LANCZOS)
    sq_sel = tint_rgba(load_tex("T_UI_Slot_Square_Selected"), PALETTE["BloodBright"], 1.0).resize((96, 96), Image.Resampling.LANCZOS)
    for k, iname in enumerate(inv_icons):
        sx = 84 + k * 108
        sy = 925
        canvas.alpha_composite(sq_mask, (sx, sy))
        canvas.alpha_composite(sq_norm, (sx, sy))
        if k == 1:
            canvas.alpha_composite(tint_rgba(load_tex("T_UI_Slot_Square_FillMask"), PALETTE["Blood"], 0.25).resize((96, 96), Image.Resampling.LANCZOS), (sx, sy))
            canvas.alpha_composite(sq_sel, (sx, sy))
        ic = tint_rgba(load_tex(iname), PALETTE["Bone"]).resize((68, 68), Image.Resampling.LANCZOS)
        canvas.alpha_composite(ic, (sx + 14, sy + 14))

    # --- Boss Bar M02 Composition (Bottom-Right) ---
    bx, by = 780, 748
    boss_w, boss_h = 980, 44
    crest = tint_rgba(load_tex("T_UI_Orn_Crest_Boss"), PALETTE["Bone"], 0.95).resize((640, 90), Image.Resampling.LANCZOS)
    canvas.alpha_composite(crest, (bx + (boss_w - 640) // 2, by - 116))
    bname = "E L   P A L A D Í N   C A Í D O"
    bb = draw.textbbox((0, 0), bname, font=f_boss)
    draw.text((bx + (boss_w - (bb[2] - bb[0])) // 2, by - 36), bname, fill=PALETTE["Bone"], font=f_boss)

    bb_bg = tint_rgba(load_tex("T_UI_Bar_Boss_FillMask"), PALETTE["Charcoal"], 0.9).resize((boss_w, boss_h), Image.Resampling.LANCZOS)
    bb_tr = tint_rgba(load_tex("T_UI_Bar_Boss_FillMask"), PALETTE["BloodDried"], 1.0).resize((boss_w, boss_h), Image.Resampling.LANCZOS).crop((0, 0, int(boss_w * 0.72), boss_h))
    bb_fl = tint_rgba(load_tex("T_UI_Bar_Boss_FillMask"), PALETTE["Blood"], 1.0).resize((boss_w, boss_h), Image.Resampling.LANCZOS).crop((0, 0, int(boss_w * 0.58), boss_h))
    bb_fr = nine_slice_render(load_tex("T_UI_Bar_Boss_Frame"), boss_w, boss_h, by_name["T_UI_Bar_Boss_Frame"]["margin"], "Border", PALETTE["Bone"], 0.95)
    canvas.alpha_composite(bb_bg, (bx, by))
    canvas.alpha_composite(bb_tr, (bx, by))
    canvas.alpha_composite(bb_fl, (bx, by))
    canvas.alpha_composite(bb_fr, (bx, by))
    # Boss Finials + Phase markers
    bf_l = tint_rgba(load_tex("T_UI_Bar_Finial_Boss_L"), PALETTE["Bone"]).resize((88, 44), Image.Resampling.LANCZOS)
    bf_r = tint_rgba(load_tex("T_UI_Bar_Finial_Boss_R"), PALETTE["Bone"]).resize((88, 44), Image.Resampling.LANCZOS)
    canvas.alpha_composite(bf_l, (bx - 66, by))
    canvas.alpha_composite(bf_r, (bx + boss_w - 22, by))
    pm = tint_rgba(load_tex("T_UI_Bar_PhaseMarker"), PALETTE["Bone"]).resize((28, 28), Image.Resampling.LANCZOS)
    for frac in (0.33, 0.66):
        canvas.alpha_composite(pm, (bx + int(boss_w * frac) - 14, by + (boss_h - 28) // 2))

    # --- Death Banner Blood Stroke + Slider + Blood Currency Demo ---
    stroke = tint_rgba(load_tex("T_UI_Mask_BloodStroke_Long_B"), PALETTE["Blood"], 0.88).resize((620, 58), Image.Resampling.LANCZOS)
    canvas.alpha_composite(stroke, (780, 868))
    draw.text((925, 876), "H A S   C A Í D O", fill=PALETTE["Bone"], font=f_h1)

    # Slider demo
    sl_tr = nine_slice_render(load_tex("T_UI_Slider_Track"), 280, 16, by_name["T_UI_Slider_Track"]["margin"], "Box", PALETTE["BoneMuted"], 0.9)
    sl_th = tint_rgba(load_tex("T_UI_Slider_Thumb"), PALETTE["BloodBright"]).resize((28, 28), Image.Resampling.LANCZOS)
    draw.text((1460, 864), "VOLUMEN  80", fill=PALETTE["BoneMuted"], font=f_small)
    canvas.alpha_composite(sl_tr, (1460, 892))
    canvas.alpha_composite(sl_th, (1460 + int(280 * 0.8) - 14, 892 - 6))

    # Blood currency bottom-right
    ic_blood = tint_rgba(load_tex("T_UI_Icon_Stat_Blood"), PALETTE["BloodBright"]).resize((36, 36), Image.Resampling.LANCZOS)
    canvas.alpha_composite(ic_blood, (1590, 968))
    draw.text((1636, 972), "1 . 2 5 0", fill=PALETTE["Bone"], font=f_boss)

    save_preview(canvas.convert("RGB"), "preview_04_live_composition.png")


def main() -> None:
    manifest = json.loads((PROC_DIR / "manifest.json").read_text(encoding="utf-8"))
    f_cinzel, f_cinzel_bold, f_forum = extract_fonts()
    f_title = load_font(f_cinzel_bold, 22)
    f_sub = load_font(f_cinzel, 14)
    f_lbl = load_font(f_cinzel_bold, 11)
    f_meta = load_font(f_forum, 13)

    build_sheet_01(manifest, f_title, f_sub, f_lbl, f_meta)
    build_sheet_02(manifest, f_title, f_sub, f_lbl, f_meta)
    build_sheet_03(manifest, f_title, f_sub, f_lbl, f_meta)
    build_sheet_04_live_composition(manifest, f_cinzel, f_cinzel_bold, f_forum)
    print("SUCCESS: Generated all 4 visual preview sheets in", PREVIEW_DIR, "and", ARTIFACT_DIR)


if __name__ == "__main__":
    main()
