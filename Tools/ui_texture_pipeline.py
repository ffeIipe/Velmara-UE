"""
Velmara UI Texture Pipeline — Phase 1
Extracts & cleans raw assets (Asset_01..Asset_08) and procedurally generates
pixel-exact, 4x-supersampled, symmetric 9-slice/3-slice UI frames, slots,
bars, masks, ornaments, and smooth gradients for Unreal Engine 5.5 Common UI.
"""

from __future__ import annotations

import json
import math
import pathlib
from dataclasses import dataclass, asdict
from typing import Dict, List, Optional, Tuple

import numpy as np
from PIL import Image, ImageDraw

ROOT = pathlib.Path(r"i:\UnrealProjects\TesisUE")
RAW_DIR = ROOT / "UI_Raw"
OUT_DIR = ROOT / "UI_Processed"


@dataclass
class AssetEntry:
    name: str
    rel_path: str
    category: str
    source: str
    width: int
    height: int
    draw_as: str  # "Image", "Border", "Box"
    margin: Optional[List[float]] = None  # [Left, Top, Right, Bottom] normalized 0-1
    srgb: bool = True
    notes: str = ""


MANIFEST: List[AssetEntry] = []


def ensure_dirs() -> None:
    for sub in ("Frames", "Bars", "Gradients", "Ornaments", "Icons", "Masks", "Backgrounds"):
        (OUT_DIR / sub).mkdir(parents=True, exist_ok=True)


def save_white_alpha(
    alpha_u8: np.ndarray,
    sub: str,
    name: str,
    source: str,
    draw_as: str = "Image",
    margin: Optional[List[float]] = None,
    notes: str = "",
) -> pathlib.Path:
    h, w = alpha_u8.shape
    rgba = np.empty((h, w, 4), dtype=np.uint8)
    rgba[..., 0] = 255
    rgba[..., 1] = 255
    rgba[..., 2] = 255
    rgba[..., 3] = alpha_u8
    dst = OUT_DIR / sub / f"{name}.png"
    Image.fromarray(rgba, mode="RGBA").save(dst, optimize=True)
    MANIFEST.append(
        AssetEntry(
            name=name,
            rel_path=f"{sub}/{name}.png",
            category=sub,
            source=source,
            width=w,
            height=h,
            draw_as=draw_as,
            margin=[round(m, 5) for m in margin] if margin else None,
            notes=notes,
        )
    )
    return dst


def round_up_mult4(n: int) -> int:
    return ((n + 3) // 4) * 4


def clean_luminance_to_alpha(
    img_rgb: np.ndarray,
    black_floor: float = 12.0,
    white_ceil: float = 240.0,
) -> np.ndarray:
    gray = img_rgb.max(axis=2).astype(np.float32)
    norm = np.clip((gray - black_floor) / (white_ceil - black_floor), 0.0, 1.0)
    return np.round(norm * 255.0).astype(np.uint8)


def tight_bbox(alpha: np.ndarray, thresh: int = 3) -> Tuple[int, int, int, int]:
    ys, xs = np.where(alpha > thresh)
    if len(ys) == 0:
        return (0, 0, alpha.shape[1], alpha.shape[0])
    return (int(xs.min()), int(ys.min()), int(xs.max()) + 1, int(ys.max()) + 1)


def fit_centered_alpha(
    alpha: np.ndarray,
    target_w: int,
    target_h: int,
    safe_ratio: float = 0.82,
    pad_px: Optional[int] = None,
) -> np.ndarray:
    x0, y0, x1, y1 = tight_bbox(alpha, thresh=3)
    crop = alpha[y0:y1, x0:x1]
    cw, ch = crop.shape[1], crop.shape[0]
    if pad_px is not None:
        max_w = max(4, target_w - 2 * pad_px)
        max_h = max(4, target_h - 2 * pad_px)
    else:
        max_w = max(4, int(round(target_w * safe_ratio)))
        max_h = max(4, int(round(target_h * safe_ratio)))
    scale = min(max_w / cw, max_h / ch)
    nw = max(1, int(round(cw * scale)))
    nh = max(1, int(round(ch * scale)))
    resized = np.array(
        Image.fromarray(crop, mode="L").resize((nw, nh), Image.Resampling.LANCZOS),
        dtype=np.uint8,
    )
    canvas = np.zeros((target_h, target_w), dtype=np.uint8)
    ox = (target_w - nw) // 2
    oy = (target_h - nh) // 2
    canvas[oy : oy + nh, ox : ox + nw] = resized
    return canvas


def extract_region(
    alpha_full: np.ndarray,
    region: Tuple[int, int, int, int],
) -> np.ndarray:
    rx0, ry0, rx1, ry1 = region
    sub = alpha_full[ry0:ry1, rx0:rx1]
    x0, y0, x1, y1 = tight_bbox(sub, thresh=3)
    return sub[y0:y1, x0:x1]


def pad_tight_mult4(alpha: np.ndarray, pad: int = 8) -> np.ndarray:
    h, w = alpha.shape
    tw = round_up_mult4(w + 2 * pad)
    th = round_up_mult4(h + 2 * pad)
    canvas = np.zeros((th, tw), dtype=np.uint8)
    ox = (tw - w) // 2
    oy = (th - h) // 2
    canvas[oy : oy + h, ox : ox + w] = alpha
    return canvas


def downsample_ss(canvas_ss: np.ndarray, ss: int) -> np.ndarray:
    h, w = canvas_ss.shape
    assert h % ss == 0 and w % ss == 0
    arr = canvas_ss.reshape(h // ss, ss, w // ss, ss).mean(axis=(1, 3))
    return np.clip(np.round(arr), 0, 255).astype(np.uint8)


def make_ss_canvas(w: int, h: int, ss: int = 4) -> Tuple[Image.Image, ImageDraw.ImageDraw]:
    img = Image.new("L", (w * ss, h * ss), 0)
    return img, ImageDraw.Draw(img)


# =============================================================================
# PART 1: EXTRACT & CLEAN RAW ASSETS (Asset_01 .. Asset_08)
# =============================================================================


def load_raw_alpha(filename: str, black: float = 12.0, white: float = 240.0) -> np.ndarray:
    rgb = np.array(Image.open(RAW_DIR / filename).convert("RGB"))
    return clean_luminance_to_alpha(rgb, black_floor=black, white_ceil=white)


def extract_asset_01() -> None:
    src = "Asset_01_BarFinials_Corners.png"
    a = load_raw_alpha(src, black=10.0, white=238.0)

    # Simple bar finials: mirror _L cleanly to guarantee exact symmetric alignment on bar ends
    fin_simple_l = extract_region(a, (0, 150, 260, 360))
    fin_simple_l_canvas = fit_centered_alpha(fin_simple_l, 144, 96, pad_px=8)
    fin_simple_r_canvas = np.fliplr(fin_simple_l_canvas)
    save_white_alpha(
        fin_simple_l_canvas, "Bars", "T_UI_Bar_Finial_Simple_L", src,
        notes="Left end-cap finial for player health/shield bars"
    )
    save_white_alpha(
        fin_simple_r_canvas, "Bars", "T_UI_Bar_Finial_Simple_R", src,
        notes="Right end-cap finial (exact mirror of L)"
    )

    # Boss bar finials: mirror _L cleanly for exact symmetry
    fin_boss_l = extract_region(a, (500, 150, 760, 360))
    fin_boss_l_canvas = fit_centered_alpha(fin_boss_l, 192, 96, pad_px=8)
    fin_boss_r_canvas = np.fliplr(fin_boss_l_canvas)
    save_white_alpha(
        fin_boss_l_canvas, "Bars", "T_UI_Bar_Finial_Boss_L", src,
        notes="Left ornate double-chevron + star finial for Boss bar"
    )
    save_white_alpha(
        fin_boss_r_canvas, "Bars", "T_UI_Bar_Finial_Boss_R", src,
        notes="Right ornate Boss bar finial (exact mirror of L)"
    )

    # Corner fading L-bracket with diamond tip (bottom-left)
    corner_bl = extract_region(a, (80, 560, 460, 880))
    corner_bl_canvas = fit_centered_alpha(corner_bl, 256, 256, pad_px=16)
    save_white_alpha(
        corner_bl_canvas, "Ornaments", "T_UI_Orn_CornerFade_BL", src,
        notes="Fading L-bracket corner ornament with corner diamond (flip in UMG for other corners)"
    )

    # Hollow diamond & 4-point Gothic star
    diam = extract_region(a, (620, 680, 770, 840))
    save_white_alpha(
        fit_centered_alpha(diam, 64, 64, safe_ratio=0.80), "Ornaments", "T_UI_Orn_Diamond_Raw", src,
        notes="Hollow diamond ornament from Asset_01"
    )
    star4 = extract_region(a, (770, 660, 920, 860))
    save_white_alpha(
        fit_centered_alpha(star4, 96, 128, safe_ratio=0.84), "Ornaments", "T_UI_Orn_Star4Pt_Raw", src,
        notes="4-point vertical gothic star from Asset_01"
    )


def extract_asset_02() -> None:
    src = "Asset_02_Crests_Separators.png"
    a = load_raw_alpha(src, black=10.0, white=240.0)

    # [0] Boss Crest (enforce clean centered canvas, 1024x144)
    crest = extract_region(a, (0, 0, 1792, 240))
    # Symmetrize subtly around center column for pixel-perfect boss header centering
    cw = crest.shape[1]
    half = cw // 2
    left_h = crest[:, :half]
    sym_crest = np.zeros_like(crest)
    sym_crest[:, :half] = left_h
    sym_crest[:, cw - half :] = np.fliplr(left_h)
    if cw % 2 == 1:
        sym_crest[:, half] = crest[:, half]
    save_white_alpha(
        fit_centered_alpha(sym_crest, 1024, 144, pad_px=8),
        "Ornaments", "T_UI_Orn_Crest_Boss", src,
        notes="Symmetric winged Gothic boss crest for above Boss Health Bar"
    )

    # [1] Velmara Crescent + Vertical Needle Emblem
    emblem = extract_region(a, (750, 240, 1050, 640))
    save_white_alpha(
        fit_centered_alpha(emblem, 128, 384, pad_px=12),
        "Ornaments", "T_UI_Orn_Crest_Velmara", src,
        notes="Crescent moon + vertical needle emblem (Settings top-right & title accent)"
    )

    # [2] Horizontal Separator with central 4-point star
    sep_star = extract_region(a, (0, 640, 1792, 830))
    # Center exactly on brightest central peak column
    col_sums = sep_star.sum(axis=0)
    peak_x = int(np.argmax(col_sums))
    half_span = min(peak_x, sep_star.shape[1] - 1 - peak_x)
    sep_star_sym = sep_star[:, peak_x - half_span : peak_x + half_span + 1]
    save_white_alpha(
        fit_centered_alpha(sep_star_sym, 1024, 112, pad_px=8),
        "Ornaments", "T_UI_Orn_Sep_Star", src,
        notes="Wide horizontal separator with centered 4-point gothic star"
    )

    # [3] Horizontal Separator with central hollow diamond + 2 dots
    sep_diam = extract_region(a, (0, 840, 1792, 980))
    save_white_alpha(
        fit_centered_alpha(sep_diam, 1024, 48, pad_px=8),
        "Ornaments", "T_UI_Orn_Sep_Diamond", src,
        notes="Horizontal separator with centered hollow diamond & flanking dots"
    )


def extract_asset_03() -> None:
    src = "Asset_03_Reticles_Spinner.png"
    a = load_raw_alpha(src, black=12.0, white=240.0)
    specs = [
        ((0, 0, 512, 512), "T_UI_Reticle_LockOn", 256, "Lock-on target reticle (circle + 4 needle spikes)"),
        ((512, 0, 1024, 512), "T_UI_Reticle_Possession", 256, "Possession target indicator ring with 4 cardinal diamonds"),
        ((0, 512, 512, 1024), "T_UI_Spinner_Moon", 256, "Loading screen circular moon sigil spinner"),
        ((512, 512, 1024, 1024), "T_UI_Orn_Node_Diamond", 256, "Double-outline diamond node/prompt frame with corner dots"),
    ]
    for box, name, size, note in specs:
        crop = extract_region(a, box)
        save_white_alpha(
            fit_centered_alpha(crop, size, size, safe_ratio=0.88),
            "Ornaments", name, src, notes=note
        )


def extract_icon_grid_3x2(
    filename: str,
    names_notes: List[Tuple[str, str]],
    out_size: int = 128,
    safe_ratio: float = 0.80,
) -> None:
    a = load_raw_alpha(filename, black=12.0, white=240.0)
    h, w = a.shape
    cw, ch = w // 3, h // 2
    idx = 0
    for r in range(2):
        for c in range(3):
            name, note = names_notes[idx]
            idx += 1
            crop = extract_region(a, (c * cw, r * ch, (c + 1) * cw, (r + 1) * ch))
            save_white_alpha(
                fit_centered_alpha(crop, out_size, out_size, safe_ratio=safe_ratio),
                "Icons", name, filename, notes=note
            )


def extract_assets_04_05_06() -> None:
    extract_icon_grid_3x2(
        "Asset_04_Icons_Combat_Stats.png",
        [
            ("T_UI_Icon_Stat_Blood", "Health / Blood currency (drop + crescent)"),
            ("T_UI_Icon_Stat_Shield", "Shield / Defense (gothic heater shield)"),
            ("T_UI_Icon_Stat_Spectral", "Spectral energy (wisp flame + star)"),
            ("T_UI_Icon_Stat_Damage", "Base damage (downward gothic sword)"),
            ("T_UI_Icon_Stat_Speed", "Movement speed (twin bat wings)"),
            ("T_UI_Icon_Stat_AttackSpeed", "Attack speed (double diagonal slash)"),
        ],
        out_size=128,
        safe_ratio=0.82,
    )
    extract_icon_grid_3x2(
        "Asset_05_Icons_Weapons.png",
        [
            ("T_UI_Icon_Weapon_Longsword", "Longsword weapon icon"),
            ("T_UI_Icon_Weapon_DualBlades", "Crossed dual blades weapon icon"),
            ("T_UI_Icon_Weapon_Flintlock", "Flintlock pistol weapon icon"),
            ("T_UI_Icon_Weapon_Spear", "Spear / lance weapon icon"),
            ("T_UI_Icon_Weapon_Scythe", "Gothic scythe weapon icon"),
            ("T_UI_Icon_Item_Vial", "Blood vial consumable icon"),
        ],
        out_size=256,
        safe_ratio=0.84,
    )
    extract_icon_grid_3x2(
        "Asset_06_Icons_Menu_Status.png",
        [
            ("T_UI_Icon_Menu_Altar", "Blood altar (chalice + floating diamond)"),
            ("T_UI_Icon_Menu_Skills", "Attribute / skill tree (thorn branches)"),
            ("T_UI_Icon_Status_Sunlight", "Sunlight exposure warning (8-ray sun)"),
            ("T_UI_Icon_Status_Totem", "Carved obelisk / totem checkpoint"),
            ("T_UI_Icon_Menu_Lore", "Quill feather (save / journal / objective)"),
            ("T_UI_Icon_Status_Death", "Gothic skull (death / danger)"),
        ],
        out_size=128,
        safe_ratio=0.82,
    )


def extract_asset_07() -> None:
    for suffix, fname in (("A", "Asset_07_Blood_StrokesB1.png"), ("B", "Asset_07_Blood_StrokesB2.png")):
        a = load_raw_alpha(fname, black=12.0, white=245.0)
        h, w = a.shape
        top = extract_region(a, (0, 0, w, h // 2))
        bot = extract_region(a, (0, h // 2, w, h))
        save_white_alpha(
            fit_centered_alpha(top, 1024, 96, pad_px=8),
            "Masks", f"T_UI_Mask_BloodStroke_Long_{suffix}", fname,
            notes=f"Wide organic dry-brush blood stroke mask (variant {suffix}) for Death / Title banners"
        )
        save_white_alpha(
            fit_centered_alpha(bot, 768, 64, pad_px=8),
            "Masks", f"T_UI_Mask_BloodStroke_Short_{suffix}", fname,
            notes=f"Compact dry-brush stroke with right splatter dots (variant {suffix})"
        )


def extract_asset_08() -> None:
    src = "Asset_08_Bg_MainMenu.png"
    im = Image.open(RAW_DIR / src).convert("RGB")
    arr = np.array(im)
    # Remove top and bottom pure-black letterbox bars cleanly (rows 24..791 -> 768px tall)
    row_max = arr.max(axis=(1, 2))
    valid = np.where(row_max > 10)[0]
    y0, y1 = int(valid[0]) + 1, int(valid[-1])  # trim 1px transition row too
    # Make exact 16:9 aspect (trim purely empty dark-left edge so cathedral + hunter stay intact)
    crop_h = y1 - y0
    target_w_16_9 = round_up_mult4(int(round(crop_h * 16.0 / 9.0)))
    crop_h_m4 = round_up_mult4(crop_h)
    y1 = y0 + crop_h_m4
    x0 = max(0, arr.shape[1] - target_w_16_9)
    cropped = im.crop((x0, y0, x0 + target_w_16_9, y1))
    dst = OUT_DIR / "Backgrounds" / "T_UI_Bg_MainMenu.png"
    cropped.save(dst, optimize=True)
    MANIFEST.append(
        AssetEntry(
            name="T_UI_Bg_MainMenu",
            rel_path="Backgrounds/T_UI_Bg_MainMenu.png",
            category="Backgrounds",
            source=src,
            width=cropped.width,
            height=cropped.height,
            draw_as="Image",
            notes="Full-color Main Menu key art, black letterbox bars removed & cropped to exact 16:9",
        )
    )


# =============================================================================
# PART 2: PROCEDURAL PIXEL-EXACT 4x-SUPERSAMPLED UI ASSETS
# =============================================================================


def draw_tapered_chevron_polygon(
    draw: ImageDraw.ImageDraw,
    tip_x: float,
    cy: float,
    dx: float,
    half_h: float,
    thickness: float,
    direction: int,  # -1 = '<' (tip on left), +1 = '>' (tip on right)
    ss: int = 4,
    fill: int = 255,
) -> None:
    """
    Draws the sharp tapered Gothic chevron seen in Mockup_Ajustes.png:
    slightly wider at the vertex, tapering smoothly to razor-sharp tips.
    """
    s = ss
    # direction = -1 means tip is at tip_x, arms extend right (+dx)
    arm_x = tip_x - direction * dx
    inner_x = tip_x - direction * thickness
    pts = [
        (tip_x * s, cy * s),
        (arm_x * s, (cy - half_h) * s),
        (inner_x * s, cy * s),
        (arm_x * s, (cy + half_h) * s),
    ]
    draw.polygon(pts, fill=fill)


def draw_gothic_star_4pt(
    draw: ImageDraw.ImageDraw,
    cx: float,
    cy: float,
    rx: float,
    ry_top: float,
    ry_bot: float,
    waist: float,
    ss: int = 4,
    fill: int = 255,
) -> None:
    """Draws a crisp concave 4-point Gothic star / drop-star."""
    s = ss
    pts = [
        (cx * s, (cy - ry_top) * s),
        ((cx + waist) * s, (cy - waist) * s),
        ((cx + rx) * s, cy * s),
        ((cx + waist) * s, (cy + waist) * s),
        (cx * s, (cy + ry_bot) * s),
        ((cx - waist) * s, (cy + waist) * s),
        ((cx - rx) * s, cy * s),
        ((cx - waist) * s, (cy - waist) * s),
    ]
    draw.polygon(pts, fill=fill)


def draw_diamond(
    draw: ImageDraw.ImageDraw,
    cx: float,
    cy: float,
    rx: float,
    ry: float,
    ss: int = 4,
    fill: int = 255,
) -> None:
    s = ss
    pts = [
        (cx * s, (cy - ry) * s),
        ((cx + rx) * s, cy * s),
        (cx * s, (cy + ry) * s),
        ((cx - rx) * s, cy * s),
    ]
    draw.polygon(pts, fill=fill)


def draw_rect_outline_exact(
    arr: np.ndarray,
    x0: int,
    y0: int,
    x1: int,
    y1: int,
    thick: int,
    ss: int = 4,
    val: int = 255,
) -> None:
    """Pixel-exact axis-aligned rectangle outline at ss scale (zero half-pixel blur)."""
    s = ss
    t = thick * s
    arr[y0 * s : y0 * s + t, x0 * s : x1 * s] = np.maximum(arr[y0 * s : y0 * s + t, x0 * s : x1 * s], val)
    arr[y1 * s - t : y1 * s, x0 * s : x1 * s] = np.maximum(arr[y1 * s - t : y1 * s, x0 * s : x1 * s], val)
    arr[y0 * s : y1 * s, x0 * s : x0 * s + t] = np.maximum(arr[y0 * s : y1 * s, x0 * s : x0 * s + t], val)
    arr[y0 * s : y1 * s, x1 * s - t : x1 * s] = np.maximum(arr[y0 * s : y1 * s, x1 * s - t : x1 * s], val)


def generate_frames() -> None:
    ss = 4

    # -------------------------------------------------------------------------
    # 1. T_UI_Frame_OptionRow (512x128, 3-slice/9-slice safe: outer hairline + < >)
    # -------------------------------------------------------------------------
    W, H = 512, 128
    img, draw = make_ss_canvas(W, H, ss)
    # Draw left tapered '<' chevron then mirror array cleanly for bit-exact L-R & T-B symmetry
    draw_tapered_chevron_polygon(draw, tip_x=16.0, cy=64.0, dx=26.0, half_h=38.0, thickness=5.2, direction=-1, ss=ss)
    arr = np.array(img, dtype=np.uint8)
    hw, hh = (W // 2) * ss, (H // 2) * ss
    arr[hh:, :hw] = np.flipud(arr[:hh, :hw])
    arr[:, hw:] = np.fliplr(arr[:, :hw])
    # Outer crisp 2px hairline rectangle (y=4..124, x=4..508)
    draw_rect_outline_exact(arr, 4, 4, W - 4, H - 4, thick=2, ss=ss, val=220)
    out = downsample_ss(arr, ss)
    assert np.array_equal(out, np.fliplr(out)), "OptionRow not L-R symmetric"
    assert np.array_equal(out, np.flipud(out)), "OptionRow not T-B symmetric"
    # Chevrons end at x=42, safe margin at x=52 -> 52/512 = 0.10156
    save_white_alpha(
        out, "Frames", "T_UI_Frame_OptionRow", "Procedural (Mockup_Ajustes)",
        draw_as="Border", margin=[52 / W, 16 / H, 52 / W, 16 / H],
        notes="3-slice/9-slice safe Settings option row frame with razor-sharp < > chevrons"
    )

    # -------------------------------------------------------------------------
    # 2. T_UI_Frame_OptionRow_WithStar (1024x144, full row + centered bottom star)
    # -------------------------------------------------------------------------
    W2, H2 = 1024, 144
    img, draw = make_ss_canvas(W2, H2, ss)
    cy = 68.0
    draw_tapered_chevron_polygon(draw, tip_x=18.0, cy=cy, dx=26.0, half_h=38.0, thickness=5.2, direction=-1, ss=ss)
    # Crisp bottom vertical diamond/drop star on bottom hairline (y=128)
    draw_gothic_star_4pt(draw, cx=W2 / 2.0, cy=127.0, rx=5.5, ry_top=14.0, ry_bot=11.0, waist=2.6, ss=ss, fill=255)
    arr = np.array(img, dtype=np.uint8)
    hw2 = (W2 // 2) * ss
    arr[:, hw2:] = np.fliplr(arr[:, :hw2])
    draw_rect_outline_exact(arr, 4, 8, W2 - 4, 128, thick=2, ss=ss, val=220)
    out = downsample_ss(arr, ss)
    assert np.array_equal(out, np.fliplr(out)), "OptionRow_WithStar not L-R symmetric"
    save_white_alpha(
        out, "Frames", "T_UI_Frame_OptionRow_WithStar", "Procedural (Mockup_Ajustes)",
        draw_as="Image",
        notes="Fixed-aspect complete option row frame with < > chevrons and centered bottom star"
    )

    # -------------------------------------------------------------------------
    # 3. T_UI_Frame_Brackets_Panel (512x512, Mockup_Ajustes right Description Panel)
    #    Bold 100% white L-brackets at 4 corners + delicate 60% hairline with clean gap
    # -------------------------------------------------------------------------
    S = 512
    arr = np.zeros((S * ss, S * ss), dtype=np.uint8)
    pad = 8
    arm = 52
    b_thick = 6
    gap = 16
    line_alpha = 155
    # Build top-left quadrant then mirror cleanly for mathematical 4-way symmetry
    half_s = (S // 2) * ss
    # Bold L-bracket top-left
    arr[pad * ss : (pad + b_thick) * ss, pad * ss : (pad + arm) * ss] = 255
    arr[pad * ss : (pad + arm) * ss, pad * ss : (pad + b_thick) * ss] = 255
    # Delicate hairline (2px thick, centered with bracket inner/mid alignment)
    line_start = pad + arm + gap
    line_offset = pad + 2  # sits cleanly aligned with bracket edge
    arr[line_offset * ss : (line_offset + 2) * ss, line_start * ss : half_s] = line_alpha
    arr[line_start * ss : half_s, line_offset * ss : (line_offset + 2) * ss] = line_alpha
    # Mirror H and V
    arr[:half_s, half_s:] = np.fliplr(arr[:half_s, :half_s])
    arr[half_s:, :] = np.flipud(arr[:half_s, :])
    out = downsample_ss(arr, ss)
    assert np.array_equal(out, np.fliplr(out)) and np.array_equal(out, np.flipud(out))
    m_px = line_start + 4  # 80px -> 0.15625
    save_white_alpha(
        out, "Frames", "T_UI_Frame_Brackets_Panel", "Procedural (Mockup_Ajustes)",
        draw_as="Border", margin=[m_px / S] * 4,
        notes="Exact Mockup_Ajustes right description panel: bold L-brackets + subtle hairline with gap"
    )

    # -------------------------------------------------------------------------
    # 4. T_UI_Frame_Brackets_Minimal (512x512, PromptKit F01: pure L-brackets + corner diamonds)
    # -------------------------------------------------------------------------
    S = 512
    img, draw = make_ss_canvas(S, S, ss)
    pad = 16
    arm = 64
    b_thick = 3
    # Top-left L-bracket + corner diamond
    draw.rectangle([pad * ss, pad * ss, (pad + arm) * ss - 1, (pad + b_thick) * ss - 1], fill=255)
    draw.rectangle([pad * ss, pad * ss, (pad + b_thick) * ss - 1, (pad + arm) * ss - 1], fill=255)
    draw_diamond(draw, cx=pad + b_thick / 2.0, cy=pad + b_thick / 2.0, rx=6.5, ry=6.5, ss=ss, fill=255)
    arr = np.array(img, dtype=np.uint8)
    half_s = (S // 2) * ss
    arr[:half_s, half_s:] = np.fliplr(arr[:half_s, :half_s])
    arr[half_s:, :] = np.flipud(arr[:half_s, :])
    out = downsample_ss(arr, ss)
    assert np.array_equal(out, np.fliplr(out)) and np.array_equal(out, np.flipud(out))
    m_px = pad + arm + 6  # 86px -> 0.16797
    save_white_alpha(
        out, "Frames", "T_UI_Frame_Brackets_Minimal", "Procedural (PromptKit F01)",
        draw_as="Border", margin=[m_px / S] * 4,
        notes="Pure corner L-brackets with tiny corner diamonds (modals, tooltips, PBZ panels)"
    )

    # -------------------------------------------------------------------------
    # 5. T_UI_Frame_Panel_Gothic (512x512, PromptKit F02: thin hairline + notched gothic corners)
    # -------------------------------------------------------------------------
    S = 512
    img, draw = make_ss_canvas(S, S, ss)
    pad = 14
    chamfer = 14
    # Draw crisp diagonal chamfer corner + corner thorn diamond in top-left quadrant
    # Straight lines start at pad + chamfer
    pts_outer = [
        (pad * ss, (pad + chamfer) * ss),
        ((pad + chamfer) * ss, pad * ss),
        ((pad + chamfer) * ss, (pad + 2.5) * ss),
        ((pad + 2.5) * ss, (pad + chamfer) * ss),
    ]
    draw.polygon(pts_outer, fill=255)
    # Tiny thorn diamond nestled at corner point
    draw_diamond(draw, cx=pad + 3.0, cy=pad + 3.0, rx=4.5, ry=4.5, ss=ss, fill=255)
    # Small inner L-accent
    in_p = pad + 12
    draw.rectangle([in_p * ss, in_p * ss, (in_p + 20) * ss - 1, (in_p + 2) * ss - 1], fill=180)
    draw.rectangle([in_p * ss, in_p * ss, (in_p + 2) * ss - 1, (in_p + 20) * ss - 1], fill=180)
    arr = np.array(img, dtype=np.uint8)
    half_s = (S // 2) * ss
    # Straight hairline segments
    arr[pad * ss : (pad + 2) * ss, (pad + chamfer) * ss : half_s] = 255
    arr[(pad + chamfer) * ss : half_s, pad * ss : (pad + 2) * ss] = 255
    arr[:half_s, half_s:] = np.fliplr(arr[:half_s, :half_s])
    arr[half_s:, :] = np.flipud(arr[:half_s, :])
    out = downsample_ss(arr, ss)
    assert np.array_equal(out, np.fliplr(out)) and np.array_equal(out, np.flipud(out))
    m_px = in_p + 24  # 50px -> 0.09766
    save_white_alpha(
        out, "Frames", "T_UI_Frame_Panel_Gothic", "Procedural (PromptKit F02)",
        draw_as="Border", margin=[m_px / S] * 4,
        notes="Full hairline gothic panel with chamfered corners, corner diamonds and inner ticks"
    )

    # -------------------------------------------------------------------------
    # 6. T_UI_Frame_Sidebar_Active (512x128, Mockup_Ajustes left sidebar 'JUEGO' active border)
    #    Bold left bar + fading top & bottom hairlines going right
    # -------------------------------------------------------------------------
    W, H = 512, 128
    arr = np.zeros((H, W), dtype=np.float32)
    pad_x, pad_y = 4, 4
    # Bold 4px left vertical bar
    arr[pad_y : H - pad_y, pad_x : pad_x + 4] = 255.0
    # Smooth horizontal fade profile for top & bottom 2px hairlines
    xs = np.arange(W, dtype=np.float32)
    t = np.clip((xs - 64.0) / (W - 76.0), 0.0, 1.0)
    fade = 1.0 - (3 * t**2 - 2 * t**3)  # smoothstep 1 -> 0
    fade[:pad_x] = 0.0
    arr[pad_y : pad_y + 2, pad_x:] = np.maximum(arr[pad_y : pad_y + 2, pad_x:], 255.0 * fade[pad_x:])
    arr[H - pad_y - 2 : H - pad_y, pad_x:] = np.maximum(
        arr[H - pad_y - 2 : H - pad_y, pad_x:], 255.0 * fade[pad_x:]
    )
    out = np.clip(np.round(arr), 0, 255).astype(np.uint8)
    assert np.array_equal(out, np.flipud(out))
    save_white_alpha(
        out, "Frames", "T_UI_Frame_Sidebar_Active", "Procedural (Mockup_Ajustes)",
        draw_as="Image",
        notes="Left-anchored active category border (bold left bar + fading top/bottom hairlines; tint BloodBright)"
    )

    # -------------------------------------------------------------------------
    # 7. T_UI_Btn_Hex_3Slice (512x128, PromptKit F05: blade-pointed hexagonal button)
    # -------------------------------------------------------------------------
    W, H = 512, 128
    img, draw = make_ss_canvas(W, H, ss)
    tip_x = 24.0
    shoulder_x = 68.0
    top_y = 12.0
    bot_y = H - 12.0
    cy = H / 2.0
    # Outer polygon - inner polygon for exact crisp angled outline
    outer = [
        (tip_x * ss, cy * ss),
        (shoulder_x * ss, top_y * ss),
        ((W - shoulder_x) * ss, top_y * ss),
        ((W - tip_x) * ss, cy * ss),
        ((W - shoulder_x) * ss, bot_y * ss),
        (shoulder_x * ss, bot_y * ss),
    ]
    draw.polygon(outer, fill=255)
    # Cut interior with exact 2.2px uniform inset
    inset_y = 2.0
    # Slope offset so diagonal stroke matches horizontal thickness
    slope_len = math.hypot(shoulder_x - tip_x, cy - top_y)
    dx_in = inset_y * slope_len / (cy - top_y)
    inner = [
        ((tip_x + dx_in) * ss, cy * ss),
        ((shoulder_x + 1.0) * ss, (top_y + inset_y) * ss),
        ((W - shoulder_x - 1.0) * ss, (top_y + inset_y) * ss),
        ((W - tip_x - dx_in) * ss, cy * ss),
        ((W - shoulder_x - 1.0) * ss, (bot_y - inset_y) * ss),
        ((shoulder_x + 1.0) * ss, (bot_y - inset_y) * ss),
    ]
    draw.polygon(inner, fill=0)
    # Tiny flanking diamonds outside each tip
    draw_diamond(draw, cx=11.0, cy=cy, rx=4.5, ry=4.5, ss=ss, fill=255)
    draw_diamond(draw, cx=W - 11.0, cy=cy, rx=4.5, ry=4.5, ss=ss, fill=255)
    arr = np.array(img, dtype=np.uint8)
    # Enforce exact symmetry
    hw = (W // 2) * ss
    hh = (H // 2) * ss
    arr[:hh, hw:] = np.fliplr(arr[:hh, :hw])
    arr[hh:, :] = np.flipud(arr[:hh, :])
    out = downsample_ss(arr, ss)
    save_white_alpha(
        out, "Frames", "T_UI_Btn_Hex_3Slice", "Procedural (PromptKit F05)",
        draw_as="Border", margin=[76 / W, 20 / H, 76 / W, 20 / H],
        notes="Blade-pointed hexagonal 3-slice button frame with flanking tip diamonds"
    )


def generate_slots() -> None:
    ss = 4

    # -------------------------------------------------------------------------
    # 1. T_UI_Slot_Square_Normal (256x256, PromptKit F03)
    # -------------------------------------------------------------------------
    S = 256
    img, draw = make_ss_canvas(S, S, ss)
    pad = 16
    ch = 12  # 45-deg corner chamfer
    # Outer chamfered octagon minus inner octagon
    def chamfer_poly(p: float, c: float) -> List[Tuple[float, float]]:
        return [
            ((p + c) * ss, p * ss),
            ((S - p - c) * ss, p * ss),
            ((S - p) * ss, (p + c) * ss),
            ((S - p) * ss, (S - p - c) * ss),
            ((S - p - c) * ss, (S - p) * ss),
            ((p + c) * ss, (S - p) * ss),
            (p * ss, (S - p - c) * ss),
            (p * ss, (p + c) * ss),
        ]

    draw.polygon(chamfer_poly(pad, ch), fill=255)
    draw.polygon(chamfer_poly(pad + 2.0, ch - 0.8), fill=0)
    # Tiny diamond dot outside each chamfered corner
    draw_diamond(draw, cx=pad + 2.5, cy=pad + 2.5, rx=3.5, ry=3.5, ss=ss, fill=255)
    arr = np.array(img, dtype=np.uint8)
    hs = (S // 2) * ss
    arr[:hs, hs:] = np.fliplr(arr[:hs, :hs])
    arr[hs:, :] = np.flipud(arr[:hs, :])
    out = downsample_ss(arr, ss)
    m_px = pad + ch + 6  # 34px -> 0.1328
    save_white_alpha(
        out, "Frames", "T_UI_Slot_Square_Normal", "Procedural (PromptKit F03)",
        draw_as="Border", margin=[m_px / S] * 4,
        notes="Square inventory slot with 45-deg chamfered corners and corner diamond dots"
    )

    # -------------------------------------------------------------------------
    # 2. T_UI_Slot_Square_Selected (256x256, PromptKit F04 overlay on F03)
    # -------------------------------------------------------------------------
    img, draw = make_ss_canvas(S, S, ss)
    # Inset secondary hairline
    in_p = pad + 8
    draw.rectangle([in_p * ss, in_p * ss, (S - in_p) * ss - 1, (S - in_p) * ss - 1], fill=170)
    draw.rectangle([(in_p + 2) * ss, (in_p + 2) * ss, (S - in_p - 2) * ss - 1, (S - in_p - 2) * ss - 1], fill=0)
    # Thicker inner corner L-brackets
    bp = pad + 6
    b_arm = 32
    b_t = 4
    draw.rectangle([bp * ss, bp * ss, (bp + b_arm) * ss - 1, (bp + b_t) * ss - 1], fill=255)
    draw.rectangle([bp * ss, bp * ss, (bp + b_t) * ss - 1, (bp + b_arm) * ss - 1], fill=255)
    arr = np.array(img, dtype=np.uint8)
    arr[:hs, hs:] = np.fliplr(arr[:hs, :hs])
    arr[hs:, :] = np.flipud(arr[:hs, :])
    out = downsample_ss(arr, ss)
    m_px = bp + b_arm + 6  # 60px -> 0.2344
    save_white_alpha(
        out, "Frames", "T_UI_Slot_Square_Selected", "Procedural (PromptKit F04)",
        draw_as="Border", margin=[m_px / S] * 4,
        notes="Selected square slot overlay (inner corner L-brackets + inset hairline; tint BloodBright)"
    )

    # -------------------------------------------------------------------------
    # 3. T_UI_Slot_Square_FillMask (256x256, exact interior fill for F03)
    # -------------------------------------------------------------------------
    img, draw = make_ss_canvas(S, S, ss)
    draw.polygon(chamfer_poly(pad + 3.0, ch - 1.2), fill=255)
    arr = np.array(img, dtype=np.uint8)
    arr[:hs, hs:] = np.fliplr(arr[:hs, :hs])
    arr[hs:, :] = np.flipud(arr[:hs, :])
    out = downsample_ss(arr, ss)
    save_white_alpha(
        out, "Frames", "T_UI_Slot_Square_FillMask", "Procedural (PromptKit F03)",
        draw_as="Box", margin=[36 / S] * 4,
        notes="Interior chamfered fill mask for square slots (Charcoal backdrop / Blood selected tint)"
    )

    # -------------------------------------------------------------------------
    # 4. T_UI_Slot_Diamond_Normal (256x256, HUD skill slots M01 & interaction prompt M03)
    # -------------------------------------------------------------------------
    img, draw = make_ss_canvas(S, S, ss)
    c = S / 2.0
    r_out = 104.0
    stroke = 2.8  # ~2px perpendicular thickness at 45 deg (2 * sqrt(2) = 2.83)
    draw_diamond(draw, c, c, r_out, r_out, ss=ss, fill=255)
    draw_diamond(draw, c, c, r_out - stroke, r_out - stroke, ss=ss, fill=0)
    # Crisp cardinal outer needle ticks at the 4 tips
    draw.rectangle([(c - 1.0) * ss, (c - r_out - 12.0) * ss, (c + 1.0) * ss, (c - r_out - 4.0) * ss], fill=255)
    draw.rectangle([(c - r_out - 12.0) * ss, (c - 1.0) * ss, (c - r_out - 4.0) * ss, (c + 1.0) * ss], fill=255)
    arr = np.array(img, dtype=np.uint8)
    arr[:hs, hs:] = np.fliplr(arr[:hs, :hs])
    arr[hs:, :] = np.flipud(arr[:hs, :])
    out = downsample_ss(arr, ss)
    save_white_alpha(
        out, "Frames", "T_UI_Slot_Diamond_Normal", "Procedural (HUD M01 / M03)",
        draw_as="Image",
        notes="45-deg rotated diamond slot outline with cardinal tip ticks (HUD skills & interact prompt)"
    )

    # -------------------------------------------------------------------------
    # 5. T_UI_Slot_Diamond_Selected (256x256, active skill / selected node)
    # -------------------------------------------------------------------------
    img, draw = make_ss_canvas(S, S, ss)
    # Outer diamond
    draw_diamond(draw, c, c, r_out, r_out, ss=ss, fill=255)
    draw_diamond(draw, c, c, r_out - 3.6, r_out - 3.6, ss=ss, fill=0)
    # Inner second hairline diamond
    r_in = r_out - 11.0
    draw_diamond(draw, c, c, r_in, r_in, ss=ss, fill=190)
    draw_diamond(draw, c, c, r_in - 2.6, r_in - 2.6, ss=ss, fill=0)
    # Solid diamond caps at top & left tips (mirrored to all 4)
    draw_diamond(draw, c, c - r_out - 8.0, 5.0, 5.0, ss=ss, fill=255)
    draw_diamond(draw, c - r_out - 8.0, c, 5.0, 5.0, ss=ss, fill=255)
    arr = np.array(img, dtype=np.uint8)
    arr[:hs, hs:] = np.fliplr(arr[:hs, :hs])
    arr[hs:, :] = np.flipud(arr[:hs, :])
    out = downsample_ss(arr, ss)
    save_white_alpha(
        out, "Frames", "T_UI_Slot_Diamond_Selected", "Procedural (HUD M01 / M09)",
        draw_as="Image",
        notes="Double-outline selected diamond slot with cardinal corner diamonds (tint BloodBright)"
    )

    # -------------------------------------------------------------------------
    # 6. T_UI_Slot_Diamond_FillMask (256x256, exact interior fill/cooldown mask)
    # -------------------------------------------------------------------------
    img, draw = make_ss_canvas(S, S, ss)
    draw_diamond(draw, c, c, r_out - 4.0, r_out - 4.0, ss=ss, fill=255)
    arr = np.array(img, dtype=np.uint8)
    arr[:hs, hs:] = np.fliplr(arr[:hs, :hs])
    arr[hs:, :] = np.flipud(arr[:hs, :])
    out = downsample_ss(arr, ss)
    save_white_alpha(
        out, "Frames", "T_UI_Slot_Diamond_FillMask", "Procedural (HUD M01)",
        draw_as="Image",
        notes="Interior diamond mask for HUD skill backdrop & radial cooldown sweep"
    )

    # -------------------------------------------------------------------------
    # 7. T_UI_Pip_Diamond_Hollow & Filled (64x64, HUD M01 spectral energy pips)
    # -------------------------------------------------------------------------
    P = 64
    pc = P / 2.0
    hp = (P // 2) * ss
    # Hollow
    img, draw = make_ss_canvas(P, P, ss)
    draw_diamond(draw, pc, pc, 22.0, 22.0, ss=ss, fill=255)
    draw_diamond(draw, pc, pc, 17.5, 17.5, ss=ss, fill=0)
    arr = np.array(img, dtype=np.uint8)
    arr[:hp, hp:] = np.fliplr(arr[:hp, :hp])
    arr[hp:, :] = np.flipud(arr[:hp, :])
    save_white_alpha(
        downsample_ss(arr, ss), "Bars", "T_UI_Pip_Diamond_Hollow", "Procedural (HUD M01)",
        notes="Empty spectral energy diamond pip (tint Ash / BoneMuted)"
    )
    # Filled (crisp outer outline + inset glowing solid core)
    img, draw = make_ss_canvas(P, P, ss)
    draw_diamond(draw, pc, pc, 22.0, 22.0, ss=ss, fill=255)
    arr = np.array(img, dtype=np.uint8)
    arr[:hp, hp:] = np.fliplr(arr[:hp, :hp])
    arr[hp:, :] = np.flipud(arr[:hp, :])
    save_white_alpha(
        downsample_ss(arr, ss), "Bars", "T_UI_Pip_Diamond_Filled", "Procedural (HUD M01)",
        notes="Filled spectral energy diamond pip (tint Spectral #8F86A8)"
    )


def generate_bars_and_controls() -> None:
    ss = 4

    def make_tapered_bar_pair(
        W: int,
        H: int,
        tip_x: float,
        shoulder_x: float,
        half_h: float,
        stroke_y: float,
        fill_inset_y: float,
        frame_name: str,
        mask_name: str,
        notes_prefix: str,
        ornate_boss: bool = False,
    ) -> None:
        cy = H / 2.0
        top_y = cy - half_h
        bot_y = cy + half_h
        slope_len = math.hypot(shoulder_x - tip_x, half_h)

        # --- FRAME ---
        img, draw = make_ss_canvas(W, H, ss)
        outer = [
            (tip_x * ss, cy * ss),
            (shoulder_x * ss, top_y * ss),
            ((W - shoulder_x) * ss, top_y * ss),
            ((W - tip_x) * ss, cy * ss),
            ((W - shoulder_x) * ss, bot_y * ss),
            (shoulder_x * ss, bot_y * ss),
        ]
        draw.polygon(outer, fill=255)
        dx_in = stroke_y * slope_len / half_h
        inner = [
            ((tip_x + dx_in) * ss, cy * ss),
            ((shoulder_x + 1.0) * ss, (top_y + stroke_y) * ss),
            ((W - shoulder_x - 1.0) * ss, (top_y + stroke_y) * ss),
            ((W - tip_x - dx_in) * ss, cy * ss),
            ((W - shoulder_x - 1.0) * ss, (bot_y - stroke_y) * ss),
            ((shoulder_x + 1.0) * ss, (bot_y - stroke_y) * ss),
        ]
        draw.polygon(inner, fill=0)

        if ornate_boss:
            # Subtle double outer needle accents flanking the boss bar shoulder
            draw_diamond(draw, cx=tip_x - 10.0, cy=cy, rx=5.5, ry=5.5, ss=ss, fill=255)
            draw_diamond(draw, cx=W - tip_x + 10.0, cy=cy, rx=5.5, ry=5.5, ss=ss, fill=255)
            # Small outer bevel accent ticks above/below shoulder
            draw.polygon([
                ((tip_x + 12) * ss, (cy - half_h - 6) * ss),
                ((shoulder_x + 12) * ss, (cy - half_h - 6) * ss),
                ((shoulder_x + 8) * ss, (cy - half_h - 4) * ss),
                ((tip_x + 18) * ss, (cy - half_h - 4) * ss),
            ], fill=180)

        arr = np.array(img, dtype=np.uint8)
        hw, hh = (W // 2) * ss, (H // 2) * ss
        arr[:hh, hw:] = np.fliplr(arr[:hh, :hw])
        arr[hh:, :] = np.flipud(arr[:hh, :])
        m_x = (shoulder_x + (18.0 if ornate_boss else 8.0)) / W
        # Top/Bottom margin must strictly enclose the horizontal bar strokes so Border draw never clips them
        m_y = (math.ceil(top_y + stroke_y) + 2) / H
        save_white_alpha(
            downsample_ss(arr, ss), "Bars", frame_name, f"Procedural ({notes_prefix})",
            draw_as="Border", margin=[m_x, m_y, m_x, m_y],
            notes=f"{notes_prefix} tapered blade-end 3-slice outline frame"
        )

        # --- FILL MASK ---
        img, draw = make_ss_canvas(W, H, ss)
        dx_m = fill_inset_y * slope_len / half_h
        mask_poly = [
            ((tip_x + dx_m) * ss, cy * ss),
            ((shoulder_x + 1.5) * ss, (top_y + fill_inset_y) * ss),
            ((W - shoulder_x - 1.5) * ss, (top_y + fill_inset_y) * ss),
            ((W - tip_x - dx_m) * ss, cy * ss),
            ((W - shoulder_x - 1.5) * ss, (bot_y - fill_inset_y) * ss),
            ((shoulder_x + 1.5) * ss, (bot_y - fill_inset_y) * ss),
        ]
        draw.polygon(mask_poly, fill=255)
        arr = np.array(img, dtype=np.uint8)
        arr[:hh, hw:] = np.fliplr(arr[:hh, :hw])
        arr[hh:, :] = np.flipud(arr[:hh, :])
        save_white_alpha(
            downsample_ss(arr, ss), "Bars", mask_name, f"Procedural ({notes_prefix})",
            draw_as="Image",
            notes=f"{notes_prefix} solid white fill mask matching frame interior (Blood / BloodDried / Shield)"
        )

    # 1. Player Health Bar (1024x64 -> at 1080p ~420x12 px inner height)
    make_tapered_bar_pair(
        W=1024, H=64, tip_x=16.0, shoulder_x=64.0, half_h=14.0,
        stroke_y=2.0, fill_inset_y=2.5,
        frame_name="T_UI_Bar_Health_Frame",
        mask_name="T_UI_Bar_Health_FillMask",
        notes_prefix="HUD M01 Health Bar",
    )

    # 2. Player Shield Bar (1024x32 -> ultra-sleek hairline silver bar above health)
    make_tapered_bar_pair(
        W=1024, H=32, tip_x=16.0, shoulder_x=48.0, half_h=6.0,
        stroke_y=1.6, fill_inset_y=1.8,
        frame_name="T_UI_Bar_Shield_Frame",
        mask_name="T_UI_Bar_Shield_FillMask",
        notes_prefix="HUD M01 Shield Bar",
    )

    # 3. Boss Health Bar (1536x80 -> grand ~1100px boss bar M02)
    make_tapered_bar_pair(
        W=1536, H=80, tip_x=28.0, shoulder_x=92.0, half_h=16.0,
        stroke_y=2.4, fill_inset_y=3.0,
        frame_name="T_UI_Bar_Boss_Frame",
        mask_name="T_UI_Bar_Boss_FillMask",
        notes_prefix="Boss M02 Health Bar",
        ornate_boss=True,
    )

    # 4. Boss Phase Threshold Marker (64x64)
    P = 64
    img, draw = make_ss_canvas(P, P, ss)
    draw_gothic_star_4pt(draw, cx=32.0, cy=32.0, rx=9.0, ry_top=24.0, ry_bot=24.0, waist=4.2, ss=ss, fill=255)
    arr = np.array(img, dtype=np.uint8)
    hp = (P // 2) * ss
    arr[:hp, hp:] = np.fliplr(arr[:hp, :hp])
    arr[hp:, :] = np.flipud(arr[:hp, :])
    save_white_alpha(
        downsample_ss(arr, ss), "Bars", "T_UI_Bar_PhaseMarker", "Procedural (Boss M02)",
        notes="Boss health bar phase-threshold diamond marker"
    )

    # 5. Settings Slider Track (512x32, 3-slice) & Diamond Thumb (64x64)
    W, H = 512, 32
    arr = np.zeros((H * ss, W * ss), dtype=np.uint8)
    # 2px center hairline
    arr[15 * ss : 17 * ss, 8 * ss : (W - 8) * ss] = 255
    # Crisp vertical end caps (10px tall, 2px wide)
    arr[11 * ss : 21 * ss, 8 * ss : 10 * ss] = 255
    arr[11 * ss : 21 * ss, (W - 10) * ss : (W - 8) * ss] = 255
    save_white_alpha(
        downsample_ss(arr, ss), "Bars", "T_UI_Slider_Track", "Procedural (Settings M07)",
        draw_as="Box", margin=[16 / W, 0.0, 16 / W, 0.0],
        notes="Hairline horizontal 3-slice slider track with vertical end ticks (Draw As = Box)"
    )
    # Slider thumb: crisp Gothic diamond with sharp tiny hollow center slit
    img, draw = make_ss_canvas(64, 64, ss)
    draw_diamond(draw, 32.0, 32.0, 14.0, 18.0, ss=ss, fill=255)
    draw_diamond(draw, 32.0, 32.0, 4.5, 6.5, ss=ss, fill=0)
    arr = np.array(img, dtype=np.uint8)
    arr[:hp, hp:] = np.fliplr(arr[:hp, :hp])
    arr[hp:, :] = np.flipud(arr[:hp, :])
    save_white_alpha(
        downsample_ss(arr, ss), "Bars", "T_UI_Slider_Thumb", "Procedural (Settings M07)",
        notes="Gothic faceted diamond slider thumb"
    )

    # 6. Circular Hold-Progress Ring (256x256, M03 Interaction prompt)
    S = 256
    yy, xx = np.mgrid[0 : S * ss, 0 : S * ss].astype(np.float32)
    c = (S * ss - 1) / 2.0
    dist = np.hypot(xx - c, yy - c) / ss
    r_mid = 108.0
    half_w = 2.0
    ring_ss = np.clip((half_w + 0.5 - np.abs(dist - r_mid)) * 255.0, 0, 255).astype(np.uint8)
    save_white_alpha(
        downsample_ss(ring_ss, ss), "Bars", "T_UI_Ring_Progress", "Procedural (Interact M03)",
        notes="Uniform thin circular ring for radial hold-to-interact progress material"
    )


def generate_ornaments_and_markers() -> None:
    ss = 4

    # 1. Option Row Bottom Star (64x64) — exact match to zoom_option_rows.png
    img, draw = make_ss_canvas(64, 64, ss)
    draw_gothic_star_4pt(draw, cx=32.0, cy=32.0, rx=9.0, ry_top=22.0, ry_bot=16.0, waist=4.2, ss=ss, fill=255)
    arr = np.array(img, dtype=np.uint8)
    arr[:, 32 * ss :] = np.fliplr(arr[:, : 32 * ss])
    save_white_alpha(
        downsample_ss(arr, ss), "Ornaments", "T_UI_Orn_RowStar", "Procedural (Mockup_Ajustes)",
        notes="Standalone bottom diamond-star ornament anchored under Settings option values"
    )

    # 2. Standalone Tapered Chevrons (<, >, v) — exact match to zoom_option_rows.png
    img, draw = make_ss_canvas(64, 96, ss)
    draw_tapered_chevron_polygon(draw, tip_x=16.0, cy=48.0, dx=32.0, half_h=38.0, thickness=6.2, direction=-1, ss=ss)
    arr_l = np.array(img, dtype=np.uint8)
    arr_l[48 * ss :, :] = np.flipud(arr_l[: 48 * ss, :])
    chev_l = downsample_ss(arr_l, ss)
    save_white_alpha(
        chev_l, "Ornaments", "T_UI_Orn_Chevron_L", "Procedural (Mockup_Ajustes)",
        notes="Razor-sharp tapered left chevron '<' for Settings value cyclers"
    )
    save_white_alpha(
        np.fliplr(chev_l), "Ornaments", "T_UI_Orn_Chevron_R", "Procedural (Mockup_Ajustes)",
        notes="Razor-sharp tapered right chevron '>' (exact mirror of L)"
    )
    # Downward chevron for dropdowns (64x48) — matching sleek razor-tapered weight of L/R chevrons
    img, draw = make_ss_canvas(64, 48, ss)
    pts = [
        (32.0 * ss, 35.0 * ss),
        (10.0 * ss, 15.0 * ss),
        (32.0 * ss, 30.8 * ss),
        (54.0 * ss, 15.0 * ss),
    ]
    draw.polygon(pts, fill=255)
    arr_d = np.array(img, dtype=np.uint8)
    arr_d[:, 32 * ss :] = np.fliplr(arr_d[:, : 32 * ss])
    save_white_alpha(
        downsample_ss(arr_d, ss), "Ornaments", "T_UI_Orn_Chevron_Down", "Procedural (Settings M07)",
        notes="Sharp tapered downward chevron for dropdown menus"
    )

    # 3. Crisp Hollow & Solid Selection Diamonds (64x64) — exact match to zoom_left_sidebar.png
    hp = 32 * ss
    img, draw = make_ss_canvas(64, 64, ss)
    draw_diamond(draw, 32.0, 32.0, 20.0, 20.0, ss=ss, fill=255)
    draw_diamond(draw, 32.0, 32.0, 12.5, 12.5, ss=ss, fill=0)
    arr = np.array(img, dtype=np.uint8)
    arr[:hp, hp:] = np.fliplr(arr[:hp, :hp])
    arr[hp:, :] = np.flipud(arr[:hp, :])
    save_white_alpha(
        downsample_ss(arr, ss), "Ornaments", "T_UI_Marker_Diamond_Hollow", "Procedural (Mockup_Ajustes)",
        notes="Crisp hollow diamond marker for Settings sidebar categories (Normal & Focused)"
    )
    img, draw = make_ss_canvas(64, 64, ss)
    draw_diamond(draw, 32.0, 32.0, 18.0, 18.0, ss=ss, fill=255)
    arr = np.array(img, dtype=np.uint8)
    arr[:hp, hp:] = np.fliplr(arr[:hp, :hp])
    arr[hp:, :] = np.flipud(arr[:hp, :])
    save_white_alpha(
        downsample_ss(arr, ss), "Ornaments", "T_UI_Marker_Diamond_Solid", "Procedural (PromptKit §2.4)",
        notes="Solid diamond selection marker for hovered/selected menu buttons"
    )

    # 4. Top Tab Bar Divider (32x64) — exact match to zoom_top_tabs.png
    img, draw = make_ss_canvas(32, 64, ss)
    # Tapered vertical needle + small central diamond
    pts = [
        (16.0 * ss, 6.0 * ss),
        (17.2 * ss, 26.0 * ss),
        (21.5 * ss, 32.0 * ss),
        (17.2 * ss, 38.0 * ss),
        (16.0 * ss, 58.0 * ss),
        (14.8 * ss, 38.0 * ss),
        (10.5 * ss, 32.0 * ss),
        (14.8 * ss, 26.0 * ss),
    ]
    draw.polygon(pts, fill=255)
    arr = np.array(img, dtype=np.uint8)
    arr[: 32 * ss, 16 * ss :] = np.fliplr(arr[: 32 * ss, : 16 * ss])
    arr[32 * ss :, :] = np.flipud(arr[: 32 * ss, :])
    save_white_alpha(
        downsample_ss(arr, ss), "Ornaments", "T_UI_Orn_TabDivider", "Procedural (Mockup_Ajustes)",
        notes="Vertical needle-with-diamond separator between top navigation tabs"
    )

    # 5. Active Tab Underline + Central Vertical Diamond (256x32) — exact match to AJUSTES red underline
    W, H = 256, 32
    img, draw = make_ss_canvas(W, H, ss)
    draw_gothic_star_4pt(draw, cx=W / 2.0, cy=H / 2.0, rx=7.5, ry_top=12.0, ry_bot=12.0, waist=3.2, ss=ss, fill=255)
    arr = np.array(img, dtype=np.uint8)
    # Crisp 2px horizontal line tapering slightly at the far ends
    xs = np.arange(W * ss, dtype=np.float32) / ss
    u = np.abs(xs - W / 2.0) / (W / 2.0)
    line_fade = np.clip(1.0 - np.maximum(0.0, (u - 0.85) / 0.15), 0.0, 1.0)
    line_row = np.round(255.0 * line_fade).astype(np.uint8)
    arr[15 * ss : 17 * ss, :] = np.maximum(arr[15 * ss : 17 * ss, :], line_row[None, :])
    arr[: 16 * ss, 128 * ss :] = np.fliplr(arr[: 16 * ss, : 128 * ss])
    arr[16 * ss :, :] = np.flipud(arr[: 16 * ss, :])
    save_white_alpha(
        downsample_ss(arr, ss), "Ornaments", "T_UI_Orn_TabActiveLine", "Procedural (Mockup_Ajustes)",
        notes="Active top-tab underline with centered elongated diamond (tint BloodBright)"
    )

    # 6. Vertical Sidebar Accent Line with Gothic Star (32x512) — left edge of Mockup_Ajustes
    W, H = 32, 512
    img, draw = make_ss_canvas(W, H, ss)
    # 4-point star positioned cleanly at 25% height (or centered — center is most reusable!)
    draw_gothic_star_4pt(draw, cx=16.0, cy=256.0, rx=9.0, ry_top=28.0, ry_bot=28.0, waist=3.0, ss=ss, fill=255)
    arr = np.array(img, dtype=np.uint8)
    ys = np.arange(H * ss, dtype=np.float32) / ss
    v = np.abs(ys - H / 2.0) / (H / 2.0)
    v_fade = np.clip(1.0 - np.maximum(0.0, (v - 0.72) / 0.28), 0.0, 1.0)
    v_col = np.round(210.0 * v_fade).astype(np.uint8)
    arr[:, 15 * ss : 17 * ss] = np.maximum(arr[:, 15 * ss : 17 * ss], v_col[:, None])
    arr[:, 16 * ss :] = np.fliplr(arr[:, : 16 * ss])
    save_white_alpha(
        downsample_ss(arr, ss), "Ornaments", "T_UI_Orn_SidebarLine_V", "Procedural (Mockup_Ajustes)",
        notes="Vertical fading hairline divider with centered gothic star (Settings column separator)"
    )


def generate_gradients() -> None:
    # 1. Centered Horizontal Hover Fade (0 -> 1 -> 0) for buttons/rows
    W, H = 512, 8
    u = np.linspace(0.0, 1.0, W, dtype=np.float32)
    # Raised smooth bell curve (stays rich across middle ~40%, silky falloff to true 0 at edges)
    bell = np.maximum(0.0, np.sin(np.pi * u)) ** 1.55
    alpha = np.tile(np.round(bell * 255.0).astype(np.uint8)[None, :], (H, 1))
    save_white_alpha(
        alpha, "Gradients", "T_UI_Grad_HFade_Center", "Procedural (PromptKit §2.4)",
        notes="Smooth 0->1->0 horizontal hover strip gradient (tint Blood @ 70-85% alpha)"
    )

    # 2. Left-to-Right Soft Fade (1 -> 0) for Settings left sidebar 'JUEGO' active strip
    t = np.clip((u - 0.12) / 0.88, 0.0, 1.0)
    ltr = (1.0 - (3 * t**2 - 2 * t**3)) ** 1.25
    alpha_ltr = np.tile(np.round(ltr * 255.0).astype(np.uint8)[None, :], (H, 1))
    save_white_alpha(
        alpha_ltr, "Gradients", "T_UI_Grad_HFade_LeftToRight", "Procedural (Mockup_Ajustes)",
        notes="Left-anchored fade-to-right gradient (Settings active sidebar & left-aligned menus)"
    )

    # 3. Right-to-Left Soft Fade (0 -> 1) for Top-Right Notifications (M03)
    save_white_alpha(
        np.fliplr(alpha_ltr), "Gradients", "T_UI_Grad_HFade_RightToLeft", "Procedural (HUD M03)",
        notes="Right-anchored fade-to-left gradient for top-right objective/loot notifications"
    )

    # 4. Bottom-Up Vertical Fade (0 top -> 1 bottom) for Dialogue & Subtitle bands (M03/M04)
    W_v, H_v = 8, 256
    v = np.linspace(0.0, 1.0, H_v, dtype=np.float32)
    bu = (3 * v**2 - 2 * v**3) ** 1.15
    alpha_bu = np.tile(np.round(bu * 255.0).astype(np.uint8)[:, None], (1, W_v))
    save_white_alpha(
        alpha_bu, "Gradients", "T_UI_Grad_VFade_BottomUp", "Procedural (Dialogue M03/M04)",
        notes="Transparent-top to opaque-bottom vertical band (subtitles & dialogue backdrop)"
    )

    # 5. Soft Radial Edge Vignette (512x512, 0 center -> 1 corners) for Death Screen / Modals
    S = 512
    yy, xx = np.mgrid[0:S, 0:S].astype(np.float32)
    nx = (xx - (S - 1) / 2.0) / ((S - 1) / 2.0)
    ny = (yy - (S - 1) / 2.0) / ((S - 1) / 2.0)
    r = np.hypot(nx, ny)
    t_r = np.clip((r - 0.35) / 0.85, 0.0, 1.0)
    vig = (3 * t_r**2 - 2 * t_r**3)
    alpha_vig = np.clip(np.round(vig * 255.0), 0, 255).astype(np.uint8)
    save_white_alpha(
        alpha_vig, "Gradients", "T_UI_Grad_Vignette_Radial", "Procedural (Death M11)",
        notes="Smooth radial edge vignette mask (tint Void or BloodDried for death/modal overlays)"
    )


def write_manifest() -> None:
    manifest_path = OUT_DIR / "manifest.json"
    data = {
        "project": "Velmara (TesisUE)",
        "ue_import_settings": {
            "TextureGroup": "TEXTUREGROUP_UI",
            "CompressionSettings": "TC_EditorIcon",  # UserInterface2D (RGBA)
            "MipGenSettings": "TMGS_NoMipmaps",
            "SRGB": True,
            "NeverStream": True,
        },
        "total_assets": len(MANIFEST),
        "assets": [asdict(m) for m in MANIFEST],
    }
    manifest_path.write_text(json.dumps(data, indent=2, ensure_ascii=False), encoding="utf-8")


def main() -> None:
    ensure_dirs()
    extract_asset_01()
    extract_asset_02()
    extract_asset_03()
    extract_assets_04_05_06()
    extract_asset_07()
    extract_asset_08()
    generate_frames()
    generate_slots()
    generate_bars_and_controls()
    generate_ornaments_and_markers()
    generate_gradients()
    write_manifest()
    print(f"SUCCESS: Exported {len(MANIFEST)} textures to {OUT_DIR}")
    by_cat: Dict[str, int] = {}
    for m in MANIFEST:
        by_cat[m.category] = by_cat.get(m.category, 0) + 1
    for k, v in sorted(by_cat.items()):
        print(f"  {k:12s}: {v}")


if __name__ == "__main__":
    main()
