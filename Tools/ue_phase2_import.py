"""
Velmara UI — Phase 2 Step A: Batch Import Textures (77 PNGs) & Fonts (Cinzel + Forum)
Executed inside Unreal Engine 5.5 Editor via EngineWire MCP execute_python.
"""

import json
import os
import unreal

ROOT = r"i:\UnrealProjects\TesisUE"
PROC_DIR = os.path.join(ROOT, "UI_Processed")
MANIFEST_PATH = os.path.join(PROC_DIR, "manifest.json")
FONTS_DISK_DIR = os.path.join(ROOT, "Content", "Fonts")

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary


def import_textures():
    with open(MANIFEST_PATH, "r", encoding="utf-8") as f:
        manifest = json.load(f)

    tasks = []
    expected_paths = []
    for entry in manifest["assets"]:
        src_file = os.path.join(PROC_DIR, entry["rel_path"].replace("/", os.sep))
        dest_dir = f"/Game/UI/Textures/{entry['category']}"
        asset_name = entry["name"]

        task = unreal.AssetImportTask()
        task.filename = src_file
        task.destination_path = dest_dir
        task.destination_name = asset_name
        task.replace_existing = True
        task.replace_existing_settings = True
        task.automated = True
        task.save = False
        tasks.append(task)
        expected_paths.append((f"{dest_dir}/{asset_name}", entry))

    print(f"Importing {len(tasks)} UI textures...")
    asset_tools.import_asset_tasks(tasks)

    ok_count = 0
    failed = []
    for asset_path, entry in expected_paths:
        tex = unreal.load_asset(asset_path)
        if not tex or not isinstance(tex, unreal.Texture2D):
            failed.append(asset_path)
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
        ok_count += 1

    print(f"Textures imported & configured: {ok_count}/{len(expected_paths)}")
    if failed:
        print("FAILED TEXTURES:", failed)


def import_fonts():
    font_files = [
        ("Cinzel-Regular.ttf", "Cinzel-Regular"),
        ("Cinzel-SemiBold.ttf", "Cinzel-SemiBold"),
        ("Cinzel-Bold.ttf", "Cinzel-Bold"),
        ("Forum-Regular.ttf", "Forum-Regular"),
    ]
    tasks = []
    for fname, aname in font_files:
        task = unreal.AssetImportTask()
        task.filename = os.path.join(FONTS_DISK_DIR, fname)
        task.destination_path = "/Game/Fonts"
        task.destination_name = aname
        task.replace_existing = True
        task.replace_existing_settings = True
        task.automated = True
        task.save = True
        tasks.append(task)

    print("Importing FontFace assets (Cinzel & Forum)...")
    asset_tools.import_asset_tasks(tasks)

    # Check what UE created in /Game/Fonts for Cinzel & Forum
    for p in sorted(eal.list_assets("/Game/Fonts", recursive=False)):
        if "Cinzel" in p or "Forum" in p:
            obj = unreal.load_asset(p)
            print(f"  Font asset: {p} ({type(obj).__name__})")

    # Build clean composite UFont assets: /Game/Fonts/Cinzel_Font (Regular, SemiBold, Bold)
    # and /Game/Fonts/Forum_Font (Regular) by creating Font assets via FontFactory
    # and populating composite_font via export/import text (100% reliable in UE5 Python).
    def make_composite_font(font_name, faces_map):
        pkg_path = f"/Game/Fonts/{font_name}"
        if eal.does_asset_exist(pkg_path):
            font_obj = unreal.load_asset(pkg_path)
        else:
            factory = unreal.FontFactory()
            font_obj = asset_tools.create_asset(font_name, "/Game/Fonts", unreal.Font, factory)

        font_obj.set_editor_property("font_cache_type", unreal.FontCacheType.RUNTIME)

        # Build CompositeFont text representation matching Unreal's native UFont serialization
        entries_txt = []
        for typeface_name, face_asset_path in faces_map:
            face_obj = unreal.load_asset(face_asset_path)
            if not face_obj:
                raise RuntimeError(f"Missing FontFace: {face_asset_path}")
            face_ref = face_obj.get_path_name()
            entries_txt.append(
                f'(Name="{typeface_name}",Font=(FontFilename="",Hinting=Default,LoadingPolicy=LazyLoad,FontFaceAsset=FontFace\'"{face_ref}"\'))'
            )
        comp_txt = f'(DefaultTypeface=(Fonts=({",".join(entries_txt)})),FallbackTypeface=(Typeface=(Fonts=()),ScalingFactor=1.000000),SubTypefaces=())'

        comp = unreal.CompositeFont()
        comp.import_text(comp_txt)
        font_obj.set_editor_property("composite_font", comp)
        eal.save_loaded_asset(font_obj, only_if_is_dirty=False)
        print(f"  Saved composite UFont: {pkg_path} -> export: {font_obj.get_editor_property('composite_font').export_text()[:260]}")

    make_composite_font(
        "Cinzel_Font",
        [
            ("Regular", "/Game/Fonts/Cinzel-Regular"),
            ("SemiBold", "/Game/Fonts/Cinzel-SemiBold"),
            ("Bold", "/Game/Fonts/Cinzel-Bold"),
        ],
    )
    make_composite_font(
        "Forum_Font",
        [
            ("Regular", "/Game/Fonts/Forum-Regular"),
        ],
    )


def inspect_platform_ini_example():
    # Check exact PlatformSettings INI naming in UE 5.5 engine config
    eng_cfg = os.path.abspath(os.path.join(unreal.Paths.engine_plugins_dir(), "Runtime/CommonUI/Config"))
    if os.path.exists(eng_cfg):
        for root, _, files in os.walk(eng_cfg):
            for fn in files:
                fp = os.path.join(root, fn)
                print(f"=== {os.path.relpath(fp, eng_cfg)} ===")
                print(open(fp, encoding="utf-8", errors="ignore").read()[:1200])


if __name__ == "__main__":
    import_textures()
    import_fonts()
    inspect_platform_ini_example()
    print("=== PHASE 2 IMPORT COMPLETE ===")
