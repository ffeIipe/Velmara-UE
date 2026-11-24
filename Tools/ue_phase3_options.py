"""
Velmara UI — Phase 3 Step D: WBP_OptionsMenu Redesign (Mockup_Ajustes.png)
3-column AAA gothic settings screen preserving all 12 existing control bindings:
  - Left column: vertical star divider, AJUSTES title + star separator, active red fade category
  - Center column: Top Tabs + BloodBright active underline + 12 T_UI_Frame_OptionRow rows + styled controls
  - Right column: Bold L-Brackets description panel (SettingsPanel + SettingsPanelFrame)
  - Bottom footer: Tagline + dynamic CommonActionWidget prompts (Confirm / Back / Reset)
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
        if image_size:
            b.set_editor_property("image_size", vec2_slate(image_size[0], image_size[1]))
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


def make_cinzel_font(size: float, typeface: str = "Regular", spacing: int = 100) -> unreal.SlateFontInfo:
    f = unreal.SlateFontInfo()
    f.set_editor_property("font_object", unreal.load_asset("/Game/Fonts/Cinzel_Font"))
    f.set_editor_property("typeface_font_name", typeface)
    f.set_editor_property("size", float(size))
    f.set_editor_property("letter_spacing", int(spacing))
    return f


def restyle_options_menu():
    wb = unreal.load_asset("/Game/Blueprints/HUD/Menus/WBP_OptionsMenu")
    wt = unreal.find_object(wb, "WidgetTree")

    # 1. Backdrop (Blur + Void Dim + BloodDried subtle vignette + Top-right emblem)
    blur = unreal.find_object(wt, "BackgroundBlur_0")
    blur.set_blur_strength(10.0)
    set_canvas_slot(blur, (0, 0), (1, 1), (0, 0), (0, 0), (0, 0), z=0)

    dim = unreal.find_object(wt, "DimOverlay")
    dim.set_editor_property("brush", make_brush(None, "Box", "Void", 0.78))
    dim.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    set_canvas_slot(dim, (0, 0), (1, 1), (0, 0), (0, 0), (0, 0), z=1)

    vig = unreal.find_object(wt, "VignetteOverlay")
    vig.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Gradients/T_UI_Grad_Vignette_Radial", "Image", "BloodDried", 0.24))
    vig.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    set_canvas_slot(vig, (0, 0), (1, 1), (0, 0), (0, 0), (0, 0), z=1)

    emblem = unreal.find_object(wt, "TopRightEmblem")
    emblem.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Ornaments/T_UI_Orn_Crest_Velmara", "Image", "Bone", 0.92, image_size=(44, 132)))
    emblem.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    set_canvas_slot(emblem, (1, 0), (1, 0), (-64, 32), (44, 132), (1, 0), z=3)

    # 2. Left Sidebar Column (Mockup_Ajustes: vertical star line + AJUSTES + active red category)
    sline = unreal.find_object(wt, "SidebarLineV")
    sline.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Ornaments/T_UI_Orn_SidebarLine_V", "Image", "Bone", 0.80, image_size=(24, 640)))
    sline.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    set_canvas_slot(sline, (0, 0), (0, 0), (48, 152), (24, 640), (0.5, 0), z=2)

    sidebar = unreal.find_object(wt, "SidebarColumn")
    set_canvas_slot(sidebar, (0, 0), (0, 0), (76, 176), (344, 600), (0, 0), z=2)

    t_aj = unreal.find_object(wt, "TitleAjustes")
    t_aj.set_editor_property("style", style_cls("TS_H1"))
    t_aj.set_text("AJUSTES")
    if isinstance(t_aj.slot, unreal.VerticalBoxSlot):
        t_aj.slot.set_padding(unreal.Margin(4, 0, 0, 4))

    t_sep = unreal.find_object(wt, "TitleSepStar")
    t_sep.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Ornaments/T_UI_Orn_Sep_Star", "Image", "Bone", 0.90, image_size=(340, 38)))
    if isinstance(t_sep.slot, unreal.VerticalBoxSlot):
        t_sep.slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
        t_sep.slot.set_padding(unreal.Margin(0, 0, 0, 20))

    act_ov = unreal.find_object(wt, "ActiveCategoryOverlay")
    if isinstance(act_ov.slot, unreal.VerticalBoxSlot):
        act_ov.slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
        act_ov.slot.set_padding(unreal.Margin(0, 4, 0, 4))

    act_grad = unreal.find_object(wt, "ActiveCatGrad")
    act_grad.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Gradients/T_UI_Grad_HFade_LeftToRight", "Image", "Blood", 0.72, image_size=(344, 54)))
    if isinstance(act_grad.slot, unreal.OverlaySlot):
        act_grad.slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
        act_grad.slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_FILL)

    act_brd = unreal.find_object(wt, "ActiveCatBorder")
    act_brd.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Frames/T_UI_Frame_Sidebar_Active", "Image", "BloodBright", 0.95, image_size=(344, 54)))
    if isinstance(act_brd.slot, unreal.OverlaySlot):
        act_brd.slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
        act_brd.slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_FILL)

    act_row = unreal.find_object(wt, "ActiveCatRow")
    if isinstance(act_row.slot, unreal.OverlaySlot):
        act_row.slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_LEFT)
        act_row.slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)
        act_row.slot.set_padding(unreal.Margin(18, 12, 16, 12))

    act_diam = unreal.find_object(wt, "ActiveCatDiamond")
    act_diam.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Ornaments/T_UI_Marker_Diamond_Hollow", "Image", "BloodBright", 1.0, image_size=(22, 22)))
    if isinstance(act_diam.slot, unreal.HorizontalBoxSlot):
        act_diam.slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)
        act_diam.slot.set_padding(unreal.Margin(0, 0, 16, 0))

    act_lbl = unreal.find_object(wt, "ActiveCatLabel")
    act_lbl.set_editor_property("style", style_cls("TS_Button_Hover"))
    act_lbl.set_text("PANTALLA")
    if isinstance(act_lbl.slot, unreal.HorizontalBoxSlot):
        act_lbl.slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)

    # 3. Center Column: Tabs + Active Underline + ScrollBox with 12 T_UI_Frame_OptionRow rows + Buttons
    vbox = unreal.find_object(wt, "VerticalBox_1")
    set_canvas_slot(vbox, (0, 0), (0, 0), (464, 64), (736, 920), (0, 0), z=3)

    g_tab = unreal.find_object(wt, "GraphicsTab")
    s_tab = unreal.find_object(wt, "SoundTab")
    t_div = unreal.find_object(wt, "TabSeparator")
    h_line = unreal.find_object(wt, "HeaderBloodLine")

    g_tab.set_editor_property("style", style_cls("TS_Tab_Selected"))
    g_tab.set_text("GRÁFICOS")
    s_tab.set_editor_property("style", style_cls("TS_Tab_Normal"))
    s_tab.set_text("SONIDO")
    t_div.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Ornaments/T_UI_Orn_TabDivider", "Image", "BoneMuted", 0.90, image_size=(16, 32)))
    if isinstance(t_div.slot, unreal.HorizontalBoxSlot):
        t_div.slot.set_padding(unreal.Margin(24, 0, 24, 0))
        t_div.slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)

    h_line.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Ornaments/T_UI_Orn_TabActiveLine", "Image", "BloodBright", 0.95, image_size=(700, 24)))
    if isinstance(h_line.slot, unreal.VerticalBoxSlot):
        h_line.slot.set_padding(unreal.Margin(0, 4, 0, 14))
        h_line.slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)

    scroll = unreal.find_object(wt, "ScrollBox_0")
    if isinstance(scroll.slot, unreal.VerticalBoxSlot):
        sz = unreal.SlateChildSize()
        sz.set_editor_property("size_rule", unreal.SlateSizeRule.FILL)
        sz.set_editor_property("value", 1.0)
        scroll.slot.set_size(sz)
        scroll.slot.set_padding(unreal.Margin(0, 0, 0, 16))

    # Row frames + Labels + Controls
    row_frame_brush = make_brush(
        f"{TEX_ROOT}/Frames/T_UI_Frame_OptionRow",
        "Border",
        "BoneMuted",
        0.78,
        margin=(0.10156, 0.125, 0.10156, 0.125),
        image_size=(256, 64),
    )

    row_specs = [
        ("Resolution", "Label_Resolution", "RESOLUCIÓN"),
        ("TextureQuality", "Label_QualityTexture", "CALIDAD DE TEXTURAS"),
        ("ShadowQuality", "Label_ShadowQuality", "CALIDAD DE SOMBRAS"),
        ("AntiAliasing", "Label_AntiAliasing", "ANTIALIASING"),
        ("ViewDistance", "Label_ViewDistance", "DISTANCIA DE VISIÓN"),
        ("VisualEffects", "Label_VisualEffects", "EFECTOS VISUALES"),
        ("PostProcessing", "Label_PostProcessing", "POSPROCESADO"),
        ("GlobalIllumination", "Label_GIllumination", "ILUMINACIÓN GLOBAL"),
        ("Shading", "Label_Shading", "SOMBREADO"),
        ("Gamma", "Label_Gamma", "GAMMA"),
        ("MaxFPS", "Label_MaxFPS", "FPS MÁXIMOS"),
        ("Lumen", "Label_NewLumen", "LUMEN"),
    ]
    for rname, lname, label_es in row_specs:
        rf = unreal.find_object(wt, f"RowFrame_{rname}")
        rf.set_editor_property("background", row_frame_brush)
        rf.set_padding(unreal.Margin(36.0, 6.0, 36.0, 6.0))
        if isinstance(rf.slot, unreal.ScrollBoxSlot):
            rf.slot.set_padding(unreal.Margin(0.0, 4.0, 8.0, 4.0))
            rf.slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)

        sb_row = unreal.find_object(wt, rname)
        sb_row.set_height_override(52.0)

        lbl = unreal.find_object(wt, lname)
        lbl.set_editor_property("style", style_cls("TS_Label"))
        lbl.set_text(label_es)

    # Style all 9 ComboBoxString dropdowns cleanly in Velmara dark gothic palette
    cb_btn_style = unreal.ButtonStyle()
    cb_btn_style.set_editor_property("normal", make_brush(None, "Box", "Void", 0.75))
    cb_btn_style.set_editor_property("hovered", make_brush(f"{TEX_ROOT}/Gradients/T_UI_Grad_HFade_Center", "Image", "Blood", 0.65))
    cb_btn_style.set_editor_property("pressed", make_brush(f"{TEX_ROOT}/Gradients/T_UI_Grad_HFade_Center", "Image", "BloodBright", 0.85))
    cb_btn_style.set_editor_property("disabled", make_brush(None, "Box", "Charcoal", 0.40))
    cb_btn_style.set_editor_property("normal_padding", unreal.Margin(12, 6, 12, 6))
    cb_btn_style.set_editor_property("pressed_padding", unreal.Margin(12, 6, 12, 6))

    cb_btn_wrap = unreal.ComboButtonStyle()
    cb_btn_wrap.set_editor_property("button_style", cb_btn_style)
    cb_btn_wrap.set_editor_property(
        "down_arrow_image",
        make_brush(f"{TEX_ROOT}/Ornaments/T_UI_Orn_Chevron_Down", "Image", "Bone", 0.90, image_size=(16, 12)),
    )
    cb_btn_wrap.set_editor_property("menu_border_brush", make_brush(None, "Box", "Void", 0.95))
    cb_btn_wrap.set_editor_property("menu_border_padding", unreal.Margin(2, 2, 2, 2))

    cb_style = unreal.ComboBoxStyle()
    cb_style.set_editor_property("combo_button_style", cb_btn_wrap)
    cb_style.set_editor_property("content_padding", unreal.Margin(8, 4, 8, 4))

    cb_item_style = unreal.TableRowStyle()
    cb_item_style.set_editor_property("active_brush", make_brush(None, "Box", "Blood", 0.85))
    cb_item_style.set_editor_property("active_hovered_brush", make_brush(None, "Box", "BloodBright", 0.95))
    cb_item_style.set_editor_property("even_row_background_brush", make_brush(None, "Box", "Void", 0.92))
    cb_item_style.set_editor_property("odd_row_background_brush", make_brush(None, "Box", "Charcoal", 0.92))
    cb_item_style.set_editor_property("even_row_background_hovered_brush", make_brush(None, "Box", "Blood", 0.70))
    cb_item_style.set_editor_property("odd_row_background_hovered_brush", make_brush(None, "Box", "Blood", 0.70))
    cb_item_style.set_editor_property("text_color", slate_col("Bone", 1.0))
    cb_item_style.set_editor_property("selected_text_color", slate_col("Bone", 1.0))

    cb_font = make_cinzel_font(15.0, "SemiBold", 100)
    combo_names = [
        "ComboBoxResolution", "ComboBoxQualityTexture", "ComboBoxShadowQuality",
        "ComboBoxAntiAliasing", "ComboBoxViewDistance", "ComboBoxVisualEffects",
        "ComboBoxPostProcessing", "ComboBoxGlobalIllumination", "ComboBoxShading",
    ]
    for cname in combo_names:
        cb = unreal.find_object(wt, cname)
        cb.set_editor_property("widget_style", cb_style)
        cb.set_editor_property("item_style", cb_item_style)
        cb.set_editor_property("font", cb_font)
        cb.set_editor_property("foreground_color", slate_col("Bone", 1.0))

    # Style GammaSlider with T_UI_Slider_Track + T_UI_Slider_Thumb
    sl = unreal.find_object(wt, "GammaSlider")
    sl_st = unreal.SliderStyle()
    sl_st.set_editor_property(
        "normal_bar_image",
        make_brush(f"{TEX_ROOT}/Bars/T_UI_Slider_Track", "Box", "BoneMuted", 0.85, margin=(0.03125, 0.0, 0.03125, 0.0), image_size=(256, 16)),
    )
    sl_st.set_editor_property(
        "hovered_bar_image",
        make_brush(f"{TEX_ROOT}/Bars/T_UI_Slider_Track", "Box", "BloodBright", 0.95, margin=(0.03125, 0.0, 0.03125, 0.0), image_size=(256, 16)),
    )
    sl_st.set_editor_property(
        "disabled_bar_image",
        make_brush(f"{TEX_ROOT}/Bars/T_UI_Slider_Track", "Box", "Ash", 0.45, margin=(0.03125, 0.0, 0.03125, 0.0), image_size=(256, 16)),
    )
    sl_st.set_editor_property(
        "normal_thumb_image",
        make_brush(f"{TEX_ROOT}/Bars/T_UI_Slider_Thumb", "Image", "Bone", 1.0, image_size=(24, 24)),
    )
    sl_st.set_editor_property(
        "hovered_thumb_image",
        make_brush(f"{TEX_ROOT}/Bars/T_UI_Slider_Thumb", "Image", "BloodBright", 1.0, image_size=(26, 26)),
    )
    sl_st.set_editor_property(
        "disabled_thumb_image",
        make_brush(f"{TEX_ROOT}/Bars/T_UI_Slider_Thumb", "Image", "Ash", 0.50, image_size=(22, 22)),
    )
    sl_st.set_editor_property("bar_thickness", 16.0)
    sl.set_editor_property("widget_style", sl_st)
    sl.set_editor_property("slider_bar_color", lin("White", 1.0))
    sl.set_editor_property("slider_handle_color", lin("White", 1.0))

    # Style DesiredFPS EditableTextBox
    fps = unreal.find_object(wt, "DesiredFPS")
    tb_st = unreal.TextBlockStyle()
    tb_st.set_editor_property("font", cb_font)
    tb_st.set_editor_property("color_and_opacity", slate_col("Bone", 1.0))
    et_st = unreal.EditableTextBoxStyle()
    et_st.set_editor_property("background_image_normal", make_brush(None, "Box", "Charcoal", 0.85))
    et_st.set_editor_property("background_image_hovered", make_brush(None, "Box", "BloodDried", 0.85))
    et_st.set_editor_property("background_image_focused", make_brush(None, "Box", "Blood", 0.55))
    et_st.set_editor_property("text_style", tb_st)
    et_st.set_editor_property("foreground_color", slate_col("Bone", 1.0))
    et_st.set_editor_property("focused_foreground_color", slate_col("Bone", 1.0))
    et_st.set_editor_property("padding", unreal.Margin(12, 6, 12, 6))
    fps.set_editor_property("widget_style", et_st)

    # Style Button_Lumen + Text_Lumen
    btn_l = unreal.find_object(wt, "Button_Lumen")
    hex_m = (0.14844, 0.15625, 0.14844, 0.15625)
    hex_tex = f"{TEX_ROOT}/Frames/T_UI_Btn_Hex_3Slice"
    l_st = unreal.ButtonStyle()
    l_st.set_editor_property("normal", make_brush(hex_tex, "Border", "BoneMuted", 0.85, hex_m))
    l_st.set_editor_property("hovered", make_brush(hex_tex, "Border", "BloodBright", 1.0, hex_m))
    l_st.set_editor_property("pressed", make_brush(hex_tex, "Border", "Bone", 1.0, hex_m))
    l_st.set_editor_property("normal_padding", unreal.Margin(16, 6, 16, 6))
    l_st.set_editor_property("pressed_padding", unreal.Margin(16, 6, 16, 6))
    btn_l.set_editor_property("widget_style", l_st)

    txt_l = unreal.find_object(wt, "Text_Lumen")
    txt_l.set_editor_property("style", style_cls("TS_Value"))

    # 4. Right Description Panel (SettingsPanel + SettingsPanelFrame — exact Mockup_Ajustes brackets!)
    sp = unreal.find_object(wt, "SettingsPanel")
    spf = unreal.find_object(wt, "SettingsPanelFrame")
    sp.set_editor_property("style", style_cls("BS_Panel_Void80"))
    spf.set_editor_property("style", style_cls("BS_Panel_Brackets"))
    set_canvas_slot(sp, (0, 0), (0, 0), (1248, 176), (608, 690), (0, 0), z=2)

    d_box = unreal.find_object(wt, "DescPanelContent")
    if isinstance(d_box.slot, unreal.BorderSlot):
        d_box.slot.set_padding(unreal.Margin(40, 36, 40, 36))
        d_box.slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
        d_box.slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_FILL)

    d_title = unreal.find_object(wt, "DescTitle")
    d_title.set_editor_property("style", style_cls("TS_H2"))
    d_title.set_text("GRÁFICOS Y PANTALLA")

    d_sep = unreal.find_object(wt, "DescSep")
    d_sep.set_editor_property("brush", make_brush(f"{TEX_ROOT}/Ornaments/T_UI_Orn_Sep_Diamond", "Image", "Bone", 0.90, image_size=(520, 22)))
    if isinstance(d_sep.slot, unreal.VerticalBoxSlot):
        d_sep.slot.set_padding(unreal.Margin(0, 8, 0, 22))
        d_sep.slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)

    d_body = unreal.find_object(wt, "DescBody")
    d_body.set_editor_property("style", style_cls("TS_Body"))
    d_body.set_auto_wrap_text(True)
    d_body.set_editor_property("wrap_text_at", 510.0)

    d_sp = unreal.find_object(wt, "DescSpacer")
    if isinstance(d_sp.slot, unreal.VerticalBoxSlot):
        sz = unreal.SlateChildSize()
        sz.set_editor_property("size_rule", unreal.SlateSizeRule.FILL)
        sz.set_editor_property("value", 1.0)
        d_sp.slot.set_size(sz)

    d_quote = unreal.find_object(wt, "DescQuote")
    d_quote.set_editor_property("style", style_cls("TS_Body_Muted"))
    d_quote.set_auto_wrap_text(True)

    # 5. Bottom Footer: Tagline + Dynamic CommonActionWidget prompts (Confirm / Back / Reset)
    tag = unreal.find_object(wt, "TaglineText")
    tag.set_editor_property("style", style_cls("TS_Label"))
    tag.set_text("ALGUNAS COSAS  |  NUNCA MUEREN")
    set_canvas_slot(tag, (0, 1), (0, 1), (76, -56), (480, 32), (0, 1), z=4, auto_size=True)

    hints = unreal.find_object(wt, "InputHintsBox")
    set_canvas_slot(hints, (1, 1), (1, 1), (-64, -56), (520, 40), (1, 1), z=4, auto_size=True)
    for ic_name, row_name, pad_l, lb_name, lb_txt in (
        ("ActionConfirmIcon", "Confirm", 0, "ActionConfirmLabel", "SELECCIONAR"),
        ("ActionBackIcon", "Back", 24, "ActionBackLabel", "ATRÁS"),
        ("ActionResetIcon", "Reset", 24, "ActionResetLabel", "RESTABLECER"),
    ):
        ic = unreal.find_object(wt, ic_name)
        lb = unreal.find_object(wt, lb_name)
        bind_action_widget(ic, row_name)
        if isinstance(ic.slot, unreal.HorizontalBoxSlot):
            ic.slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)
            ic.slot.set_padding(unreal.Margin(pad_l, 0, 8, 0))
        lb.set_editor_property("style", style_cls("TS_Label"))
        lb.set_text(lb_txt)
        if isinstance(lb.slot, unreal.HorizontalBoxSlot):
            lb.slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)

    unreal.BlueprintEditorLibrary.compile_blueprint(wb)
    eal.save_loaded_asset(wb, only_if_is_dirty=False)
    print("Restyled WBP_OptionsMenu (Mockup_Ajustes 3-column layout)")


if __name__ == "__main__":
    restyle_options_menu()
    print("=== PHASE 3 OPTIONS MENU COMPLETE ===")
