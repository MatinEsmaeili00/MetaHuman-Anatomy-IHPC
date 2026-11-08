# Retarget the Madison action clips (Abby's skeleton) to the MetaHuman Character skeleton (metahuman_base_skel),
# used by Beau, Madi (and Grant). Output: /Game/MadisonAnimations/MetaHuman/<name>_MH
import unreal
ar = unreal.AssetRegistryHelpers.get_asset_registry()
src_mesh = unreal.load_asset("/Game/MetaHumans/MH_CH_Abby5/Body/SKM_MH_CH_Abby5_BodyMesh")
tgt_mesh = unreal.load_asset("/Game/MadisonTesting/BeauMHCharacter/Unpacked/Beau/Body/SKM_MHC_Beau_BodyMesh")
rtg = unreal.load_asset("/MetaHumanCharacter/Animation/Retargeting/RTG_MH_IKRig")
names = ["AS_Running", "AS_MP_MocapTennisTest", "AS_SoccerFeintWarmup"]
assets = [ar.get_asset_by_object_path(f"/Game/MadisonAnimations/{n}.{n}") for n in names]
print([str(a.package_name) for a in assets])
res = unreal.IKRetargetBatchOperation.duplicate_and_retarget(assets, src_mesh, tgt_mesh, rtg, "", "", "", "_MH",
        "/Game/MadisonAnimations/MetaHuman", False, False, True)
for r in res:
    a = r.get_asset()
    if isinstance(a, unreal.AnimSequence):
        print(r.package_name, round(a.get_play_length(),3), a.get_editor_property('skeleton').get_name())
        unreal.EditorAssetLibrary.save_loaded_asset(a, False)
    else:
        print(r.package_name, type(a).__name__)
