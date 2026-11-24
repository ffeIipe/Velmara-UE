"""
Velmara UI — Phase 3 Step A: HUD M01 & Boss Bar M02 Restyling
Applies exact Velmara dark-fantasy visual styling to:
  - /Game/Blueprints/HUD/Common/WBP_DramaticBar
  - /Game/Blueprints/HUD/Common/WBP_DramaticBar_Energy
  - /Game/Blueprints/HUD/WBP_NewPlayerVitals
  - /Game/Blueprints/HUD/WBP_PlayerVitals
  - /Game/Blueprints/HUD/WBP_PlayerHUD
  - /Game/Blueprints/HUD/WBP_PaladinBossHealthBar
Preserves 100% of existing C++ BindWidget / Blueprint variable bindings.
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
    "Shield": "#B8C0C4",
    "Spectral": "#8F86A8",
    "Sunlight": "#D9A441",
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
    b.set_editor_property("tiling", unreal.SlateBrushTileType.NO_TILE)
    b.set_editor_property("mirroring", unreal.SlateBrushMirrorType.NO_MIRROR)
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
    b.set_editor_property(
        "margin",
        unreal.Margin(*margin) if margin else unreal.Margin(0.0, 0.0, 0.0, 0.0),
    )
    return b


def set_canvas_slot(
    w,
    anc_min: tuple[float, float],
    anc_max: tuple[float, float],
    pos: tuple[float, float],
    size: tuple[float, float],
    align: tuple[float, float] = (0.0, 0.0),
    z: int = 0,
    auto_size: bool = False,
):
    slot = w.slot
    if not isinstance(slot, unreal.CanvasPanelSlot):
        return
    ld = unreal.AnchorData()
    ld.anchors = unreal.Anchors(
        minimum=unreal.Vector2D(anc_min[0], anc_min[1]),
        maximum=unreal.Vector2D(anc_max[0], anc_max[1]),
    )
    ld.offsets = unreal.Margin(pos[0], pos[1], size[0], size[1])
    ld.alignment = unreal.Vector2D(align[0], align[1])
    slot.set_layout(ld)
    slot.set_auto_size(auto_size)
    slot.set_z_order(z)


def set_overlay_fill(w, pad: tuple[float, float, float, float] = (0, 0, 0, 0)):
    slot = w.slot
    if not isinstance(slot, unreal.OverlaySlot):
        return
    slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
    slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_FILL)
    slot.set_padding(unreal.Margin(*pad))


def style_progress_bar(
    pb,
    mask_tex: str,
    fill_token: str,
    fill_alpha: float = 1.0,
    bg_token: str | None = "Charcoal",
    bg_alpha: float = 0.88,
):
    st = unreal.ProgressBarStyle()
    if bg_token:
        st.set_editor_property("background_image", make_brush(mask_tex, "Image", bg_token, bg_alpha))
    else:
        st.set_editor_property("background_image", make_brush(None, "NoDrawType"))
    st.set_editor_property("fill_image", make_brush(mask_tex, "Image", "White", 1.0))
    st.set_editor_property("marquee_image", make_brush(None, "NoDrawType"))
    pb.set_editor_property("widget_style", st)
    pb.set_editor_property("fill_color_and_opacity", lin(fill_token, fill_alpha))
    pb.set_editor_property("bar_fill_type", unreal.ProgressBarFillType.LEFT_TO_RIGHT)
    pb.set_editor_property("bar_fill_style", unreal.ProgressBarFillStyle.MASK)


def style_text_cls(name: str):
    bp = unreal.load_asset(f"{STYLES_DIR}/{name}")
    return bp.generated_class()


def finish_wb(wb):
    unreal.BlueprintEditorLibrary.compile_blueprint(wb)
    eal.save_loaded_asset(wb, only_if_is_dirty=False)


# -----------------------------------------------------------------------------
# 1. WBP_DramaticBar (Player Health Bar with BloodDried Ghost Trail & Finials)
# -----------------------------------------------------------------------------
def restyle_dramatic_bar():
    wb = unreal.load_asset("/Game/Blueprints/HUD/Common/WBP_DramaticBar")
    wt = unreal.find_object(wb, "WidgetTree")

    root = unreal.find_object(wt, "Root")
    ghost = unreal.find_object(wt, "GhostBar")
    main = unreal.find_object(wt, "MainBar")
    frame = unreal.find_object(wt, "BarFrame")
    fin_l = unreal.find_object(wt, "FinialLeft")
    fin_r = unreal.find_object(wt, "FinialRight")

    set_canvas_slot(root, (0, 0), (1, 1), (0, 0), (0, 0), (0, 0), z=1)
    mask = f"{TEX_ROOT}/Bars/T_UI_Bar_Health_FillMask"
    style_progress_bar(ghost, mask, "BloodDried", 1.0, bg_token="Charcoal", bg_alpha=0.90)
    style_progress_bar(main, mask, "Blood", 1.0, bg_token=None)
    set_overlay_fill(ghost)
    set_overlay_fill(main)

    frame.set_editor_property(
        "brush",
        make_brush(
            f"{TEX_ROOT}/Bars/T_UI_Bar_Health_Frame",
            "Border",
            "Bone",
            0.95,
            margin=(0.07031, 0.34375, 0.07031, 0.34375),
        ),
    )
    frame.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    set_overlay_fill(frame)

    fin_l.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Bars/T_UI_Bar_Finial_Simple_L", "Image", "Bone", 0.92, image_size=(56, 38)))
    fin_l.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    set_canvas_slot(fin_l, (0.0, 0.5), (0.0, 0.5), (-40, 0), (56, 38), (0.0, 0.5), z=2)

    fin_r.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Bars/T_UI_Bar_Finial_Simple_R", "Image", "Bone", 0.92, image_size=(56, 38)))
    fin_r.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    set_canvas_slot(fin_r, (1.0, 0.5), (1.0, 0.5), (40, 0), (56, 38), (1.0, 0.5), z=2)

    cdo = unreal.get_default_object(wb.generated_class())
    cdo.set_editor_property("flash_color", lin("BloodBright", 1.0))
    cdo.set_editor_property("shake_intensity", 3.0)
    cdo.set_editor_property("ghost_delay", 0.22)
    cdo.set_editor_property("ghost_speed", 2.4)

    finish_wb(wb)
    print("Restyled WBP_DramaticBar")


# -----------------------------------------------------------------------------
# 2. WBP_DramaticBar_Energy (Hairline Spectral Energy / Shield Bar)
# -----------------------------------------------------------------------------
def restyle_dramatic_bar_energy():
    wb = unreal.load_asset("/Game/Blueprints/HUD/Common/WBP_DramaticBar_Energy")
    wt = unreal.find_object(wb, "WidgetTree")

    root = unreal.find_object(wt, "Root")
    ghost = unreal.find_object(wt, "GhostBar")
    main = unreal.find_object(wt, "MainBar")
    frame = unreal.find_object(wt, "BarFrame")

    set_canvas_slot(root, (0, 0), (1, 1), (0, 0), (0, 0), (0, 0), z=1)
    mask = f"{TEX_ROOT}/Bars/T_UI_Bar_Shield_FillMask"
    style_progress_bar(ghost, mask, "Spectral", 0.45, bg_token="Charcoal", bg_alpha=0.85)
    style_progress_bar(main, mask, "Spectral", 1.0, bg_token=None)
    set_overlay_fill(ghost)
    set_overlay_fill(main)

    frame.set_editor_property(
        "brush",
        make_brush(
            f"{TEX_ROOT}/Bars/T_UI_Bar_Shield_Frame",
            "Border",
            "BoneMuted",
            0.90,
            margin=(0.05469, 0.4375, 0.05469, 0.4375),
        ),
    )
    frame.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    set_overlay_fill(frame)

    cdo = unreal.get_default_object(wb.generated_class())
    cdo.set_editor_property("flash_color", lin("Bone", 1.0))
    cdo.set_editor_property("shake_intensity", 1.5)
    cdo.set_editor_property("low_threshold", 0.0)

    finish_wb(wb)
    print("Restyled WBP_DramaticBar_Energy")


# -----------------------------------------------------------------------------
# 3. WBP_NewPlayerVitals
# -----------------------------------------------------------------------------
def restyle_new_player_vitals():
    wb = unreal.load_asset("/Game/Blueprints/HUD/WBP_NewPlayerVitals")
    wt = unreal.find_object(wb, "WidgetTree")

    sb_hp = unreal.find_object(wt, "SizeBox_0")
    sb_en = unreal.find_object(wt, "SizeBox")
    sb_hp.set_width_override(440.0)
    sb_hp.set_height_override(28.0)
    if isinstance(sb_hp.slot, unreal.VerticalBoxSlot):
        sb_hp.slot.set_padding(unreal.Margin(40.0, 0.0, 40.0, 6.0))
        sb_hp.slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_LEFT)

    sb_en.set_width_override(340.0)
    sb_en.set_height_override(14.0)
    if isinstance(sb_en.slot, unreal.VerticalBoxSlot):
        sb_en.slot.set_padding(unreal.Margin(44.0, 0.0, 0.0, 0.0))
        sb_en.slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_LEFT)

    finish_wb(wb)
    print("Restyled WBP_NewPlayerVitals")


# -----------------------------------------------------------------------------
# 4. WBP_PlayerVitals (used by WBP_PlayerHUD.WBP_PlayerHealthBar)
# -----------------------------------------------------------------------------
def restyle_player_vitals():
    wb = unreal.load_asset("/Game/Blueprints/HUD/WBP_PlayerVitals")
    wt = unreal.find_object(wb, "WidgetTree")

    sb_en = unreal.find_object(wt, "SizeBox_102")
    en_bar = unreal.find_object(wt, "EnergyBar")
    hp_img = unreal.find_object(wt, "HealthBarImage")
    sb_hp = unreal.find_object(wt, "SizeBox_234")
    hp_bar = unreal.find_object(wt, "HealthBar")
    en_frame = unreal.find_object(wt, "EnergyBarFrame")
    fin_l = unreal.find_object(wt, "FinialLeft")
    fin_r = unreal.find_object(wt, "FinialRight")

    # Health Bar (top-left inside widget, 440x28 at x=46, y=14)
    sb_hp.set_width_override(440.0)
    sb_hp.set_height_override(28.0)
    set_canvas_slot(sb_hp, (0, 0), (0, 0), (46, 14), (440, 28), (0, 0), z=1)
    style_progress_bar(hp_bar, f"{TEX_ROOT}/Bars/T_UI_Bar_Health_FillMask", "Blood", 1.0, "Charcoal", 0.90)

    hp_img.set_editor_property(
        "brush",
        make_brush(
            f"{TEX_ROOT}/Bars/T_UI_Bar_Health_Frame",
            "Border",
            "Bone",
            0.95,
            margin=(0.07031, 0.34375, 0.07031, 0.34375),
        ),
    )
    hp_img.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    set_canvas_slot(hp_img, (0, 0), (0, 0), (46, 14), (440, 28), (0, 0), z=2)

    fin_l.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Bars/T_UI_Bar_Finial_Simple_L", "Image", "Bone", 0.92, image_size=(56, 38)))
    fin_l.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    set_canvas_slot(fin_l, (0, 0), (0, 0), (4, 9), (56, 38), (0, 0), z=3)

    fin_r.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Bars/T_UI_Bar_Finial_Simple_R", "Image", "Bone", 0.92, image_size=(56, 38)))
    fin_r.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    set_canvas_slot(fin_r, (0, 0), (0, 0), (472, 9), (56, 38), (0, 0), z=3)

    # Energy / Spectral Bar cleanly beneath Health Bar (340x14 at x=52, y=48)
    sb_en.set_width_override(340.0)
    sb_en.set_height_override(14.0)
    set_canvas_slot(sb_en, (0, 0), (0, 0), (52, 48), (340, 14), (0, 0), z=1)
    style_progress_bar(en_bar, f"{TEX_ROOT}/Bars/T_UI_Bar_Shield_FillMask", "Spectral", 1.0, "Charcoal", 0.85)

    en_frame.set_editor_property(
        "brush",
        make_brush(
            f"{TEX_ROOT}/Bars/T_UI_Bar_Shield_Frame",
            "Border",
            "BoneMuted",
            0.90,
            margin=(0.05469, 0.4375, 0.05469, 0.4375),
        ),
    )
    en_frame.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    set_canvas_slot(en_frame, (0, 0), (0, 0), (52, 48), (340, 14), (0, 0), z=2)

    finish_wb(wb)
    print("Restyled WBP_PlayerVitals")


# -----------------------------------------------------------------------------
# 5. WBP_PlayerHUD (64px safe-zone positioning & typography)
# -----------------------------------------------------------------------------
def restyle_player_hud():
    wb = unreal.load_asset("/Game/Blueprints/HUD/WBP_PlayerHUD")
    wt = unreal.find_object(wb, "WidgetTree")

    vitals_main = unreal.find_object(wt, "WBP_PlayerHealthBar")
    vitals_new = unreal.find_object(wt, "WBP_NewPlayerVitals")
    vbox_status = unreal.find_object(wt, "VerticalBox_0")
    txt_state = unreal.find_object(wt, "CommonTextBlock_PlayerState")
    txt_poss = unreal.find_object(wt, "CommonTextBlock_PlayerPossession")
    reticle_img = unreal.find_object(wt, "Image_51")
    reticle_sb = unreal.find_object(wt, "SizeBox_0")

    # 64px safe zone top-left for primary vitals
    set_canvas_slot(vitals_main, (0, 0), (0, 0), (64, 52), (540, 80), (0, 0), z=1)
    # 64px safe zone bottom-right for secondary dramatic vitals (if shown)
    set_canvas_slot(vitals_new, (1, 1), (1, 1), (-64, -64), (520, 56), (1, 1), z=1)
    # 64px safe zone top-right for state/possession labels
    set_canvas_slot(vbox_status, (1, 0), (1, 0), (-64, 56), (420, 80), (1, 0), z=1)

    for tb in (txt_state, txt_poss):
        tb.set_editor_property("style", style_text_cls("TS_Label"))
        tb.set_editor_property("justification", unreal.TextJustify.RIGHT)

    # Minimal subtle center reticle diamond
    reticle_sb.set_width_override(8.0)
    reticle_sb.set_height_override(8.0)
    reticle_img.set_editor_property(
        "brush",
        make_brush(f"{TEX_ROOT}/Ornaments/T_UI_Marker_Diamond_Solid", "Image", "Bone", 0.55, image_size=(8, 8)),
    )

    finish_wb(wb)
    print("Restyled WBP_PlayerHUD")


# -----------------------------------------------------------------------------
# 6. WBP_PaladinBossHealthBar (Boss M02 Masterpiece)
# -----------------------------------------------------------------------------
def restyle_boss_health_bar():
    wb = unreal.load_asset("/Game/Blueprints/HUD/WBP_PaladinBossHealthBar")
    wt = unreal.find_object(wb, "WidgetTree")

    crest = unreal.find_object(wt, "Image_0")
    boss_name = unreal.find_object(wt, "BossNameText")
    shield_bar = unreal.find_object(wt, "ShieldBar")
    shield_frame = unreal.find_object(wt, "ShieldBarFrame")
    health_bar = unreal.find_object(wt, "HealthBar")
    boss_frame = unreal.find_object(wt, "Image_1")
    fin_l = unreal.find_object(wt, "FinialLeft")
    fin_r = unreal.find_object(wt, "FinialRight")
    pm1 = unreal.find_object(wt, "PhaseMarker1")
    pm2 = unreal.find_object(wt, "PhaseMarker2")

    # Anchored Bottom-Center (0.5, 1.0), 64px safe margin from bottom
    # Boss Crest centered above Boss Name
    crest.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Ornaments/T_UI_Orn_Crest_Boss", "Image", "Bone", 0.95, image_size=(640, 90)))
    crest.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    set_canvas_slot(crest, (0.5, 1.0), (0.5, 1.0), (0, -172), (640, 90), (0.5, 0.5), z=2)

    # Boss Name ("EL PALADÍN CAÍDO")
    boss_name.set_editor_property("style", style_text_cls("TS_BossName"))
    boss_name.set_text("EL PALADÍN CAÍDO")
    boss_name.set_editor_property("justification", unreal.TextJustify.CENTER)
    set_canvas_slot(boss_name, (0.5, 1.0), (0.5, 1.0), (0, -124), (800, 42), (0.5, 0.5), z=3)

    # Shield Bar (thin silver hairline above boss health)
    style_progress_bar(shield_bar, f"{TEX_ROOT}/Bars/T_UI_Bar_Shield_FillMask", "Shield", 0.95, "Charcoal", 0.70)
    set_canvas_slot(shield_bar, (0.5, 1.0), (0.5, 1.0), (0, -98), (940, 12), (0.5, 0.5), z=1)

    shield_frame.set_editor_property(
        "brush",
        make_brush(
            f"{TEX_ROOT}/Bars/T_UI_Bar_Shield_Frame",
            "Border",
            "BoneMuted",
            0.85,
            margin=(0.05469, 0.4375, 0.05469, 0.4375),
        ),
    )
    shield_frame.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    set_canvas_slot(shield_frame, (0.5, 1.0), (0.5, 1.0), (0, -98), (940, 12), (0.5, 0.5), z=2)

    # Boss Health Bar (1040px wide, deep Blood fill over Charcoal)
    style_progress_bar(health_bar, f"{TEX_ROOT}/Bars/T_UI_Bar_Boss_FillMask", "Blood", 1.0, "Charcoal", 0.90)
    set_canvas_slot(health_bar, (0.5, 1.0), (0.5, 1.0), (0, -72), (1040, 32), (0.5, 0.5), z=1)

    boss_frame.set_editor_property(
        "brush",
        make_brush(
            f"{TEX_ROOT}/Bars/T_UI_Bar_Boss_Frame",
            "Border",
            "Bone",
            0.95,
            margin=(0.07161, 0.3625, 0.07161, 0.3625),
        ),
    )
    boss_frame.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    set_canvas_slot(boss_frame, (0.5, 1.0), (0.5, 1.0), (0, -72), (1040, 36), (0.5, 0.5), z=3)

    # Ornate Boss Bar Finials (Asset_01)
    fin_l.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Bars/T_UI_Bar_Finial_Boss_L", "Image", "Bone", 0.95, image_size=(88, 44)))
    fin_l.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    set_canvas_slot(fin_l, (0.5, 1.0), (0.5, 1.0), (-550, -72), (88, 44), (0.5, 0.5), z=4)

    fin_r.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Bars/T_UI_Bar_Finial_Boss_R", "Image", "Bone", 0.95, image_size=(88, 44)))
    fin_r.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    set_canvas_slot(fin_r, (0.5, 1.0), (0.5, 1.0), (550, -72), (88, 44), (0.5, 0.5), z=4)

    # Phase threshold diamond markers at 33% and 66% along the 1040px bar
    for pm, dx in ((pm1, -173), (pm2, 173)):
        pm.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Bars/T_UI_Bar_PhaseMarker", "Image", "Bone", 0.95, image_size=(24, 24)))
        pm.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
        set_canvas_slot(pm, (0.5, 1.0), (0.5, 1.0), (dx, -72), (24, 24), (0.5, 0.5), z=4)

    finish_wb(wb)
    print("Restyled WBP_PaladinBossHealthBar")


if __name__ == "__main__":
    restyle_dramatic_bar()
    restyle_dramatic_bar_energy()
    restyle_new_player_vitals()
    restyle_player_vitals()
    restyle_player_hud()
    restyle_boss_health_bar()
    print("=== PHASE 3 HUD & BOSS COMPLETE ===")
