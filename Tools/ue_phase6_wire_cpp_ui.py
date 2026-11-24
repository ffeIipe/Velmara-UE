"""
Velmara UI — Phase 6: V2 Logo & Extra Icons Import, DT_RichTextStyleSet Update,
CDO WidgetClass Reassignment, Bridge WidgetBlueprint Reparenting, and Compile Audit.
"""

import json
import os
import unreal

ROOT = r"i:\UnrealProjects\TesisUE"
PROC_DIR = os.path.join(ROOT, "UI_Processed")
REPORT_PATH = r"C:\Users\felipe\.gemini\antigravity\brain\f48666bf-c91a-484c-89a2-dc8b9ca072d1\scratch\phase6_wire_report.txt"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
bel = unreal.BlueprintEditorLibrary

LOG_LINES = []


def log(msg: str):
    print(msg)
    LOG_LINES.append(msg)


HEX_PALETTE = {
    "Bone": "#E8E0D0",
    "BoneMuted": "#A39C8E",
    "Blood": "#8E0F14",
    "BloodBright": "#C21A1F",
    "Spectral": "#8F86A8",
    "Sunlight": "#D9A441",
}


def srgb_to_linear(c8: int) -> float:
    c = c8 / 255.0
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def lin_color(token: str) -> tuple[float, float, float]:
    hx = HEX_PALETTE[token].lstrip("#")
    return (
        srgb_to_linear(int(hx[0:2], 16)),
        srgb_to_linear(int(hx[2:4], 16)),
        srgb_to_linear(int(hx[4:6], 16)),
    )


def import_v2_textures():
    items = [
        ("Backgrounds/T_UI_Logo_Velmara_V2.png", "/Game/UI/Textures/Backgrounds", "T_UI_Logo_Velmara_V2"),
        ("Icons/T_UI_Icon_Menu_Audio.png", "/Game/UI/Textures/Icons", "T_UI_Icon_Menu_Audio"),
        ("Icons/T_UI_Icon_Menu_SaveSlot.png", "/Game/UI/Textures/Icons", "T_UI_Icon_Menu_SaveSlot"),
        ("Icons/T_UI_Icon_Menu_Controls.png", "/Game/UI/Textures/Icons", "T_UI_Icon_Menu_Controls"),
        ("Icons/T_UI_Icon_Menu_Display.png", "/Game/UI/Textures/Icons", "T_UI_Icon_Menu_Display"),
        ("Icons/T_UI_Icon_Menu_Language.png", "/Game/UI/Textures/Icons", "T_UI_Icon_Menu_Language"),
        ("Icons/T_UI_Icon_Menu_Tip.png", "/Game/UI/Textures/Icons", "T_UI_Icon_Menu_Tip"),
    ]
    tasks = []
    expected = []
    for rel, dest_dir, name in items:
        src = os.path.join(PROC_DIR, rel.replace("/", os.sep))
        if not os.path.exists(src):
            log(f"[WARN] Missing source file: {src}")
            continue
        t = unreal.AssetImportTask()
        t.filename = src
        t.destination_path = dest_dir
        t.destination_name = name
        t.replace_existing = True
        t.replace_existing_settings = True
        t.automated = True
        t.save = False
        tasks.append(t)
        expected.append(f"{dest_dir}/{name}")

    asset_tools.import_asset_tasks(tasks)

    ok = 0
    for path in expected:
        tex = unreal.load_asset(path)
        if not tex or not isinstance(tex, unreal.Texture2D):
            log(f"[ERROR] Failed to import: {path}")
            continue
        tex.set_editor_properties({
            "lod_group": unreal.TextureGroup.TEXTUREGROUP_UI,
            "compression_settings": unreal.TextureCompressionSettings.TC_EDITOR_ICON,
            "mip_gen_settings": unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS,
            "srgb": True,
            "never_stream": True,
            "address_x": unreal.TextureAddress.TA_CLAMP,
            "address_y": unreal.TextureAddress.TA_CLAMP,
        })
        eal.save_loaded_asset(tex, only_if_is_dirty=False)
        ok += 1
    log(f"Imported & configured V2 textures: {ok}/{len(expected)}")


def update_richtext_datatable():
    dt_path = "/Game/Blueprints/DataTables/DT_RichTextStyleSet"
    dt = unreal.load_asset(dt_path)
    if not dt:
        log("[WARN] DT_RichTextStyleSet not found")
        return

    cardo_reg = "/Game/Fonts/CARDO-REGULAR_Font"
    cardo_bold = "/Game/Fonts/CARDO-BOLD_Font"
    if not eal.does_asset_exist(cardo_reg):
        cardo_reg = "/Game/NewFonts/Cardo-Regular"
    if not eal.does_asset_exist(cardo_bold):
        cardo_bold = cardo_reg

    def row_entry(name: str, font_path: str, size: float, spacing: int, col_tok: str):
        r, g, b = lin_color(col_tok)
        font_ref = f"/Script/Engine.Font'{font_path}.{font_path.split('/')[-1]}'"
        style_str = (
            f'(Font=(FontObject="{font_ref}",FontMaterial=None,'
            f'OutlineSettings=(OutlineSize=0,bMiteredCorners=False,bSeparateFillAlpha=False,bApplyOutlineToDropShadows=False,OutlineMaterial=None,OutlineColor=(R=0.0,G=0.0,B=0.0,A=1.0)),'
            f'TypefaceFontName="Default",Size={size:.6f},LetterSpacing={spacing},SkewAmount=0.0,bForceMonospaced=False,bMaterialIsStencil=False,MonospacedWidth=1.0),'
            f'ColorAndOpacity=(SpecifiedColor=(R={r:.6f},G={g:.6f},B={b:.6f},A=1.0),ColorUseRule=UseColor_Specified),'
            f'ShadowOffset=(X=1.0,Y=1.0),ShadowColorAndOpacity=(R=0.0,G=0.0,B=0.0,A=0.55),'
            f'TransformPolicy=None,OverflowPolicy=Clip)'
        )
        return {"Name": name, "TextStyle": style_str}

    rows = [
        row_entry("Default", cardo_reg, 18.0, 0, "Bone"),
        row_entry("Emphasis", cardo_bold, 18.0, 20, "BloodBright"),
        row_entry("Spectral", cardo_bold, 18.0, 20, "Spectral"),
        row_entry("Key", cardo_bold, 18.0, 40, "Bone"),
        row_entry("Action", cardo_bold, 18.0, 20, "BloodBright"),
        row_entry("Object", cardo_bold, 18.0, 20, "Spectral"),
        row_entry("Muted", cardo_reg, 18.0, 0, "BoneMuted"),
        row_entry("Value", cardo_bold, 18.0, 20, "Bone"),
        row_entry("Warning", cardo_bold, 18.0, 20, "Sunlight"),
    ]
    ok = unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(dt, json.dumps(rows, indent=2))
    if not ok:
        raise RuntimeError("Failed to update DT_RichTextStyleSet")
    eal.save_loaded_asset(dt, only_if_is_dirty=False)
    log(f"Updated DT_RichTextStyleSet rows: {[str(r) for r in dt.get_row_names()]}")


def wire_cdo_widget_classes():
    assignments = [
        ("/Game/Blueprints/HUD/HUD_Velmara", "player_main_widget_class", "/Script/TesisUE.VelmaraGameplayHUDWidget"),
        ("/Game/Blueprints/Player/PC_Velmara", "pause_menu_widget_class", "/Script/TesisUE.VelmaraPauseMenuWidget"),
        ("/Game/Blueprints/Player/PC_Velmara", "options_menu_widget_class_pause", "/Script/TesisUE.VelmaraSettingsMenuWidget"),
        ("/Game/Blueprints/Events/BP_VelmaraGameInstance", "loading_screen_widget_class", "/Script/TesisUE.VelmaraLoadingScreenWidget"),
        ("/Game/Blueprints/Entities/Enemies/Boss/CBP_Boss", "health_bar_class", "/Script/TesisUE.VelmaraBossBarWidget"),
    ]
    modified_bps = {}
    for bp_path, prop_name, cls_path in assignments:
        bp = unreal.load_asset(bp_path)
        cls = unreal.load_class(None, cls_path)
        if not bp or not cls:
            log(f"[ERROR] Missing bp or class: {bp_path} / {cls_path}")
            continue
        cdo = unreal.get_default_object(bp.generated_class())
        cdo.set_editor_property(prop_name, cls)
        modified_bps[bp_path] = bp
        log(f"Wired CDO {bp_path}.{prop_name} = {cls_path}")

    for bp_path, bp in modified_bps.items():
        bel.compile_blueprint(bp)
        eal.save_loaded_asset(bp, only_if_is_dirty=False)


def reparent_bridge_widget_blueprints():
    reparents = [
        ("/Game/Blueprints/Tutorial/WBP_PromptPopUp", "/Script/TesisUE.VelmaraLegacyPromptBridgeWidget"),
        ("/Game/Blueprints/HUD/Menus/WBP_TitleScreen", "/Script/TesisUE.VelmaraTitleScreenBridgeWidget"),
        ("/Game/WBP_DeathScreen", "/Script/TesisUE.VelmaraDeathScreenWidget"),
        ("/Game/Blueprints/HUD/Menus/WBP_LoadingScreen", "/Script/TesisUE.VelmaraLoadingScreenWidget"),
        ("/Game/Blueprints/HUD/WBP_VelmaraBootLoading", "/Script/TesisUE.VelmaraLoadingScreenWidget"),
        ("/Game/Blueprints/HUD/Inventory/WBP_Inventory", "/Script/TesisUE.VelmaraInventoryWidget"),
        ("/Game/Blueprints/HUD/Menus/WBP_ShopMenu", "/Script/TesisUE.VelmaraShopMenuWidget"),
        ("/Game/Blueprints/HUD/Gameplay/WBP_TargetEntity", "/Script/TesisUE.VelmaraLockOnReticleWidget"),
        ("/Game/Blueprints/HUD/Gameplay/WBP_PossessableEntity", "/Script/TesisUE.VelmaraPossessionReticleWidget"),
        ("/Game/Blueprints/HUD/WBP_PaladinBossHealthBar", "/Script/TesisUE.VelmaraBossBarWidget"),
    ]

    log("=== REPARENTING BRIDGE WIDGET BLUEPRINTS ===")
    for bp_path, cpp_class_path in reparents:
        if not eal.does_asset_exist(bp_path):
            log(f"  [SKIP] Missing asset: {bp_path}")
            continue
        bp = unreal.load_asset(bp_path)
        new_parent = unreal.load_class(None, cpp_class_path)
        if not bp or not new_parent:
            log(f"  [ERROR] Failed to load {bp_path} or {cpp_class_path}")
            continue
        try:
            bel.reparent_blueprint(bp, new_parent)
            if bp_path == "/Game/Blueprints/HUD/Menus/WBP_ShopMenu":
                bel.remove_unused_variables(bp)
            bel.compile_blueprint(bp)
            eal.save_loaded_asset(bp, only_if_is_dirty=False)
            gen = bp.generated_class()
            log(f"  [OK] Reparented & saved {bp_path} -> {cpp_class_path} (gen={gen.get_name() if gen else 'None'})")
        except Exception as e:
            log(f"  [ERROR] Reparent failed for {bp_path}: {e}")


def audit_all_key_blueprints():
    audit_list = [
        "/Game/Blueprints/HUD/HUD_Velmara",
        "/Game/Blueprints/Player/PC_Velmara",
        "/Game/Blueprints/Events/BP_VelmaraGameInstance",
        "/Game/Blueprints/Entities/Enemies/Boss/CBP_Boss",
        "/Game/Blueprints/Entities/Player/CBP_Player",
        "/Game/Blueprints/Entities/Player/CBP_Player_NewVelmara",
        "/Game/Blueprints/Entities/CBP_Entity",
        "/Game/Blueprints/Entities/Enemies/CBP_Enemy",
        "/Game/Blueprints/Entities/Enemies/BP_Totem",
        "/Game/Blueprints/Events/Level/Prompts/BP_TutorialPopUp",
        "/Game/Blueprints/GAS/Abilities/Player/GA_Death",
        "/Game/Blueprints/SpectralMode/BP_Altar",
        "/Game/Maps/MainMenu/BP_MenuControl",
        "/Game/Blueprints/Tutorial/WBP_PromptPopUp",
        "/Game/Blueprints/HUD/Menus/WBP_TitleScreen",
        "/Game/WBP_DeathScreen",
        "/Game/Blueprints/HUD/Menus/WBP_LoadingScreen",
        "/Game/Blueprints/HUD/WBP_VelmaraBootLoading",
        "/Game/Blueprints/HUD/Inventory/WBP_Inventory",
        "/Game/Blueprints/HUD/Menus/WBP_ShopMenu",
        "/Game/Blueprints/HUD/Gameplay/WBP_TargetEntity",
        "/Game/Blueprints/HUD/Gameplay/WBP_PossessableEntity",
        "/Game/Blueprints/HUD/WBP_PaladinBossHealthBar",
    ]
    log("=== FINAL BLUEPRINT COMPILE AUDIT ===")
    for bp_path in audit_list:
        if not eal.does_asset_exist(bp_path):
            log(f"  [MISSING] {bp_path}")
            continue
        bp = unreal.load_asset(bp_path)
        try:
            bel.compile_blueprint(bp)
            eal.save_loaded_asset(bp, only_if_is_dirty=True)
            log(f"  [COMPILED] {bp_path}")
        except Exception as e:
            log(f"  [COMPILE_ERROR] {bp_path}: {e}")


if __name__ == "__main__":
    import_v2_textures()
    update_richtext_datatable()
    wire_cdo_widget_classes()
    reparent_bridge_widget_blueprints()
    audit_all_key_blueprints()
    log("=== PHASE 6 COMPLETE ===")
    with open(REPORT_PATH, "w", encoding="utf-8") as f:
        f.write("\n".join(LOG_LINES) + "\n")
