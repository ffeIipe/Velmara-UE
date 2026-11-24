"""
Velmara UI — Phase 2 Step C: Dynamic Keyboard/Mouse & Gamepad CommonUI InputData
Creates:
  - /Game/Blueprints/HUD/InputPrompts/DT_VelmaraInputActions (CommonInputActionDataBase DataTable)
  - /Game/Blueprints/HUD/InputPrompts/BP_VelmaraInputData (CommonUIInputData)
  - /Game/Blueprints/HUD/InputPrompts/BP_ControllerData_KBM (CommonInputBaseControllerData)
  - /Game/Blueprints/HUD/InputPrompts/BP_ControllerData_Gamepad_Generic (CommonInputBaseControllerData)
  - /Game/Blueprints/HUD/InputPrompts/BP_ControllerData_Gamepad_PS5 (CommonInputBaseControllerData)
"""

import json
import unreal

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary

PROMPTS_DIR = "/Game/Blueprints/HUD/InputPrompts"
KBM_TEX_DIR = "/Game/Images/Prompts/NewPrompts/Keyboard_Mouse/White"
XG_TEX_DIR = "/Game/Images/Prompts/NewPrompts/XGamepad/Light"
P5_TEX_DIR = "/Game/Images/Prompts/NewPrompts/P5Gamepad/Light"


def srgb_to_lin(c8: int) -> float:
    c = c8 / 255.0
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


# Bone (#E8E0D0) linear color so all white prompt icons match Velmara palette seamlessly
BONE_LIN = unreal.LinearColor(srgb_to_lin(0xE8), srgb_to_lin(0xE0), srgb_to_lin(0xD0), 1.0)


def make_prompt_brush(tex_path: str, size: float = 36.0) -> unreal.SlateBrush:
    tex = unreal.load_asset(tex_path)
    if not tex:
        raise RuntimeError(f"Prompt texture not found: {tex_path}")
    b = unreal.SlateBrush()
    b.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
    sc = unreal.SlateColor()
    sc.set_editor_property("specified_color", BONE_LIN)
    sc.set_editor_property("color_use_rule", unreal.SlateColorStylingMode.USE_COLOR_SPECIFIED)
    b.set_editor_property("tint_color", sc)
    b.set_editor_property("resource_object", tex)
    v = unreal.DeprecateSlateVector2D()
    v.import_text(f"(X={size:.1f},Y={size:.1f})")
    b.set_editor_property("image_size", v)
    return b


def make_key_brush_entry(key_name: str, tex_path: str, size: float = 36.0) -> unreal.CommonInputKeyBrushConfiguration:
    cfg = unreal.CommonInputKeyBrushConfiguration()
    k = unreal.Key()
    k.import_text(key_name)
    cfg.set_editor_property("key", k)
    cfg.set_editor_property("key_brush", make_prompt_brush(tex_path, size))
    return cfg


def get_or_create_bp(name: str, parent_class_path: str) -> unreal.Blueprint:
    full_path = f"{PROMPTS_DIR}/{name}"
    if eal.does_asset_exist(full_path):
        return unreal.load_asset(full_path)
    parent_cls = unreal.load_class(None, parent_class_path)
    if not parent_cls:
        raise RuntimeError(f"Parent class not found: {parent_class_path}")
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_cls)
    bp = asset_tools.create_asset(name, PROMPTS_DIR, unreal.Blueprint, factory)
    if not bp:
        raise RuntimeError(f"Failed creating Blueprint {full_path}")
    return bp


def create_input_action_datatable() -> unreal.DataTable:
    dt_name = "DT_VelmaraInputActions"
    dt_path = f"{PROMPTS_DIR}/{dt_name}"
    if eal.does_asset_exist(dt_path):
        dt = unreal.load_asset(dt_path)
    else:
        row_struct = unreal.load_object(None, "/Script/CommonUI.CommonInputActionDataBase")
        if not row_struct:
            raise RuntimeError("Could not load CommonInputActionDataBase struct")
        factory = unreal.DataTableFactory()
        factory.set_editor_property("struct", row_struct)
        dt = asset_tools.create_asset(dt_name, PROMPTS_DIR, unreal.DataTable, factory)
        if not dt:
            raise RuntimeError(f"Failed to create {dt_path}")

    def make_input_type_info(key: str, hold: bool = False, hold_time: float = 0.0) -> str:
        return (
            f"(Key={key},OverrrideState=Enabled,bActionRequiresHold={'True' if hold else 'False'},"
            f"HoldTime={hold_time:.6f},HoldRollbackTime=0.000000,"
            f"OverrideBrush=(DrawAs=NoDrawType,ResourceObject=None))"
        )

    # Localized Spanish action rows (Namespace: VelmaraUI)
    action_specs = [
        ("Confirm", "Aceptar", "Enter", "Gamepad_FaceButton_Bottom", False, 0.0, 0),
        ("Back", "Volver", "Escape", "Gamepad_FaceButton_Right", False, 0.0, 1),
        ("Reset", "Restablecer", "R", "Gamepad_FaceButton_Left", False, 0.0, 2),
        ("Apply", "Aplicar", "F", "Gamepad_FaceButton_Top", False, 0.0, 3),
        ("PrevTab", "Pestaña anterior", "Q", "Gamepad_LeftShoulder", False, 0.0, 4),
        ("NextTab", "Pestaña siguiente", "E", "Gamepad_RightShoulder", False, 0.0, 5),
        ("Interact", "Interactuar", "E", "Gamepad_FaceButton_Left", False, 0.0, 6),
        ("InteractHold", "Mantener", "E", "Gamepad_FaceButton_Left", True, 0.65, 7),
        ("Unlock", "Desbloquear", "Enter", "Gamepad_FaceButton_Bottom", False, 0.0, 8),
        ("Skip", "Saltar", "SpaceBar", "Gamepad_FaceButton_Right", False, 0.0, 9),
        ("NavLeft", "Anterior", "Left", "Gamepad_DPad_Left", False, 0.0, 10),
        ("NavRight", "Siguiente", "Right", "Gamepad_DPad_Right", False, 0.0, 11),
    ]

    rows = []
    for row_name, display_es, kb_key, gp_key, is_hold, hold_t, prio in action_specs:
        rows.append(
            {
                "Name": row_name,
                "DisplayName": f'NSLOCTEXT("VelmaraUI", "Action_{row_name}", "{display_es}")',
                "HoldDisplayName": f'NSLOCTEXT("VelmaraUI", "ActionHold_{row_name}", "{display_es}")' if is_hold else "",
                "NavBarPriority": prio,
                "KeyboardInputTypeInfo": make_input_type_info(kb_key, is_hold, hold_t),
                "DefaultGamepadInputTypeInfo": make_input_type_info(gp_key, is_hold, hold_t),
            }
        )

    ok = unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(dt, json.dumps(rows, indent=2))
    if not ok:
        raise RuntimeError("Failed filling DT_VelmaraInputActions from JSON")
    eal.save_loaded_asset(dt, only_if_is_dirty=False)
    print(f"Saved DT_VelmaraInputActions with {len(dt.get_row_names())} rows: {dt.get_row_names()}")
    return dt


def create_velmara_input_data(dt: unreal.DataTable):
    bp = get_or_create_bp("BP_VelmaraInputData", "/Script/CommonInput.CommonUIInputData")
    cdo = unreal.get_default_object(bp.generated_class())

    click_handle = unreal.DataTableRowHandle()
    click_handle.set_editor_property("data_table", dt)
    click_handle.set_editor_property("row_name", "Confirm")

    back_handle = unreal.DataTableRowHandle()
    back_handle.set_editor_property("data_table", dt)
    back_handle.set_editor_property("row_name", "Back")

    cdo.set_editor_property("default_click_action", click_handle)
    cdo.set_editor_property("default_back_action", back_handle)

    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    eal.save_loaded_asset(bp, only_if_is_dirty=False)
    print(f"Saved BP_VelmaraInputData -> Click=Confirm, Back=Back")


def build_controller_data(
    bp_name: str,
    input_type: unreal.CommonInputType,
    gamepad_name: str,
    display_name: str,
    category: str,
    key_tex_pairs: list[tuple[str, str]],
):
    bp = get_or_create_bp(bp_name, "/Script/CommonInput.CommonInputBaseControllerData")
    cdo = unreal.get_default_object(bp.generated_class())
    cdo.set_editor_property("input_type", input_type)
    if gamepad_name:
        cdo.set_editor_property("gamepad_name", gamepad_name)
    if display_name:
        cdo.set_editor_property("gamepad_display_name", display_name)
    if category:
        cdo.set_editor_property("gamepad_category", category)

    entries = []
    missing = []
    for kname, tpath in key_tex_pairs:
        if not eal.does_asset_exist(tpath):
            missing.append((kname, tpath))
            continue
        entries.append(make_key_brush_entry(kname, tpath, size=36.0))

    cdo.set_editor_property("input_brush_data_map", entries)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    eal.save_loaded_asset(bp, only_if_is_dirty=False)
    print(f"Saved {bp_name}: {len(entries)} key brushes mapped (missing: {len(missing)})")
    if missing:
        print("  MISSING:", missing)


def create_all_controller_data():
    # 1. Keyboard & Mouse (White icons tinted Bone #E8E0D0)
    kbm_pairs = [
        ("Enter", f"{KBM_TEX_DIR}/T_Enter_Key_White"),
        ("Escape", f"{KBM_TEX_DIR}/T_Esc_Key_White"),
        ("SpaceBar", f"{KBM_TEX_DIR}/T_Space_Key_White"),
        ("Tab", f"{KBM_TEX_DIR}/T_Tab_Key_White"),
        ("BackSpace", f"{KBM_TEX_DIR}/T_BackSpace_Key_White"),
        ("LeftShift", f"{KBM_TEX_DIR}/T_Shift_Key_White"),
        ("RightShift", f"{KBM_TEX_DIR}/T_Shift_Key_White"),
        ("LeftControl", f"{KBM_TEX_DIR}/T_Crtl_Key_White"),
        ("RightControl", f"{KBM_TEX_DIR}/T_Crtl_Key_White"),
        ("LeftAlt", f"{KBM_TEX_DIR}/T_Alt_Key_White"),
        ("RightAlt", f"{KBM_TEX_DIR}/T_Alt_Key_White"),
        ("CapsLock", f"{KBM_TEX_DIR}/T_CapsLock_Key_White"),
        ("Delete", f"{KBM_TEX_DIR}/T_Del_Key_White"),
        ("Insert", f"{KBM_TEX_DIR}/T_Ins_Key_White"),
        ("Home", f"{KBM_TEX_DIR}/T_Home_Key_White"),
        ("End", f"{KBM_TEX_DIR}/T_End_Key_White"),
        ("PageUp", f"{KBM_TEX_DIR}/T_PageUp_Key_White"),
        ("PageDown", f"{KBM_TEX_DIR}/T_PageDown_Key_White"),
        ("Up", f"{KBM_TEX_DIR}/T_Up_Key_White"),
        ("Down", f"{KBM_TEX_DIR}/T_Down_Key_White"),
        ("Left", f"{KBM_TEX_DIR}/T_Left_Key_White"),
        ("Right", f"{KBM_TEX_DIR}/T_Right_Key_White"),
        ("LeftMouseButton", f"{KBM_TEX_DIR}/T_Mouse_Left_Key_White"),
        ("RightMouseButton", f"{KBM_TEX_DIR}/T_Mouse_Right_Key_White"),
        ("MiddleMouseButton", f"{KBM_TEX_DIR}/T_Mouse_Middle_Key_White"),
        ("MouseScrollUp", f"{KBM_TEX_DIR}/T_Mouse_Scroll_Up_Key_Dark_Key_White"),
        ("MouseScrollDown", f"{KBM_TEX_DIR}/T_Mouse_Scroll_Down_Key_Dark_Key_White"),
        ("Mouse2D", f"{KBM_TEX_DIR}/T_Mouse_XY_Key_White"),
        ("LeftBracket", f"{KBM_TEX_DIR}/T_Brackets_L_Key_White"),
        ("RightBracket", f"{KBM_TEX_DIR}/T_Brackets_R_Key_White"),
    ]
    for ch in "ABCDEFGHIJKLMNOPQRSTUVWXYZ":
        kbm_pairs.append((ch, f"{KBM_TEX_DIR}/T_{ch}_Key_White"))
    digit_names = [
        ("Zero", "0"), ("One", "1"), ("Two", "2"), ("Three", "3"), ("Four", "4"),
        ("Five", "5"), ("Six", "6"), ("Seven", "7"), ("Eight", "8"), ("Nine", "9"),
    ]
    for kname, d in digit_names:
        kbm_pairs.append((kname, f"{KBM_TEX_DIR}/T_{d}_Key_White"))
    for i in range(1, 13):
        kbm_pairs.append((f"F{i}", f"{KBM_TEX_DIR}/T_F{i}_Key_White"))

    build_controller_data(
        "BP_ControllerData_KBM",
        unreal.CommonInputType.MOUSE_AND_KEYBOARD,
        "", "", "",
        kbm_pairs,
    )

    # 2. Gamepad Generic / Xbox (Light monochrome white icons tinted Bone #E8E0D0)
    xg_pairs = [
        ("Gamepad_FaceButton_Bottom", f"{XG_TEX_DIR}/T_X_A_White_Light"),
        ("Gamepad_FaceButton_Right", f"{XG_TEX_DIR}/T_X_B_White_Light"),
        ("Gamepad_FaceButton_Left", f"{XG_TEX_DIR}/T_X_X_White_Light"),
        ("Gamepad_FaceButton_Top", f"{XG_TEX_DIR}/T_X_Y_White_Light"),
        ("Gamepad_LeftShoulder", f"{XG_TEX_DIR}/T_X_LB_Light"),
        ("Gamepad_RightShoulder", f"{XG_TEX_DIR}/T_X_RB_Light"),
        ("Gamepad_LeftTrigger", f"{XG_TEX_DIR}/T_X_LT_Light"),
        ("Gamepad_LeftTriggerAxis", f"{XG_TEX_DIR}/T_X_LT_Light"),
        ("Gamepad_RightTrigger", f"{XG_TEX_DIR}/T_X_RT_Light"),
        ("Gamepad_RightTriggerAxis", f"{XG_TEX_DIR}/T_X_RT_Light"),
        ("Gamepad_DPad_Up", f"{XG_TEX_DIR}/T_X_Dpad_Up_Light"),
        ("Gamepad_DPad_Down", f"{XG_TEX_DIR}/T_X_Dpad_Down_Light"),
        ("Gamepad_DPad_Left", f"{XG_TEX_DIR}/T_X_Dpad_Left_Light"),
        ("Gamepad_DPad_Right", f"{XG_TEX_DIR}/T_X_Dpad_Right_Light"),
        ("Gamepad_LeftThumbstick", f"{XG_TEX_DIR}/T_X_Left_Stick_Click_Light"),
        ("Gamepad_RightThumbstick", f"{XG_TEX_DIR}/T_X_Right_Stick_Click_Light"),
        ("Gamepad_Left2D", f"{XG_TEX_DIR}/T_X_L_2D_Light"),
        ("Gamepad_Right2D", f"{XG_TEX_DIR}/T_X_R_2D_Light"),
        ("Gamepad_LeftX", f"{XG_TEX_DIR}/T_X_L_X_Light"),
        ("Gamepad_LeftY", f"{XG_TEX_DIR}/T_X_L_Y_Light"),
        ("Gamepad_RightX", f"{XG_TEX_DIR}/T_X_R_X_Light"),
        ("Gamepad_RightY", f"{XG_TEX_DIR}/T_X_R_Y_Light"),
        ("Gamepad_Special_Left", f"{XG_TEX_DIR}/T_X_Share_Light"),
        ("Gamepad_Special_Right", f"{XG_TEX_DIR}/T_X_Share_Light"),
    ]
    build_controller_data(
        "BP_ControllerData_Gamepad_Generic",
        unreal.CommonInputType.GAMEPAD,
        "Generic", "Gamepad", "Xbox",
        xg_pairs,
    )

    # 3. Gamepad PS5 / DualSense (Light monochrome white icons tinted Bone #E8E0D0)
    p5_pairs = [
        ("Gamepad_FaceButton_Bottom", f"{P5_TEX_DIR}/T_P5_Cross_Light"),
        ("Gamepad_FaceButton_Right", f"{P5_TEX_DIR}/T_P5_Circle_Light"),
        ("Gamepad_FaceButton_Left", f"{P5_TEX_DIR}/T_P5_Square_Light"),
        ("Gamepad_FaceButton_Top", f"{P5_TEX_DIR}/T_P5_Triangle_Light"),
        ("Gamepad_LeftShoulder", f"{P5_TEX_DIR}/T_P5_L1_Light"),
        ("Gamepad_RightShoulder", f"{P5_TEX_DIR}/T_P5_R1_Light"),
        ("Gamepad_LeftTrigger", f"{P5_TEX_DIR}/T_P5_L2_Light"),
        ("Gamepad_LeftTriggerAxis", f"{P5_TEX_DIR}/T_P5_L2_Light"),
        ("Gamepad_RightTrigger", f"{P5_TEX_DIR}/T_P5_R2_Light"),
        ("Gamepad_RightTriggerAxis", f"{P5_TEX_DIR}/T_P5_R2_Light"),
        ("Gamepad_DPad_Up", f"{P5_TEX_DIR}/T_P5_Dpad_UP_Light"),
        ("Gamepad_DPad_Down", f"{P5_TEX_DIR}/T_P5_Dpad_Down_Light"),
        ("Gamepad_DPad_Left", f"{P5_TEX_DIR}/T_P5_Dpad_Left_Light"),
        ("Gamepad_DPad_Right", f"{P5_TEX_DIR}/T_P5_Dpad_Right_Light"),
        ("Gamepad_LeftThumbstick", f"{P5_TEX_DIR}/T_P5_Left_Stick_Click_Alt_Light"),
        ("Gamepad_RightThumbstick", f"{P5_TEX_DIR}/T_P5_Right_Stick_Click_Alt_Light"),
        ("Gamepad_Left2D", f"{P5_TEX_DIR}/T_P5_L_2D_Light"),
        ("Gamepad_Right2D", f"{P5_TEX_DIR}/T_P5_R_2D_Light"),
        ("Gamepad_LeftX", f"{P5_TEX_DIR}/T_P5_L_X_Light"),
        ("Gamepad_LeftY", f"{P5_TEX_DIR}/T_P5_L_Y_Light"),
        ("Gamepad_RightX", f"{P5_TEX_DIR}/T_P5_R_X_Light"),
        ("Gamepad_RightY", f"{P5_TEX_DIR}/T_P5_R_Y_Light"),
        ("Gamepad_Special_Left", f"{P5_TEX_DIR}/T_P5_Share_Light"),
        ("Gamepad_Special_Right", f"{P5_TEX_DIR}/T_P5_Options_Light"),
    ]
    build_controller_data(
        "BP_ControllerData_Gamepad_PS5",
        unreal.CommonInputType.GAMEPAD,
        "PS5", "DualSense", "PlayStation",
        p5_pairs,
    )


if __name__ == "__main__":
    dt = create_input_action_datatable()
    create_velmara_input_data(dt)
    create_all_controller_data()
    print("=== PHASE 2 INPUT DATA COMPLETE ===")
