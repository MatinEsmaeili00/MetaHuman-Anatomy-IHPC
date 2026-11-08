# 13 - Tighter pick radii now that the atlases have ~3x more samples (job 08 "samples_*" step):
#      muscles 3.0 -> 2.0 cm, bones 2.5 -> 1.5 cm, on BP_MH_CH_Abby8 and BP_Beau_Pawn.
import unreal
SDS = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
LIB = unreal.SubobjectDataBlueprintFunctionLibrary
for path in ["/Game/MetaHumans/MH_CH_Abby5/BP_MH_CH_Abby8", "/Game/MadisonTesting/BeauMHCharacter/BP_Beau_Pawn"]:
    bp = unreal.load_asset(path)
    for h in SDS.k2_gather_subobject_data_for_blueprint(bp):
        o = LIB.get_object(LIB.get_data(h))
        if isinstance(o, unreal.AnatomyPickerComp):
            r = 1.5 if str(o.get_editor_property("mesh_component_name")) == "SkeletonMesh" else 2.0
            o.set_editor_property("pick_radius", r)
            print(path.split("/")[-1], o.get_name(), "radius", r)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
