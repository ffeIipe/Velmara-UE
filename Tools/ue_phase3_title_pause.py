"""
Velmara UI — Phase 3 Step C: TitleScreen, MainMenu & PauseMenu Redesign
Faithful to Mockup_Ajustes visual grammar & UI_Velmara_PromptKit (M05 / M06).
Preserves 100% of existing variable bindings and OnClicked event graphs.
"""

import unreal

eal = unreal.EditorAssetLibrary
TEX_ROOT = "/Game/UI/Textures"
STYLES_DIR = "/Game/Blueprints/HUD/Styles"
INPUT_DT_PATH = "/Game/Blueprints/HUD/InputPrompts/DT_VelmaraInputActions"

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


def bind_action_widget(aw, row_name: str):
    dt = unreal.load_asset(INPUT_DT_PATH)
    h = unreal.DataTableRowHandle()
    h.set_editor_property("data_table", dt)
    h.set_editor_property("row_name", row_name)
    aw.set_editor_property("input_actions", [h])


def finish_bp(bp):
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    eal.save_loaded_asset(bp, only_if_is_dirty=False)


# -----------------------------------------------------------------------------
# 1. WBP_TitleScreen (Main Menu Host Screen)
# -----------------------------------------------------------------------------
def restyle_title_screen():
    wb = unreal.load_asset("/Game/Blueprints/HUD/Menus/WBP_TitleScreen")
    wt = unreal.find_object(wb, "WidgetTree")

    bg = unreal.find_object(wt, "Image_63")
    scrim = unreal.find_object(wt, "LeftScrim")
    sline = unreal.find_object(wt, "SidebarLineV")
    logo = unreal.find_object(wt, "TitleLogo")
    tsep = unreal.find_object(wt, "TitleSeparator")
    mpanel = unreal.find_object(wt, "MenuPanel")
    mframe = unreal.find_object(wt, "MenuPanelFrame")
    stack = unreal.find_object(wt, "WidgetStack")
    emblem = unreal.find_object(wt, "TopRightEmblem")
    tagline = unreal.find_object(wt, "TaglineText")
    hints = unreal.find_object(wt, "InputHintsBox")
    act_icon = unreal.find_object(wt, "ActionConfirmIcon")
    act_lbl = unreal.find_object(wt, "ActionConfirmLabel")

    # Fullscreen Blood-Moon Cathedral Key Art (16:9 clean, zero letterbox)
    bg.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Backgrounds/T_UI_Bg_MainMenu", "Image", "White", 1.0))
    set_canvas_slot(bg, (0, 0), (1, 1), (0, 0), (0, 0), (0, 0), z=0)

    # Left dark translucent fade scrim so left-aligned menu reads razor-sharp
    scrim.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Gradients/T_UI_Grad_HFade_LeftToRight", "Image", "Void", 0.88))
    scrim.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    set_canvas_slot(scrim, (0, 0), (0.58, 1), (0, 0), (0, 0), (0, 0), z=1)

    # Left vertical signature gothic star line (Mockup_Ajustes language)
    sline.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Ornaments/T_UI_Orn_SidebarLine_V", "Image", "Bone", 0.78, image_size=(26, 720)))
    sline.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    set_canvas_slot(sline, (0, 0.5), (0, 0.5), (48, 0), (26, 720), (0.5, 0.5), z=2)

    # Logo top-left + Star Separator underneath
    set_canvas_slot(logo, (0, 0), (0, 0), (88, 72), (480, 140), (0, 0), z=3)
    tsep.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Ornaments/T_UI_Orn_Sep_Star", "Image", "Bone", 0.90, image_size=(460, 46)))
    tsep.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    set_canvas_slot(tsep, (0, 0), (0, 0), (84, 212), (460, 46), (0, 0), z=3)

    # Refined left-aligned minimal bracket frame around the menu list
    mpanel.set_editor_property("style", style_cls("BS_Panel_Void80"))
    mpanel.set_render_opacity(0.45)
    mframe.set_editor_property("style", style_cls("BS_Panel_BracketsMinimal"))
    set_canvas_slot(mpanel, (0, 0.5), (0, 0.5), (88, 60), (460, 440), (0, 0.5), z=2)

    # WidgetStack aligned cleanly inside the left menu zone
    set_canvas_slot(stack, (0, 0.5), (0, 0.5), (104, 60), (428, 400), (0, 0.5), z=4)

    # Top-right Velmara Crescent Emblem (Mockup_Ajustes signature)
    emblem.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Ornaments/T_UI_Orn_Crest_Velmara", "Image", "Bone", 0.90, image_size=(48, 144)))
    emblem.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    set_canvas_slot(emblem, (1, 0), (1, 0), (-64, 48), (48, 144), (1, 0), z=3)

    # Bottom-left tagline
    tagline.set_editor_property("style", style_cls("TS_Label"))
    tagline.set_text("ALGUNAS COSAS  |  NUNCA MUEREN")
    set_canvas_slot(tagline, (0, 1), (0, 1), (88, -64), (480, 32), (0, 1), z=3, auto_size=True)

    # Bottom-right dynamic CommonActionWidget prompt
    set_canvas_slot(hints, (1, 1), (1, 1), (-64, -64), (240, 40), (1, 1), z=3, auto_size=True)
    bind_action_widget(act_icon, "Confirm")
    if isinstance(act_icon.slot, unreal.HorizontalBoxSlot):
        act_icon.slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)
        act_icon.slot.set_padding(unreal.Margin(0, 0, 10, 0))
    act_lbl.set_editor_property("style", style_cls("TS_Label"))
    act_lbl.set_text("SELECCIONAR")
    if isinstance(act_lbl.slot, unreal.HorizontalBoxSlot):
        act_lbl.slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)

    finish_bp(wb)
    print("Restyled WBP_TitleScreen")


# -----------------------------------------------------------------------------
# 2. WBP_MainMenu (Left-aligned Blood-Fade Gothic Menu List)
# -----------------------------------------------------------------------------
def restyle_main_menu():
    wb = unreal.load_asset("/Game/Blueprints/HUD/Menus/WBP_MainMenu")
    wt = unreal.find_object(wb, "WidgetTree")

    cinzel_font = unreal.load_asset("/Game/Fonts/Cinzel_Font")
    font_info = unreal.SlateFontInfo()
    font_info.set_editor_property("font_object", cinzel_font)
    font_info.set_editor_property("typeface_font_name", "Regular")
    font_info.set_editor_property("size", 22.0)
    font_info.set_editor_property("letter_spacing", 160)

    grad_ltr = f"{TEX_ROOT}/Gradients/T_UI_Grad_HFade_LeftToRight"
    btn_style = unreal.ButtonStyle()
    btn_style.set_editor_property("normal", make_brush(None, "NoDrawType"))
    btn_style.set_editor_property("hovered", make_brush(grad_ltr, "Image", "Blood", 0.78))
    btn_style.set_editor_property("pressed", make_brush(grad_ltr, "Image", "BloodBright", 0.92))
    btn_style.set_editor_property("disabled", make_brush(None, "NoDrawType"))
    btn_style.set_editor_property("normal_padding", unreal.Margin(24.0, 14.0, 24.0, 14.0))
    btn_style.set_editor_property("pressed_padding", unreal.Margin(28.0, 14.0, 20.0, 14.0))

    items = [
        ("NewGameButton", "NewGameText", "NUEVA PARTIDA"),
        ("LoadGameButton", "LoadGameText", "CARGAR PARTIDA"),
        ("PracticeMap", "LoadGameText_1", "ZONA DE PRÁCTICA"),
        ("OptionsButton", "OptionsText", "AJUSTES"),
        ("QuitButton", "QuitText", "SALIR"),
    ]
    for bname, tname, label_es in items:
        btn = unreal.find_object(wt, bname)
        txt = unreal.find_object(wt, tname)
        btn.set_editor_property("widget_style", btn_style)
        if isinstance(btn.slot, unreal.VerticalBoxSlot):
            btn.slot.set_padding(unreal.Margin(0.0, 4.0, 0.0, 4.0))
            btn.slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
        txt.set_text(label_es)
        txt.set_editor_property("font", font_info)
        txt.set_editor_property("color_and_opacity", slate_col("Bone", 0.92))
        txt.set_editor_property("justification", unreal.TextJustify.LEFT)
        if isinstance(txt.slot, unreal.ButtonSlot):
            txt.slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_LEFT)
            txt.slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)

    finish_bp(wb)
    print("Restyled WBP_MainMenu")


# -----------------------------------------------------------------------------
# 3. WBP_PauseMenu (Mockup_Ajustes L-Bracket Panel + Blur + Dynamic Prompts)
# -----------------------------------------------------------------------------
def restyle_pause_menu():
    wb = unreal.load_asset("/Game/Blueprints/HUD/Menus/WBP_PauseMenu")
    wt = unreal.find_object(wb, "WidgetTree")

    blur = unreal.find_object(wt, "BackgroundBlur_187")
    dim = unreal.find_object(wt, "DimOverlay")
    vig = unreal.find_object(wt, "VignetteOverlay")
    sb_panel = unreal.find_object(wt, "SizeBox_3")
    p_panel = unreal.find_object(wt, "PausePanel")
    p_frame = unreal.find_object(wt, "PausePanelFrame")
    sb_btns = unreal.find_object(wt, "SizeBox_4")
    title = unreal.find_object(wt, "CommonTextBlock_0")
    tsep = unreal.find_object(wt, "TitleSeparator")
    spacer = unreal.find_object(wt, "Spacer_152")
    hints = unreal.find_object(wt, "InputHintsBox")
    act_c_ic = unreal.find_object(wt, "ActionConfirmIcon")
    act_c_lb = unreal.find_object(wt, "ActionConfirmLabel")
    act_b_ic = unreal.find_object(wt, "ActionBackIcon")
    act_b_lb = unreal.find_object(wt, "ActionBackLabel")

    blur.set_blur_strength(8.0)
    set_canvas_slot(blur, (0, 0), (1, 1), (0, 0), (0, 0), (0, 0), z=0)

    dim.set_editor_property("brush", make_brush(None, "Box", "Void", 0.62))
    dim.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    set_canvas_slot(dim, (0, 0), (1, 1), (0, 0), (0, 0), (0, 0), z=1)

    vig.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Gradients/T_UI_Grad_Vignette_Radial", "Image", "BloodDried", 0.28))
    vig.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    set_canvas_slot(vig, (0, 0), (1, 1), (0, 0), (0, 0), (0, 0), z=1)

    set_canvas_slot(sb_panel, (0.5, 0.5), (0.5, 0.5), (0, 0), (540, 560), (0.5, 0.5), z=2)
    p_panel.set_editor_property("style", style_cls("BS_Panel_Void80"))
    p_frame.set_editor_property("style", style_cls("BS_Panel_Brackets"))

    set_canvas_slot(sb_btns, (0.5, 0.5), (0.5, 0.5), (0, 0), (440, 440), (0.5, 0.5), z=3)
    title.set_editor_property("style", style_cls("TS_H1"))
    title.set_text("PAUSA")
    title.set_editor_property("justification", unreal.TextJustify.CENTER)

    tsep.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Ornaments/T_UI_Orn_Sep_Star", "Image", "Bone", 0.92, image_size=(380, 40)))
    if isinstance(tsep.slot, unreal.VerticalBoxSlot):
        tsep.slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_CENTER)
        tsep.slot.set_padding(unreal.Margin(0, 4, 0, 8))

    spacer.set_size(unreal.Vector2D(1.0, 24.0))

    for sb_name in ("SizeBox_0", "SizeBox_1", "SizeBox_2"):
        sb = unreal.find_object(wt, sb_name)
        sb.set_height_override(56.0)
        if isinstance(sb.slot, unreal.VerticalBoxSlot):
            sb.slot.set_padding(unreal.Margin(0, 6, 0, 6))
            sb.slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)

    # Bottom-right dynamic input prompts (Confirm + Back)
    set_canvas_slot(hints, (1, 1), (1, 1), (-64, -64), (360, 40), (1, 1), z=4, auto_size=True)
    bind_action_widget(act_c_ic, "Confirm")
    bind_action_widget(act_b_ic, "Back")
    for ic, pad in ((act_c_ic, (0, 0, 8, 0)), (act_b_ic, (28, 0, 8, 0))):
        if isinstance(ic.slot, unreal.HorizontalBoxSlot):
            ic.slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)
            ic.slot.set_padding(unreal.Margin(*pad))
    for lb, txt in ((act_c_lb, "SELECCIONAR"), (act_b_lb, "ATRÁS")):
        lb.set_editor_property("style", style_cls("TS_Label"))
        lb.set_text(txt)
        if isinstance(lb.slot, unreal.HorizontalBoxSlot):
            lb.slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)

    finish_bp(wb)
    print("Restyled WBP_PauseMenu")


if __name__ == "__main__":
    restyle_title_screen()
    restyle_main_menu()
    restyle_pause_menu()
    print("=== PHASE 3 TITLE, MAIN MENU & PAUSE COMPLETE ===")
