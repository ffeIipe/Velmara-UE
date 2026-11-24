"""
Velmara UI — Phase 3 Step B: Shared WBP_MenuButton, Legacy Common Borders, & Loading Screens
Styles:
  - /Game/Blueprints/HUD/Common/BS_Bg_Menu & BS_Frame_Large (upgraded to Void80 & Brackets Panel)
  - /Game/Blueprints/HUD/Menus/WBP_MenuButton (BloodBright diamond markers + BP_ButtonStyle)
  - /Game/Blueprints/HUD/Menus/WBP_LoadingScreen (Void Black #0A0809 + vignette + tip + spinner)
  - /Game/Blueprints/HUD/WBP_VelmaraBootLoading (matching Void Black boot loading screen)
"""

import unreal

eal = unreal.EditorAssetLibrary
TEX_ROOT = "/Game/UI/Textures"
STYLES_DIR = "/Game/Blueprints/HUD/Styles"

HEX = {
    "Void": "#0A0809",
    "Charcoal": "#1C1A1C",
    "Ash": "#3A3638",
    "Bone": "#E8E0D0",
    "BoneMuted": "#A39C8E",
    "Blood": "#8E0F14",
    "BloodBright": "#C21A1F",
    "BloodDried": "#4A0A0D",
    "White": "#FFFFFF",
}


def srgb_to_lin(c8: int) -> float:
    c = c8 / 255.0
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def lin(tok: str, alpha: float = 1.0) -> unreal.LinearColor:
    hx = HEX[tok].lstrip("#")
    return unreal.LinearColor(
        srgb_to_lin(int(hx[0:2], 16)),
        srgb_to_lin(int(hx[2:4], 16)),
        srgb_to_lin(int(hx[4:6], 16)),
        alpha,
    )


def slate_col(tok: str, alpha: float = 1.0) -> unreal.SlateColor:
    sc = unreal.SlateColor()
    sc.set_editor_property("specified_color", lin(tok, alpha))
    sc.set_editor_property("color_use_rule", unreal.SlateColorStylingMode.USE_COLOR_SPECIFIED)
    return sc


def vec2_slate(x: float, y: float) -> unreal.DeprecateSlateVector2D:
    v = unreal.DeprecateSlateVector2D()
    v.import_text(f"(X={float(x):.2f},Y={float(y):.2f})")
    return v


def make_brush(
    tex_path: str | None = None,
    draw_as: str = "Image",
    tint: str = "White",
    alpha: float = 1.0,
    margin: tuple[float, float, float, float] | None = None,
    image_size: tuple[float, float] | None = None,
) -> unreal.SlateBrush:
    dm = {
        "NoDrawType": unreal.SlateBrushDrawType.NO_DRAW_TYPE,
        "Box": unreal.SlateBrushDrawType.BOX,
        "Border": unreal.SlateBrushDrawType.BORDER,
        "Image": unreal.SlateBrushDrawType.IMAGE,
    }
    b = unreal.SlateBrush()
    b.set_editor_property("draw_as", dm[draw_as])
    b.set_editor_property("tint_color", slate_col(tint, alpha))
    if tex_path:
        t = unreal.load_asset(tex_path)
        if not t:
            raise RuntimeError(f"Missing texture: {tex_path}")
        b.set_editor_property("resource_object", t)
        if image_size:
            b.set_editor_property("image_size", vec2_slate(image_size[0], image_size[1]))
        elif draw_as == "Border":
            b.set_editor_property("image_size", vec2_slate(t.blueprint_get_size_x() * 0.5, t.blueprint_get_size_y() * 0.5))
        else:
            b.set_editor_property("image_size", vec2_slate(t.blueprint_get_size_x(), t.blueprint_get_size_y()))
    else:
        b.set_editor_property("resource_object", None)
    b.set_editor_property("margin", unreal.Margin(*margin) if margin else unreal.Margin(0.0, 0.0, 0.0, 0.0))
    return b


def set_canvas_slot(w, anc_min, anc_max, pos, size, align=(0.0, 0.0), z=0, auto_size=False):
    slot = w.slot
    if not isinstance(slot, unreal.CanvasPanelSlot):
        return
    ld = unreal.AnchorData()
    ld.anchors = unreal.Anchors(minimum=unreal.Vector2D(*anc_min), maximum=unreal.Vector2D(*anc_max))
    ld.offsets = unreal.Margin(pos[0], pos[1], size[0], size[1])
    ld.alignment = unreal.Vector2D(*align)
    slot.set_layout(ld)
    slot.set_auto_size(auto_size)
    slot.set_z_order(z)


def style_cls(name: str):
    return unreal.load_asset(f"{STYLES_DIR}/{name}").generated_class()


def finish_bp(bp):
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    eal.save_loaded_asset(bp, only_if_is_dirty=False)


def update_legacy_common_borders():
    bg_bp = unreal.load_asset("/Game/Blueprints/HUD/Common/BS_Bg_Menu")
    if bg_bp:
        cdo = unreal.get_default_object(bg_bp.generated_class())
        cdo.set_editor_property("background", make_brush(None, "Box", "Void", 0.80))
        finish_bp(bg_bp)

    fr_bp = unreal.load_asset("/Game/Blueprints/HUD/Common/BS_Frame_Large")
    if fr_bp:
        cdo = unreal.get_default_object(fr_bp.generated_class())
        cdo.set_editor_property(
            "background",
            make_brush(
                f"{TEX_ROOT}/Frames/T_UI_Frame_Brackets_Panel",
                "Border",
                "Bone",
                0.92,
                margin=(0.15625, 0.15625, 0.15625, 0.15625),
            ),
        )
        finish_bp(fr_bp)
    print("Updated legacy BS_Bg_Menu & BS_Frame_Large")


def restyle_menu_button():
    wb = unreal.load_asset("/Game/Blueprints/HUD/Menus/WBP_MenuButton")
    wt = unreal.find_object(wb, "WidgetTree")

    cdo = unreal.get_default_object(wb.generated_class())
    cdo.set_editor_property("style", style_cls("BP_ButtonStyle"))

    al = unreal.find_object(wt, "ArrowLeft")
    ar = unreal.find_object(wt, "ArrowRight")
    lbl = unreal.find_object(wt, "Label")

    diamond_brush = make_brush(f"{TEX_ROOT}/Ornaments/T_UI_Marker_Diamond_Solid", "Image", "BloodBright", 1.0, image_size=(14, 14))
    for img, pad in ((al, (0.0, 0.0, 14.0, 0.0)), (ar, (14.0, 0.0, 0.0, 0.0))):
        img.set_editor_property("brush", diamond_brush)
        if isinstance(img.slot, unreal.HorizontalBoxSlot):
            img.slot.set_padding(unreal.Margin(*pad))
            img.slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)

    lbl.set_editor_property("style", style_cls("BP_ButtonTextStyle"))
    lbl.set_editor_property("justification", unreal.TextJustify.CENTER)
    if isinstance(lbl.slot, unreal.HorizontalBoxSlot):
        lbl.slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)

    finish_bp(wb)
    print("Restyled WBP_MenuButton")


def restyle_loading_screen(wb_path: str, bg_border_name: str, tip_text: str):
    wb = unreal.load_asset(wb_path)
    wt = unreal.find_object(wb, "WidgetTree")

    bg = unreal.find_object(wt, bg_border_name)
    vig = unreal.find_object(wt, "VignetteOverlay")
    tip_box = unreal.find_object(wt, "TipContainer")
    tip_hdr = unreal.find_object(wt, "TipHeader")
    tip_sep = unreal.find_object(wt, "TipSeparator")
    tip_body = unreal.find_object(wt, "TipBody")
    spinner = unreal.find_object(wt, "LoadingSpinner")

    # Fullscreen Void Black (#0A0809)
    bg.set_editor_property("background", make_brush(None, "Box", "Void", 1.0))
    bg.set_brush_color(lin("Void", 1.0))
    set_canvas_slot(bg, (0, 0), (1, 1), (0, 0), (0, 0), (0, 0), z=0)

    # Subtle dark blood-dried radial vignette overlay
    vig.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Gradients/T_UI_Grad_Vignette_Radial", "Image", "BloodDried", 0.35))
    vig.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    set_canvas_slot(vig, (0, 0), (1, 1), (0, 0), (0, 0), (0, 0), z=1)

    # Bottom-left lore tip (64px safe margin)
    set_canvas_slot(tip_box, (0, 1), (0, 1), (64, -64), (720, 160), (0, 1), z=2, auto_size=True)
    tip_hdr.set_editor_property("style", style_cls("TS_Speaker"))
    tip_hdr.set_text("CONSEJO")
    if isinstance(tip_hdr.slot, unreal.VerticalBoxSlot):
        tip_hdr.slot.set_padding(unreal.Margin(0, 0, 0, 6))

    tip_sep.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Ornaments/T_UI_Orn_Sep_Diamond", "Image", "Bone", 0.85, image_size=(420, 20)))
    if isinstance(tip_sep.slot, unreal.VerticalBoxSlot):
        tip_sep.slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_LEFT)
        tip_sep.slot.set_padding(unreal.Margin(0, 0, 0, 12))

    tip_body.set_editor_property("style", style_cls("TS_Body"))
    tip_body.set_text(tip_text)
    tip_body.set_auto_wrap_text(True)
    tip_body.set_editor_property("wrap_text_at", 680.0)

    # Bottom-right rotating moon sigil spinner (64px safe margin, center pivot)
    spinner.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Ornaments/T_UI_Spinner_Moon", "Image", "Bone", 0.90, image_size=(76, 76)))
    spinner.set_render_transform_pivot(unreal.Vector2D(0.5, 0.5))
    set_canvas_slot(spinner, (1, 1), (1, 1), (-64, -64), (76, 76), (1, 1), z=2)

    finish_bp(wb)
    print(f"Restyled {wb_path.split('/')[-1]}")


if __name__ == "__main__":
    update_legacy_common_borders()
    restyle_menu_button()
    restyle_loading_screen(
        "/Game/Blueprints/HUD/Menus/WBP_LoadingScreen",
        "Border_61",
        "La luz del sol debilita tu esencia; busca las sombras de la catedral para recuperar tu fuerza.",
    )
    restyle_loading_screen(
        "/Game/Blueprints/HUD/WBP_VelmaraBootLoading",
        "VoidBg",
        "«Hay verdades que solo se oyen en el silencio.»",
    )
    print("=== PHASE 3 SHARED & LOADING COMPLETE ===")
