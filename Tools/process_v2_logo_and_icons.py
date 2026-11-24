from __future__ import annotations

import pathlib
import numpy as np

ROOT = pathlib.Path(r"i:\UnrealProjects\TesisUE")
RAW_DIR = ROOT / "UI_Raw"
OUT_DIR = ROOT / "UI_Processed"


def round_up_mult4(n: int) -> int:
    return ((n + 3) // 4) * 4


def tight_bbox(alpha: np.ndarray, thresh: int = 6):
    ys, xs = np.where(alpha > thresh)
    if len(ys) == 0:
        return (0, 0, alpha.shape[1], alpha.shape[0])
    return (int(xs.min()), int(ys.min()), int(xs.max()) + 1, int(ys.max()) + 1)


def load_rgba_topdown(path: pathlib.Path) -> np.ndarray:
    try:
        from PIL import Image
        return np.array(Image.open(path).convert("RGBA"), dtype=np.uint8)
    except ImportError:
        import bpy
        img = bpy.data.images.load(str(path), check_existing=False)
        w, h = img.size[0], img.size[1]
        px = np.array(img.pixels[:], dtype=np.float32).reshape((h, w, 4))
        px = np.flipud(px)
        bpy.data.images.remove(img)
        return np.clip(np.round(px * 255.0), 0.0, 255.0).astype(np.uint8)


def save_rgba_topdown(rgba: np.ndarray, path: pathlib.Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    try:
        from PIL import Image
        Image.fromarray(rgba, mode="RGBA").save(path, optimize=True)
    except ImportError:
        import bpy
        h, w = rgba.shape[0], rgba.shape[1]
        img = bpy.data.images.new(name=path.stem, width=w, height=h, alpha=True, float_buffer=False)
        img.colorspace_settings.name = "sRGB"
        flipped = np.flipud(rgba).astype(np.float32) / 255.0
        img.pixels.foreach_set(flipped.ravel())
        img.filepath_raw = str(path)
        img.file_format = "PNG"
        img.save()
        bpy.data.images.remove(img)


def resize_rgba_topdown(rgba: np.ndarray, new_w: int, new_h: int) -> np.ndarray:
    try:
        from PIL import Image
        return np.array(
            Image.fromarray(rgba, mode="RGBA").resize((new_w, new_h), Image.Resampling.LANCZOS),
            dtype=np.uint8,
        )
    except ImportError:
        import bpy
        h, w = rgba.shape[0], rgba.shape[1]
        img = bpy.data.images.new(name="tmp_resize", width=w, height=h, alpha=True, float_buffer=False)
        flipped = np.flipud(rgba).astype(np.float32) / 255.0
        img.pixels.foreach_set(flipped.ravel())
        img.scale(new_w, new_h)
        out_px = np.array(img.pixels[:], dtype=np.float32).reshape((new_h, new_w, 4))
        out_px = np.flipud(out_px)
        bpy.data.images.remove(img)
        return np.clip(np.round(out_px * 255.0), 0.0, 255.0).astype(np.uint8)


def process_logo_v2() -> pathlib.Path:
    src_path = RAW_DIR / "Logo_Velmara_Remaster.png.png"
    rgba_in = load_rgba_topdown(src_path)
    rgb = rgba_in[..., :3].astype(np.float32)

    max_c = rgb.max(axis=2)
    alpha_f = np.clip((max_c - 10.0) / 55.0, 0.0, 1.0)

    safe_a = np.maximum(alpha_f, 0.15)[..., None]
    unpremult = np.where(alpha_f[..., None] > 0.01, np.clip(rgb / safe_a, 0.0, 255.0), 0.0)
    rgb_out = np.where(alpha_f[..., None] >= 0.99, rgb, unpremult)

    rgba = np.zeros((rgb.shape[0], rgb.shape[1], 4), dtype=np.uint8)
    rgba[..., :3] = np.round(rgb_out).astype(np.uint8)
    rgba[..., 3] = np.round(alpha_f * 255.0).astype(np.uint8)

    x0, y0, x1, y1 = tight_bbox(rgba[..., 3], thresh=8)
    cropped = rgba[y0:y1, x0:x1]

    pad = 16
    ch, cw = cropped.shape[:2]
    canvas_w = round_up_mult4(cw + 2 * pad)
    canvas_h = round_up_mult4(ch + 2 * pad)
    canvas = np.zeros((canvas_h, canvas_w, 4), dtype=np.uint8)
    ox = (canvas_w - cw) // 2
    oy = (canvas_h - ch) // 2
    canvas[oy : oy + ch, ox : ox + cw] = cropped

    if canvas_w > 1024:
        scale = 1024.0 / canvas_w
        nw = round_up_mult4(int(round(canvas_w * scale)))
        nh = round_up_mult4(int(round(canvas_h * scale)))
        canvas = resize_rgba_topdown(canvas, nw, nh)

    out_path = OUT_DIR / "Backgrounds" / "T_UI_Logo_Velmara_V2.png"
    save_rgba_topdown(canvas, out_path)
    print(f"Saved logo V2: {out_path} ({canvas.shape[1]}x{canvas.shape[0]})")
    return out_path


def process_extra_icons() -> list[pathlib.Path]:
    src_path = RAW_DIR / "Asset-Extra-01.png"
    rgba_in = load_rgba_topdown(src_path)
    rgb = rgba_in[..., :3].astype(np.float32)
    h, w, _ = rgb.shape

    gray = rgb.max(axis=2)
    alpha_full = np.round(np.clip((gray - 12.0) / 228.0, 0.0, 1.0) * 255.0).astype(np.uint8)

    names = [
        ("T_UI_Icon_Menu_Audio", 0, 0),
        ("T_UI_Icon_Menu_SaveSlot", 0, 1),
        ("T_UI_Icon_Menu_Controls", 0, 2),
        ("T_UI_Icon_Menu_Display", 1, 0),
        ("T_UI_Icon_Menu_Language", 1, 1),
        ("T_UI_Icon_Menu_Tip", 1, 2),
    ]

    cell_w = w // 3
    cell_h = h // 2
    target_dim = 256
    pad_px = 20
    outputs = []

    for name, row, col in names:
        x0 = col * cell_w
        y0 = row * cell_h
        x1 = w if col == 2 else (col + 1) * cell_w
        y1 = h if row == 1 else (row + 1) * cell_h

        cell = alpha_full[y0:y1, x0:x1]
        bx0, by0, bx1, by1 = tight_bbox(cell, thresh=6)
        crop = cell[by0:by1, bx0:bx1]

        cw, ch = crop.shape[1], crop.shape[0]
        max_dim = target_dim - 2 * pad_px
        scale = min(max_dim / cw, max_dim / ch)
        nw = max(1, int(round(cw * scale)))
        nh = max(1, int(round(ch * scale)))

        crop_rgba = np.zeros((ch, cw, 4), dtype=np.uint8)
        crop_rgba[..., 0] = 255
        crop_rgba[..., 1] = 255
        crop_rgba[..., 2] = 255
        crop_rgba[..., 3] = crop
        resized = resize_rgba_topdown(crop_rgba, nw, nh)

        rgba = np.zeros((target_dim, target_dim, 4), dtype=np.uint8)
        rgba[..., 0] = 255
        rgba[..., 1] = 255
        rgba[..., 2] = 255
        ox = (target_dim - nw) // 2
        oy = (target_dim - nh) // 2
        rgba[oy : oy + nh, ox : ox + nw, 3] = resized[..., 3]

        dst = OUT_DIR / "Icons" / f"{name}.png"
        save_rgba_topdown(rgba, dst)
        print(f"Saved icon: {dst} ({target_dim}x{target_dim})")
        outputs.append(dst)

    return outputs


if __name__ == "__main__":
    process_logo_v2()
    process_extra_icons()
