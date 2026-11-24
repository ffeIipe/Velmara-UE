"""
Velmara UI — Phase 3 Polish Pass
Fixes all visual details caught by live 1920x1080 Unreal widget screenshots:
  1. Exact 0px bar frame alignment (DrawAs=Image matching FillMask rect) on
     WBP_DramaticBar, WBP_DramaticBar_Energy, WBP_PlayerVitals, WBP_PaladinBossHealthBar
  2. WBP_PlayerHUD: moves active WBP_NewPlayerVitals cleanly to Top-Left (64, 52) per M01
  3. WBP_OptionsMenu: 58px horizontal row padding (zero '<' chevron overlap),
     clean 'GRÁFICOS' right panel header + quote padding (zero L-bracket clipping)
  4. Subtle obsidian BloodDried vignette (0.13 alpha) on Options, Pause & LoadingScreen
  5. Removes CommonActionWidget white-box progress_material_brush fallback & sets design_time_key
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


def make_brush(tex_path=None, draw_as="Image", tint="White", alpha=1.0, margin=None, image_size=None):
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


def fix_action_widget(aw, design_key_name: str):
    # Remove default white square progress_material_brush fallback
    aw.set_editor_property("progress_material_brush", make_brush(None, "NoDrawType"))
    aw.set_editor_property("icon_rim_brush", make_brush(None, "NoDrawType"))
    k = unreal.Key()
    k.import_text(design_key_name)
    aw.set_editor_property("design_time_key", k)


def finish_bp(bp):
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    eal.save_loaded_asset(bp, only_if_is_dirty=False)


# 1. Exact 0px bar frame alignment (DrawAs=Image matching FillMask canvas 1:1)
def polish_bars_and_hud():
    # WBP_DramaticBar
    wb = unreal.load_asset("/Game/Blueprints/HUD/Common/WBP_DramaticBar")
    wt = unreal.find_object(wb, "WidgetTree")
    frame = unreal.find_object(wt, "BarFrame")
    frame.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Bars/T_UI_Bar_Health_Frame", "Image", "Bone", 0.96))
    fin_l = unreal.find_object(wt, "FinialLeft")
    fin_r = unreal.find_object(wt, "FinialRight")
    set_canvas_slot(fin_l, (0.0, 0.5), (0.0, 0.5), (-38, 0), (54, 36), (0.0, 0.5), z=2)
    set_canvas_slot(fin_r, (1.0, 0.5), (1.0, 0.5), (38, 0), (54, 36), (1.0, 0.5), z=2)
    finish_bp(wb)

    # WBP_DramaticBar_Energy
    wb = unreal.load_asset("/Game/Blueprints/HUD/Common/WBP_DramaticBar_Energy")
    wt = unreal.find_object(wb, "WidgetTree")
    frame = unreal.find_object(wt, "BarFrame")
    frame.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Bars/T_UI_Bar_Shield_Frame", "Image", "BoneMuted", 0.92))
    finish_bp(wb)

    # WBP_NewPlayerVitals
    wb = unreal.load_asset("/Game/Blueprints/HUD/WBP_NewPlayerVitals")
    wt = unreal.find_object(wb, "WidgetTree")
    sb_hp = unreal.find_object(wt, "SizeBox_0")
    sb_en = unreal.find_object(wt, "SizeBox")
    sb_hp.set_width_override(440.0)
    sb_hp.set_height_override(30.0)
    sb_en.set_width_override(320.0)
    sb_en.set_height_override(14.0)
    finish_bp(wb)

    # WBP_PlayerVitals
    wb = unreal.load_asset("/Game/Blueprints/HUD/WBP_PlayerVitals")
    wt = unreal.find_object(wb, "WidgetTree")
    hp_img = unreal.find_object(wt, "HealthBarImage")
    hp_img.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Bars/T_UI_Bar_Health_Frame", "Image", "Bone", 0.96))
    en_frame = unreal.find_object(wt, "EnergyBarFrame")
    en_frame.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Bars/T_UI_Bar_Shield_Frame", "Image", "BoneMuted", 0.92))
    finish_bp(wb)

    # WBP_PlayerHUD: Move active WBP_NewPlayerVitals cleanly to Top-Left (64, 52) per M01!
    wb = unreal.load_asset("/Game/Blueprints/HUD/WBP_PlayerHUD")
    wt = unreal.find_object(wb, "WidgetTree")
    vitals_new = unreal.find_object(wt, "WBP_NewPlayerVitals")
    set_canvas_slot(vitals_new, (0, 0), (0, 0), (64, 52), (520, 60), (0, 0), z=2)
    finish_bp(wb)

    # WBP_PaladinBossHealthBar: exact 1:1 Image rect match between FillMask and Frame!
    wb = unreal.load_asset("/Game/Blueprints/HUD/WBP_PaladinBossHealthBar")
    wt = unreal.find_object(wb, "WidgetTree")
    shield_bar = unreal.find_object(wt, "ShieldBar")
    shield_frame = unreal.find_object(wt, "ShieldBarFrame")
    health_bar = unreal.find_object(wt, "HealthBar")
    boss_frame = unreal.find_object(wt, "Image_1")
    fin_l = unreal.find_object(wt, "FinialLeft")
    fin_r = unreal.find_object(wt, "FinialRight")
    pm1 = unreal.find_object(wt, "PhaseMarker1")
    pm2 = unreal.find_object(wt, "PhaseMarker2")

    set_canvas_slot(shield_bar, (0.5, 1.0), (0.5, 1.0), (0, -104), (940, 16), (0.5, 0.5), z=1)
    shield_frame.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Bars/T_UI_Bar_Shield_Frame", "Image", "BoneMuted", 0.90))
    set_canvas_slot(shield_frame, (0.5, 1.0), (0.5, 1.0), (0, -104), (940, 16), (0.5, 0.5), z=2)

    set_canvas_slot(health_bar, (0.5, 1.0), (0.5, 1.0), (0, -72), (1040, 44), (0.5, 0.5), z=1)
    boss_frame.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Bars/T_UI_Bar_Boss_Frame", "Image", "Bone", 0.96))
    set_canvas_slot(boss_frame, (0.5, 1.0), (0.5, 1.0), (0, -72), (1040, 44), (0.5, 0.5), z=3)

    set_canvas_slot(fin_l, (0.5, 1.0), (0.5, 1.0), (-548, -72), (88, 44), (0.5, 0.5), z=4)
    set_canvas_slot(fin_r, (0.5, 1.0), (0.5, 1.0), (548, -72), (88, 44), (0.5, 0.5), z=4)
    set_canvas_slot(pm1, (0.5, 1.0), (0.5, 1.0), (-173, -72), (26, 26), (0.5, 0.5), z=4)
    set_canvas_slot(pm2, (0.5, 1.0), (0.5, 1.0), (173, -72), (26, 26), (0.5, 0.5), z=4)
    finish_bp(wb)
    print("Polished HUD & Boss bars (0px alignment + Top-Left M01 vitals)")


# 2. Polish WBP_OptionsMenu
def polish_options_menu():
    wb = unreal.load_asset("/Game/Blueprints/HUD/Menus/WBP_OptionsMenu")
    wt = unreal.find_object(wb, "WidgetTree")

    # Subtle obsidian vignette instead of heavy red corners
    dim = unreal.find_object(wt, "DimOverlay")
    dim.set_editor_property("brush", make_brush(None, "Box", "Void", 0.88))
    vig = unreal.find_object(wt, "VignetteOverlay")
    vig.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Gradients/T_UI_Grad_Vignette_Radial", "Image", "BloodDried", 0.13))

    # Wider horizontal padding (58px) inside RowFrame_* so text never touches '<' or '>' chevrons
    row_names = [
        "Resolution", "TextureQuality", "ShadowQuality", "AntiAliasing",
        "ViewDistance", "VisualEffects", "PostProcessing", "GlobalIllumination",
        "Shading", "Gamma", "MaxFPS", "Lumen",
    ]
    for rname in row_names:
        rf = unreal.find_object(wt, f"RowFrame_{rname}")
        rf.set_padding(unreal.Margin(58.0, 4.0, 58.0, 4.0))

    # Fix Right Description Panel title & quote margins (zero L-bracket overlap!)
    d_box = unreal.find_object(wt, "DescPanelContent")
    if isinstance(d_box.slot, unreal.BorderSlot):
        d_box.slot.set_padding(unreal.Margin(46, 44, 46, 44))
    d_title = unreal.find_object(wt, "DescTitle")
    d_title.set_text("GRÁFICOS")
    d_sep = unreal.find_object(wt, "DescSep")
    d_sep.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Ornaments/T_UI_Orn_Sep_Diamond", "Image", "Bone", 0.90, image_size=(480, 20)))
    d_body = unreal.find_object(wt, "DescBody")
    d_body.set_editor_property("wrap_text_at", 490.0)
    d_quote = unreal.find_object(wt, "DescQuote")
    d_quote.set_editor_property("wrap_text_at", 490.0)
    if isinstance(d_quote.slot, unreal.VerticalBoxSlot):
        d_quote.slot.set_padding(unreal.Margin(0, 0, 0, 8))

    # Fix CommonActionWidget white-box fallback
    fix_action_widget(unreal.find_object(wt, "ActionConfirmIcon"), "Enter")
    fix_action_widget(unreal.find_object(wt, "ActionBackIcon"), "Escape")
    fix_action_widget(unreal.find_object(wt, "ActionResetIcon"), "R")

    finish_bp(wb)
    print("Polished WBP_OptionsMenu")


# 3. Polish WBP_PauseMenu, WBP_TitleScreen & Loading Screens
def polish_pause_title_loading():
    # Pause
    wb = unreal.load_asset("/Game/Blueprints/HUD/Menus/WBP_PauseMenu")
    wt = unreal.find_object(wb, "WidgetTree")
    unreal.find_object(wt, "DimOverlay").set_editor_property("brush", make_brush(None, "Box", "Void", 0.78))
    unreal.find_object(wt, "VignetteOverlay").set_editor_property(
        "brush", make_brush(f"{TEX_ROOT}/Gradients/T_UI_Grad_Vignette_Radial", "Image", "BloodDried", 0.13)
    )
    fix_action_widget(unreal.find_object(wt, "ActionConfirmIcon"), "Enter")
    fix_action_widget(unreal.find_object(wt, "ActionBackIcon"), "Escape")
    finish_bp(wb)

    # TitleScreen
    wb = unreal.load_asset("/Game/Blueprints/HUD/Menus/WBP_TitleScreen")
    wt = unreal.find_object(wb, "WidgetTree")
    fix_action_widget(unreal.find_object(wt, "ActionConfirmIcon"), "Enter")
    finish_bp(wb)

    # Loading screens: subtler obsidian vignette
    for lp in ("/Game/Blueprints/HUD/Menus/WBP_LoadingScreen", "/Game/Blueprints/HUD/WBP_VelmaraBootLoading"):
        wb = unreal.load_asset(lp)
        wt = unreal.find_object(wb, "WidgetTree")
        unreal.find_object(wt, "VignetteOverlay").set_editor_property(
            "brush", make_brush(f"{TEX_ROOT}/Gradients/T_UI_Grad_Vignette_Radial", "Image", "BloodDried", 0.14)
        )
        finish_bp(wb)
    print("Polished Pause, TitleScreen, and LoadingScreens")


if __name__ == "__main__":
    polish_bars_and_hud()
    polish_options_menu()
    polish_pause_title_loading()
    print("=== PHASE 3 POLISH COMPLETE ===")
