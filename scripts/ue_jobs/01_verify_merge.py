# 01 - After the merge: load the merged assets, look for references to assets that don't exist,
#      compile the Blueprints / Widget Blueprints against the merged C++ and report their status.
import unreal

ar = unreal.AssetRegistryHelpers.get_asset_registry()
ROOTS = [
    "/Game/Blueprints/BP_PlayerController",
    "/Game/Blueprints/BP_GameMode",
    "/Game/Blueprints/BP_SpeedSlider",
    "/Game/MetaHumans/MH_CH_Abby5/BP_MH_CH_Abby8",
    "/Game/MadisonTesting/BeauMHCharacter/BP_Beau_Pawn",
    "/Game/Abby_Animation/Enhanced/ABP_Abby_Timeline",
    "/Game/MadisonTesting/BeauMHCharacter/Animation/ABP_Beau_Timeline",
    "/Game/Widget/WB_MuscleInfo",
    "/Game/Widget/WB_PawnSwitcher",
    "/Game/Widget/WB_AnatomyViewSwitcher",
    "/Game/UserInterface/IHPC/Widgets/WBP_IHPC_HUD",
    "/Game/UserInterface/IHPC/Widgets/WBP_IHPC_LayerSwitcher",
    "/Game/UserInterface/IHPC/Widgets/WBP_IHPC_Timeline",
]

opts = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True)
missing = {}
for root in ROOTS:
    deps = ar.get_dependencies(root, opts) or []
    for d in deps:
        d = str(d)
        if d.startswith("/Script/") or d.startswith("/Engine/"):
            continue
        if not unreal.EditorAssetLibrary.does_asset_exist(d) and not ar.get_assets_by_package_name(d):
            missing.setdefault(root, []).append(d)
print("MISSING REFERENCES:", missing if missing else "none")

# Compile every Blueprint above and report status
for root in ROOTS:
    asset = unreal.load_asset(root)
    if asset is None:
        print("LOAD FAILED", root)
        continue
    if isinstance(asset, unreal.Blueprint):
        unreal.BlueprintEditorLibrary.compile_blueprint(asset)
        gen = unreal.BlueprintEditorLibrary.generated_class(asset)
        print(f"{root.split('/')[-1]:32} compiled -> {gen.get_name() if gen else None}")
    else:
        print(f"{root.split('/')[-1]:32} {type(asset).__name__}")

# What the player controller is set up with
pc = unreal.load_asset("/Game/Blueprints/BP_PlayerController")
cdo = unreal.get_default_object(unreal.BlueprintEditorLibrary.generated_class(pc))
for prop in ["main_hud_widget_class", "main_hud_z_order", "anatomy_view_switcher_widget_class", "timeline_widget_class",
             "speed_slider_widget", "pawn_switcher_widget_class", "muscle_info_widget_class", "switchable_pawn_classes"]:
    try:
        v = cdo.get_editor_property(prop)
        if isinstance(v, unreal.Array):
            v = [x.get_name() if x else None for x in v]
        elif hasattr(v, "get_name"):
            v = v.get_name()
        print(f"  PC.{prop} = {v}")
    except Exception as e:
        print(f"  PC.{prop}: {e}")

gm = unreal.load_asset("/Game/Blueprints/BP_GameMode")
gcdo = unreal.get_default_object(unreal.BlueprintEditorLibrary.generated_class(gm))
print("GameMode default pawn:", gcdo.get_editor_property("default_pawn_class").get_name(),
      "| controller:", gcdo.get_editor_property("player_controller_class").get_name())
