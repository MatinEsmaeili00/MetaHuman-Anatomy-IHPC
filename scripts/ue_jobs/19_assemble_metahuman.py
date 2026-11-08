# 19 - Assemble a MetaHuman Character asset (MHC_*) from Python, like the MetaHuman Character editor's Assemble button:
#      Cinematic pipeline, output in <character folder>/Unpacked/<MHC name>/ (BP_<MHC name>, Body, Face, Grooms ...).
#      If the character has no texture sources yet, requests them first (the "Download Texture Sources" button):
#      that needs you to be logged in to the MetaHuman cloud service in the editor (MetaHuman Character editor ->
#      log in / Auto-rig once). Madi was assembled with this; Grant needs the login first (2026-11-08: EOS_InvalidAuth).
#      Parameters: MHC (asset path), e.g. /Game/MadisonTesting/GrantMHCharacter/MHC_Grant
import unreal

MHC = "__MHC__"
sub = unreal.get_editor_subsystem(unreal.MetaHumanCharacterEditorSubsystem)
character = unreal.load_asset(MHC)
folder = MHC.rsplit("/", 1)[0]
added = sub.try_add_object_to_edit(character) if not sub.is_object_added_for_editing(character) else False
try:
    if not sub.can_build_meta_human(character, False):
        print("not ready to assemble - requesting texture sources (needs the MetaHuman cloud login)")
        req = unreal.MetaHumanCharacterTextureRequestParams()
        req.set_editor_property("report_progress", False)
        req.set_editor_property("blocking", True)
        sub.request_texture_sources(character, req)
    if not sub.can_build_meta_human(character, True):   # logs the reason
        raise SystemExit(f"{MHC} still can't be assembled - see the Output Log (LogMetaHumanCharacterEditor). "
                         "Usually: log in to the MetaHuman service, then Download Texture Sources in the MetaHuman Character editor.")
    params = unreal.MetaHumanCharacterEditorBuildParameters()
    params.set_editor_property("pipeline_type", unreal.MetaHumanDefaultPipelineType.CINEMATIC)
    params.set_editor_property("pipeline_quality", unreal.MetaHumanQualityLevel.CINEMATIC)
    params.set_editor_property("absolute_build_path", f"{folder}/Unpacked")
    sub.build_meta_human(character, params)
    dirty = [p for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages() if p.get_name().startswith(f"{folder}/")]
    print("assembled; saving", len(dirty), "packages:", unreal.EditorLoadingAndSavingUtils.save_packages(dirty, True))
    reg = unreal.AssetRegistryHelpers.get_asset_registry()
    bps = [str(a.package_name) for a in reg.get_assets_by_path(f"{folder}/Unpacked", recursive=True)
           if str(a.asset_class_path.asset_name) == "Blueprint"]
    print("assembled Blueprint:", bps)
finally:
    if added:
        sub.remove_object_to_edit(character)
