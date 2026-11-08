# 14 - BP_MH_CH_Abby8 soft-referenced a PIE world (/Game/UEDPIE_0_Main), inherited from the Abby branch: an actor
#      reference saved into a component default while Play-In-Editor was running. Blueprint defaults can't point at
#      level actors, so clear actor references held by component templates and the class defaults.
import unreal
SDS = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
LIB = unreal.SubobjectDataBlueprintFunctionLibrary
for path in ["/Game/MetaHumans/MH_CH_Abby5/BP_MH_CH_Abby8", "/Game/MadisonTesting/BeauMHCharacter/BP_Beau_Pawn"]:
    bp = unreal.load_asset(path)
    changed = False
    for h in SDS.k2_gather_subobject_data_for_blueprint(bp):
        o = LIB.get_object(LIB.get_data(h))
        if isinstance(o, unreal.CPP_MouseFunctionComp):
            refs = o.get_editor_property("player_place_holders")
            print(path.split("/")[-1], o.get_name(), "PlayerPlaceHolders:", [r.get_path_name() if r else None for r in refs])
            if refs:
                o.set_editor_property("player_place_holders", [])
                changed = True
    if changed:
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
        print("  cleared + saved")
ar = unreal.AssetRegistryHelpers.get_asset_registry()
opts = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True)
print("Abby8 deps containing UEDPIE:", [str(d) for d in ar.get_dependencies("/Game/MetaHumans/MH_CH_Abby5/BP_MH_CH_Abby8", opts) if "UEDPIE" in str(d)])
