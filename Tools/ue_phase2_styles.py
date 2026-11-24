"""
Velmara UI — Phase 2 Step B: Common UI Styles & RichText StyleSet
Creates/updates CommonTextStyle, CommonBorderStyle, CommonButtonStyle assets in
/Game/Blueprints/HUD/Styles and extends /Game/Blueprints/DataTables/DT_RichTextStyleSet.
"""

import json
import unreal

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary

STYLES_DIR = "/Game/Blueprints/HUD/Styles"
TEX_ROOT = "/Game/UI/Textures"

# Velmara Palette (sRGB hex -> exact LinearColor for Slate/UMG)
HEX_PALETTE = {
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


def srgb_channel_to_linear(c8: int) -> float:
    c = c8 / 255.0
    if c <= 0.04045:
        return c / 12.92
    return ((c + 0.055) / 1.055) ** 2.4


def lin_color(token: str, alpha: float = 1.0) -> unreal.LinearColor:
    hx = HEX_PALETTE[token].lstrip("#")
    r = srgb_channel_to_linear(int(hx[0:2], 16))
    g = srgb_channel_to_linear(int(hx[2:4], 16))
    b = srgb_channel_to_linear(int(hx[4:6], 16))
    return unreal.LinearColor(r, g, b, alpha)


def slate_color(token: str, alpha: float = 1.0) -> unreal.SlateColor:
    sc = unreal.SlateColor()
    sc.set_editor_property("specified_color", lin_color(token, alpha))
    sc.set_editor_property("color_use_rule", unreal.SlateColorStylingMode.USE_COLOR_SPECIFIED)
    return sc


def make_font_info(font_asset_path: str, typeface: str, size: float, letter_spacing: int = 0) -> unreal.SlateFontInfo:
    font_obj = unreal.load_asset(font_asset_path)
    if not font_obj:
        raise RuntimeError(f"Font asset not found: {font_asset_path}")
    info = unreal.SlateFontInfo()
    info.set_editor_property("font_object", font_obj)
    info.set_editor_property("typeface_font_name", typeface)
    info.set_editor_property("size", float(size))
    info.set_editor_property("letter_spacing", int(letter_spacing))
    return info


def make_slate_vec2(x: float, y: float) -> unreal.DeprecateSlateVector2D:
    v = unreal.DeprecateSlateVector2D()
    v.import_text(f"(X={float(x):.2f},Y={float(y):.2f})")
    return v


def make_brush(
    tex_path: str | None = None,
    draw_as: str = "Image",
    tint_token: str = "White",
    alpha: float = 1.0,
    margin: tuple[float, float, float, float] | None = None,
    image_size: tuple[float, float] | None = None,
) -> unreal.SlateBrush:
    draw_map = {
        "NoDrawType": unreal.SlateBrushDrawType.NO_DRAW_TYPE,
        "Box": unreal.SlateBrushDrawType.BOX,
        "Border": unreal.SlateBrushDrawType.BORDER,
        "Image": unreal.SlateBrushDrawType.IMAGE,
        "RoundedBox": unreal.SlateBrushDrawType.ROUNDED_BOX,
    }
    b = unreal.SlateBrush()
    b.set_editor_property("draw_as", draw_map[draw_as])
    b.set_editor_property("tint_color", slate_color(tint_token, alpha))
    b.set_editor_property("tiling", unreal.SlateBrushTileType.NO_TILE)
    b.set_editor_property("mirroring", unreal.SlateBrushMirrorType.NO_MIRROR)
    if tex_path:
        tex = unreal.load_asset(tex_path)
        if not tex:
            raise RuntimeError(f"Texture asset not found: {tex_path}")
        b.set_editor_property("resource_object", tex)
        if image_size:
            b.set_editor_property("image_size", make_slate_vec2(image_size[0], image_size[1]))
        elif draw_as == "Border":
            # 2x supersampled frames/slots render at crisp 1x Slate pixel dimensions at 1080p
            b.set_editor_property(
                "image_size",
                make_slate_vec2(tex.blueprint_get_size_x() * 0.5, tex.blueprint_get_size_y() * 0.5),
            )
        else:
            b.set_editor_property(
                "image_size",
                make_slate_vec2(tex.blueprint_get_size_x(), tex.blueprint_get_size_y()),
            )
    else:
        b.set_editor_property("resource_object", None)
        if image_size:
            b.set_editor_property("image_size", make_slate_vec2(image_size[0], image_size[1]))
    if margin:
        m = unreal.Margin(left=margin[0], top=margin[1], right=margin[2], bottom=margin[3])
        b.set_editor_property("margin", m)
    else:
        b.set_editor_property("margin", unreal.Margin(0.0, 0.0, 0.0, 0.0))
    return b


def get_or_create_bp(name: str, parent_class_path: str, folder: str = STYLES_DIR) -> unreal.Blueprint:
    full_path = f"{folder}/{name}"
    if eal.does_asset_exist(full_path):
        bp = unreal.load_asset(full_path)
        return bp
    parent_cls = unreal.load_class(None, parent_class_path)
    if not parent_cls:
        raise RuntimeError(f"Parent class not found: {parent_class_path}")
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_cls)
    bp = asset_tools.create_asset(name, folder, unreal.Blueprint, factory)
    if not bp:
        raise RuntimeError(f"Failed creating Blueprint {full_path}")
    return bp


def compile_and_save_bp(bp: unreal.Blueprint) -> None:
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    eal.save_loaded_asset(bp, only_if_is_dirty=False)


def configure_text_styles():
    CINZEL = "/Game/Fonts/Cinzel_Font"
    FORUM = "/Game/Fonts/Forum_Font"
    PARENT = "/Script/CommonUI.CommonTextStyle"

    # (asset_name, font_path, typeface, size, letter_spacing, color_token, alpha, line_height, drop_shadow)
    specs = [
        # Existing project styles updated in-place (PromptKit §7 Rule 4)
        ("BP_TitleTextStyle", CINZEL, "SemiBold", 48, 200, "Bone", 1.0, 1.0, False),
        ("BP_SubtitleTextStyle", FORUM, "Regular", 32, 0, "Bone", 1.0, 1.15, True),
        ("BP_ParragraphTextStyle", FORUM, "Regular", 24, 0, "Bone", 1.0, 1.25, False),
        ("BP_ButtonTextStyle", CINZEL, "Regular", 22, 120, "BoneMuted", 1.0, 1.0, False),
        ("BP_ButtonTextStyle_Hover", CINZEL, "Regular", 22, 120, "Bone", 1.0, 1.0, False),
        # Full Velmara Design System scale (1080p, FontDPI=72)
        ("TS_Display", CINZEL, "Regular", 72, 250, "Bone", 1.0, 1.0, True),
        ("TS_H1", CINZEL, "SemiBold", 48, 200, "Bone", 1.0, 1.0, False),
        ("TS_H2", CINZEL, "Regular", 32, 150, "Bone", 1.0, 1.0, False),
        ("TS_H3", CINZEL, "Regular", 24, 120, "Bone", 1.0, 1.0, False),
        ("TS_Button_Normal", CINZEL, "Regular", 22, 120, "BoneMuted", 1.0, 1.0, False),
        ("TS_Button_Hover", CINZEL, "Regular", 22, 120, "Bone", 1.0, 1.0, False),
        ("TS_Button_Disabled", CINZEL, "Regular", 22, 120, "Ash", 0.40, 1.0, False),
        ("TS_Tab_Normal", CINZEL, "Regular", 18, 150, "BoneMuted", 1.0, 1.0, False),
        ("TS_Tab_Selected", CINZEL, "SemiBold", 18, 150, "BloodBright", 1.0, 1.0, False),
        ("TS_Body", FORUM, "Regular", 24, 0, "Bone", 1.0, 1.25, False),
        ("TS_Body_Muted", FORUM, "Regular", 22, 0, "BoneMuted", 1.0, 1.25, False),
        ("TS_Subtitle", FORUM, "Regular", 32, 0, "Bone", 1.0, 1.15, True),
        ("TS_Label", CINZEL, "Regular", 18, 100, "BoneMuted", 1.0, 1.0, False),
        ("TS_Value", CINZEL, "SemiBold", 18, 120, "Bone", 1.0, 1.0, False),
        ("TS_Caption", FORUM, "Regular", 16, 0, "BoneMuted", 1.0, 1.15, False),
        ("TS_BossName", CINZEL, "Regular", 28, 300, "Bone", 1.0, 1.0, True),
        ("TS_Speaker", CINZEL, "SemiBold", 22, 150, "BloodBright", 1.0, 1.0, False),
    ]

    for name, fpath, face, size, spacing, col_tok, alpha, lh, shadow in specs:
        bp = get_or_create_bp(name, PARENT)
        cdo = unreal.get_default_object(bp.generated_class())
        cdo.set_editor_property("font", make_font_info(fpath, face, size, spacing))
        cdo.set_editor_property("color", lin_color(col_tok, alpha))
        cdo.set_editor_property("line_height_percentage", float(lh))
        cdo.set_editor_property("uses_drop_shadow", bool(shadow))
        if shadow:
            cdo.set_editor_property("shadow_offset", unreal.Vector2D(1.5, 1.5))
            cdo.set_editor_property("shadow_color", unreal.LinearColor(0.0, 0.0, 0.0, 0.75))
        compile_and_save_bp(bp)
    print(f"Configured {len(specs)} CommonTextStyle assets.")


def update_richtext_datatable():
    dt_path = "/Game/Blueprints/DataTables/DT_RichTextStyleSet"
    dt = unreal.load_asset(dt_path)
    if not dt:
        print("WARNING: DT_RichTextStyleSet not found")
        return

    def row_entry(name: str, font_path: str, typeface: str, size: float, spacing: int, col_tok: str):
        lc = lin_color(col_tok, 1.0)
        font_ref = f"/Script/Engine.Font'{font_path}.{font_path.split('/')[-1]}'"
        style_str = (
            f'(Font=(FontObject="{font_ref}",FontMaterial=None,'
            f'OutlineSettings=(OutlineSize=0,bMiteredCorners=False,bSeparateFillAlpha=False,bApplyOutlineToDropShadows=False,OutlineMaterial=None,OutlineColor=(R=0.0,G=0.0,B=0.0,A=1.0)),'
            f'TypefaceFontName="{typeface}",Size={size:.6f},LetterSpacing={spacing},SkewAmount=0.0,bForceMonospaced=False,bMaterialIsStencil=False,MonospacedWidth=1.0),'
            f'ColorAndOpacity=(SpecifiedColor=(R={lc.r:.6f},G={lc.g:.6f},B={lc.b:.6f},A=1.0),ColorUseRule=UseColor_Specified),'
            f'ShadowOffset=(X=1.0,Y=1.0),ShadowColorAndOpacity=(R=0.0,G=0.0,B=0.0,A=0.55),'
            f'TransformPolicy=None,OverflowPolicy=Clip)'
        )
        return {"Name": name, "TextStyle": style_str}

    rows = [
        # Existing keys preserved + upgraded to Velmara fonts/palette
        row_entry("Default", "/Game/Fonts/Forum_Font", "Regular", 24.0, 0, "Bone"),
        row_entry("Action", "/Game/Fonts/Cinzel_Font", "SemiBold", 22.0, 100, "BloodBright"),
        row_entry("Object", "/Game/Fonts/Cinzel_Font", "Regular", 22.0, 80, "Spectral"),
        # PromptKit §7 Rule 4 required inline tags
        row_entry("Emphasis", "/Game/Fonts/Cinzel_Font", "SemiBold", 24.0, 80, "BloodBright"),
        row_entry("Muted", "/Game/Fonts/Forum_Font", "Regular", 24.0, 0, "BoneMuted"),
        row_entry("Value", "/Game/Fonts/Cinzel_Font", "SemiBold", 24.0, 100, "Bone"),
        row_entry("Warning", "/Game/Fonts/Cinzel_Font", "SemiBold", 24.0, 80, "Sunlight"),
    ]
    json_str = json.dumps(rows, indent=2)
    ok = unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(dt, json_str)
    if not ok:
        raise RuntimeError("Failed to populate DT_RichTextStyleSet from JSON")
    eal.save_loaded_asset(dt, only_if_is_dirty=False)
    print(f"Updated DT_RichTextStyleSet with rows: {dt.get_row_names()}")


def configure_border_styles():
    PARENT = "/Script/CommonUI.CommonBorderStyle"
    specs = [
        # Updated existing styles in /Game/Blueprints/HUD/Styles
        ("BP_MainMenuBorderStyle", None, "Box", "Void", 0.80, None),
        ("BP_PauseBorderStyle", None, "Box", "Void", 0.65, None),
        ("BP_SettingsBorderStyle", f"{TEX_ROOT}/Frames/T_UI_Frame_Brackets_Panel", "Border", "Bone", 0.92, (0.15625,) * 4),
        # New reusable Velmara Border Styles
        ("BS_Panel_Void80", None, "Box", "Void", 0.80, None),
        ("BS_Panel_Overlay60", None, "Box", "Void", 0.60, None),
        ("BS_Panel_Brackets", f"{TEX_ROOT}/Frames/T_UI_Frame_Brackets_Panel", "Border", "Bone", 0.92, (0.15625,) * 4),
        ("BS_Panel_BracketsMinimal", f"{TEX_ROOT}/Frames/T_UI_Frame_Brackets_Minimal", "Border", "Bone", 0.92, (0.16797,) * 4),
        ("BS_Panel_Gothic", f"{TEX_ROOT}/Frames/T_UI_Frame_Panel_Gothic", "Border", "BoneMuted", 0.85, (0.09766,) * 4),
        ("BS_Slot_Square_Normal", f"{TEX_ROOT}/Frames/T_UI_Slot_Square_Normal", "Border", "BoneMuted", 0.90, (0.13281,) * 4),
        ("BS_Slot_Square_Selected", f"{TEX_ROOT}/Frames/T_UI_Slot_Square_Selected", "Border", "BloodBright", 1.0, (0.23438,) * 4),
    ]
    for name, tex, draw_as, tint, alpha, margin in specs:
        bp = get_or_create_bp(name, PARENT)
        cdo = unreal.get_default_object(bp.generated_class())
        cdo.set_editor_property("background", make_brush(tex, draw_as, tint, alpha, margin))
        compile_and_save_bp(bp)
    print(f"Configured {len(specs)} CommonBorderStyle assets.")


def text_style_class(name: str):
    bp = unreal.load_asset(f"{STYLES_DIR}/{name}")
    if not bp:
        raise RuntimeError(f"Missing text style BP: {name}")
    return bp.generated_class()


def configure_button_styles():
    PARENT = "/Script/CommonUI.CommonButtonStyle"
    grad_center = f"{TEX_ROOT}/Gradients/T_UI_Grad_HFade_Center"
    grad_ltr = f"{TEX_ROOT}/Gradients/T_UI_Grad_HFade_LeftToRight"
    row_frame = f"{TEX_ROOT}/Frames/T_UI_Frame_OptionRow"
    row_margin = (0.10156, 0.125, 0.10156, 0.125)
    sidebar_active = f"{TEX_ROOT}/Frames/T_UI_Frame_Sidebar_Active"
    hex_frame = f"{TEX_ROOT}/Frames/T_UI_Btn_Hex_3Slice"
    hex_margin = (0.14844, 0.15625, 0.14844, 0.15625)

    def apply_btn(
        name: str,
        brushes: dict,
        text_styles: dict,
        min_w: int,
        min_h: int,
        pad: tuple[float, float, float, float],
    ):
        bp = get_or_create_bp(name, PARENT)
        cdo = unreal.get_default_object(bp.generated_class())
        cdo.set_editor_property("single_material", False)
        for prop, brush in brushes.items():
            cdo.set_editor_property(prop, brush)
        for prop, ts_name in text_styles.items():
            cdo.set_editor_property(prop, text_style_class(ts_name))
        cdo.set_editor_property("min_width", min_w)
        cdo.set_editor_property("min_height", min_h)
        cdo.set_editor_property("button_padding", unreal.Margin(pad[0], pad[1], pad[2], pad[3]))
        compile_and_save_bp(bp)

    # 1. BP_ButtonStyle (existing Main Menu / Pause / Dialogue button updated in-place)
    apply_btn(
        "BP_ButtonStyle",
        brushes={
            "normal_base": make_brush(None, "NoDrawType"),
            "normal_hovered": make_brush(grad_center, "Image", "Blood", 0.75),
            "normal_pressed": make_brush(grad_center, "Image", "BloodBright", 0.88),
            "selected_base": make_brush(grad_center, "Image", "Blood", 0.55),
            "selected_hovered": make_brush(grad_center, "Image", "Blood", 0.82),
            "selected_pressed": make_brush(grad_center, "Image", "BloodBright", 0.92),
            "disabled": make_brush(None, "NoDrawType"),
        },
        text_styles={
            "normal_text_style": "BP_ButtonTextStyle",
            "normal_hovered_text_style": "BP_ButtonTextStyle_Hover",
            "selected_text_style": "BP_ButtonTextStyle_Hover",
            "selected_hovered_text_style": "BP_ButtonTextStyle_Hover",
            "disabled_text_style": "TS_Button_Disabled",
        },
        min_w=0,
        min_h=48,
        pad=(24.0, 10.0, 24.0, 10.0),
    )

    # 2. BS_Btn_SettingsRow (Mockup_Ajustes option row frame with < > chevrons)
    apply_btn(
        "BS_Btn_SettingsRow",
        brushes={
            "normal_base": make_brush(row_frame, "Border", "BoneMuted", 0.65, row_margin),
            "normal_hovered": make_brush(row_frame, "Border", "BloodBright", 1.0, row_margin),
            "normal_pressed": make_brush(row_frame, "Border", "BloodBright", 1.0, row_margin),
            "selected_base": make_brush(row_frame, "Border", "BloodBright", 1.0, row_margin),
            "selected_hovered": make_brush(row_frame, "Border", "BloodBright", 1.0, row_margin),
            "selected_pressed": make_brush(row_frame, "Border", "BloodBright", 1.0, row_margin),
            "disabled": make_brush(row_frame, "Border", "Ash", 0.40, row_margin),
        },
        text_styles={
            "normal_text_style": "TS_Label",
            "normal_hovered_text_style": "TS_Value",
            "selected_text_style": "TS_Value",
            "selected_hovered_text_style": "TS_Value",
            "disabled_text_style": "TS_Button_Disabled",
        },
        min_w=0,
        min_h=56,
        pad=(40.0, 12.0, 40.0, 12.0),
    )

    # 3. BS_Btn_SidebarCategory (Mockup_Ajustes left column category button)
    apply_btn(
        "BS_Btn_SidebarCategory",
        brushes={
            "normal_base": make_brush(None, "NoDrawType"),
            "normal_hovered": make_brush(grad_ltr, "Image", "Blood", 0.65),
            "normal_pressed": make_brush(grad_ltr, "Image", "BloodBright", 0.85),
            "selected_base": make_brush(sidebar_active, "Image", "BloodBright", 0.95),
            "selected_hovered": make_brush(sidebar_active, "Image", "BloodBright", 1.0),
            "selected_pressed": make_brush(sidebar_active, "Image", "BloodBright", 1.0),
            "disabled": make_brush(None, "NoDrawType"),
        },
        text_styles={
            "normal_text_style": "TS_Button_Normal",
            "normal_hovered_text_style": "TS_Button_Hover",
            "selected_text_style": "TS_Button_Hover",
            "selected_hovered_text_style": "TS_Button_Hover",
            "disabled_text_style": "TS_Button_Disabled",
        },
        min_w=0,
        min_h=52,
        pad=(20.0, 10.0, 20.0, 10.0),
    )

    # 4. BS_Btn_TopTab (Mockup_Ajustes top navigation tab)
    apply_btn(
        "BS_Btn_TopTab",
        brushes={
            "normal_base": make_brush(None, "NoDrawType"),
            "normal_hovered": make_brush(grad_center, "Image", "Blood", 0.35),
            "normal_pressed": make_brush(grad_center, "Image", "Blood", 0.50),
            "selected_base": make_brush(grad_center, "Image", "Blood", 0.22),
            "selected_hovered": make_brush(grad_center, "Image", "Blood", 0.40),
            "selected_pressed": make_brush(grad_center, "Image", "Blood", 0.55),
            "disabled": make_brush(None, "NoDrawType"),
        },
        text_styles={
            "normal_text_style": "TS_Tab_Normal",
            "normal_hovered_text_style": "TS_Value",
            "selected_text_style": "TS_Tab_Selected",
            "selected_hovered_text_style": "TS_Tab_Selected",
            "disabled_text_style": "TS_Button_Disabled",
        },
        min_w=0,
        min_h=40,
        pad=(20.0, 8.0, 20.0, 8.0),
    )

    # 5. BS_Btn_PrimaryHex (Hexagonal blade-pointed button for Confirm/Cancel/Actions)
    apply_btn(
        "BS_Btn_PrimaryHex",
        brushes={
            "normal_base": make_brush(hex_frame, "Border", "BoneMuted", 0.80, hex_margin),
            "normal_hovered": make_brush(hex_frame, "Border", "BloodBright", 1.0, hex_margin),
            "normal_pressed": make_brush(hex_frame, "Border", "Bone", 1.0, hex_margin),
            "selected_base": make_brush(hex_frame, "Border", "BloodBright", 1.0, hex_margin),
            "selected_hovered": make_brush(hex_frame, "Border", "BloodBright", 1.0, hex_margin),
            "selected_pressed": make_brush(hex_frame, "Border", "Bone", 1.0, hex_margin),
            "disabled": make_brush(hex_frame, "Border", "Ash", 0.40, hex_margin),
        },
        text_styles={
            "normal_text_style": "TS_Button_Normal",
            "normal_hovered_text_style": "TS_Button_Hover",
            "selected_text_style": "TS_Button_Hover",
            "selected_hovered_text_style": "TS_Button_Hover",
            "disabled_text_style": "TS_Button_Disabled",
        },
        min_w=220,
        min_h=52,
        pad=(36.0, 12.0, 36.0, 12.0),
    )
    print("Configured 5 CommonButtonStyle assets.")


if __name__ == "__main__":
    configure_text_styles()
    update_richtext_datatable()
    configure_border_styles()
    configure_button_styles()
    print("=== PHASE 2 STYLES COMPLETE ===")
