# 17 - Actions menu + layer buttons, for EVERY pawn in BP_PlayerController > Switchable Pawn Classes that has an
#      AnimationTimelineComp (run after building the C++ with UAnimationTimelineComp::ActionClips, and again after
#      adding a pawn with job 16):
#      * AnimationTimelineComp > Action Clips: Running / TennisForehand / SoccerWarmup -> the Madison clips, picked by
#        the Body mesh's skeleton:
#          Abby's skeleton (SKM_MH_CH_Abby5_BodyMesh_Skeleton) -> /Game/MadisonAnimations/AS_*             (originals)
#          metahuman_base_skel (Beau, Madi, Grant ...)        -> /Game/MadisonAnimations/MetaHuman/AS_*_MH (job 15)
#      * AnatomyView > Layers[Skin].FadeLength = 1 (= Muscles), so the layer buttons' skin amounts 1 / 0.5 / 0 land
#        exactly on skin / muscles / skeleton.
#      New action: add  "<ButtonSuffix>": "<clip name>"  to ACTIONS (and a button Action_<ButtonSuffix> in WBP_IHPC_HUD),
#      retarget the clip with job 15, run this job.
import unreal

ACTIONS = {"Running": "AS_Running", "TennisForehand": "AS_MP_MocapTennisTest", "SoccerWarmup": "AS_SoccerFeintWarmup"}
CONTROLLER = "/Game/Blueprints/BP_PlayerController"
ABBY_SKELETON = "SKM_MH_CH_Abby5_BodyMesh_Skeleton"
SDS = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
LIB = unreal.SubobjectDataBlueprintFunctionLibrary


def clip_for(name, skeleton_name):
    path = f"/Game/MadisonAnimations/{name}" if skeleton_name == ABBY_SKELETON else f"/Game/MadisonAnimations/MetaHuman/{name}_MH"
    a = unreal.load_asset(path)
    if not a:
        raise RuntimeError("missing " + path + " (run job 15 to retarget it)")
    if a.get_editor_property("skeleton").get_name() != skeleton_name:
        raise RuntimeError(f"{path} is for {a.get_editor_property('skeleton').get_name()}, the pawn uses {skeleton_name}")
    return a


cdo = unreal.get_default_object(unreal.load_asset(CONTROLLER).generated_class())
for cls in cdo.get_editor_property("switchable_pawn_classes"):
    bp = unreal.load_asset(cls.get_path_name().rsplit(".", 1)[0]) if cls else None
    if not bp:
        continue
    comps = {}
    for h in SDS.k2_gather_subobject_data_for_blueprint(bp):
        o = LIB.get_object(LIB.get_data(h))
        if o:
            comps[o.get_name().replace("_GEN_VARIABLE", "")] = o
    timeline = next((o for o in comps.values() if isinstance(o, unreal.AnimationTimelineComp)), None)
    body = comps.get("Body")
    if not timeline or not isinstance(body, unreal.SkeletalMeshComponent) or not body.get_editor_property("skeletal_mesh_asset"):
        print(bp.get_name(), "- no timeline / Body mesh, skipped (its Action buttons are greyed out)")
        continue
    skeleton = body.get_editor_property("skeletal_mesh_asset").get_editor_property("skeleton").get_name()
    timeline.set_editor_property("action_clips", {unreal.Name(k): clip_for(v, skeleton) for k, v in ACTIONS.items()})
    view = next((o for o in comps.values() if isinstance(o, unreal.AnatomyViewComp)), None)
    if view:
        layers = list(view.get_editor_property("layers"))
        if layers:
            layers[0].set_editor_property("fade_length", 1.0)
            view.set_editor_property("layers", layers)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    ok = unreal.EditorAssetLibrary.save_loaded_asset(bp, False)
    print(bp.get_name(), "saved", ok, "| skeleton", skeleton, "| actions:",
          {str(k): v.get_name() for k, v in timeline.get_editor_property("action_clips").items()},
          "| fade lengths:", [round(l.get_editor_property("fade_length"), 2) for l in view.get_editor_property("layers")] if view else None)
