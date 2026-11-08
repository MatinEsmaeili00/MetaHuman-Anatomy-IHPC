# 11 - Wire the segmented layers into the characters (BP_MH_CH_Abby8, BP_Beau_Pawn):
#      MuscleMesh   -> SKM_Anatomy_Muscles_Segmented   (Leader Pose on Body, as before)
#      SkeletonMesh -> SKM_Anatomy_Skeleton_Segmented
#      AnatomyPicker (muscles) -> DA_Atlas_Muscles_Segmented
#      BonePicker (new UAnatomyPickerComp) -> DA_Atlas_Skeleton_Segmented on SkeletonMesh, active in the Skeleton layer
#      MODE: "inspect" | "apply"
import unreal

MODE = "__MODE__"
SEG = "/MetaHumanAnatomyLayers/Anatomy/Segmented"
BPS = ["/Game/MetaHumans/MH_CH_Abby5/BP_MH_CH_Abby8", "/Game/MadisonTesting/BeauMHCharacter/BP_Beau_Pawn"]
SDS = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
LIB = unreal.SubobjectDataBlueprintFunctionLibrary


def templates(bp):
    out = {}
    for h in SDS.k2_gather_subobject_data_for_blueprint(bp):
        d = LIB.get_data(h)
        o = LIB.get_object(d)
        if o:
            out[o.get_name().replace("_GEN_VARIABLE", "")] = (h, o)
    return out


for path in BPS:
    bp = unreal.load_asset(path)
    comps = templates(bp)
    print("==", path.split("/")[-1])
    for name, (h, o) in comps.items():
        if isinstance(o, (unreal.SkinnedMeshComponent, unreal.AnatomyViewComp, unreal.AnatomyPickerComp)):
            extra = ""
            if isinstance(o, unreal.SkinnedMeshComponent):
                m = o.get_editor_property("skinned_asset") if hasattr(o, "skinned_asset") else None
                m = m or (o.get_editor_property("skeletal_mesh_asset") if isinstance(o, unreal.SkeletalMeshComponent) else None)
                extra = f"mesh={m.get_name() if m else None} overrides={len(o.get_editor_property('override_materials'))} tags={list(o.component_tags)}"
            if isinstance(o, unreal.AnatomyPickerComp):
                a = o.get_editor_property("atlas")
                extra = f"atlas={a} mesh={o.get_editor_property('mesh_component_name')} layer={o.get_editor_property('required_layer_name')} radius={o.get_editor_property('pick_radius')}"
            if isinstance(o, unreal.AnatomyViewComp):
                extra = "layers=" + str([(str(l.layer_name), [str(x) for x in l.component_names_or_tags]) for l in o.get_editor_property("layers")])
            print(f"  {name:22} {type(o).__name__:28} {extra}")

    if MODE != "apply":
        continue

    muscles = unreal.load_asset(f"{SEG}/SKM_Anatomy_Muscles_Segmented")
    skeleton = unreal.load_asset(f"{SEG}/SKM_Anatomy_Skeleton_Segmented")
    for name, mesh in (("MuscleMesh", muscles), ("SkeletonMesh", skeleton)):
        h, o = comps[name]
        o.set_editor_property("skeletal_mesh_asset", mesh)
        o.set_editor_property("override_materials", [])
    picker = comps["AnatomyPicker"][1]
    picker.set_editor_property("atlas", unreal.load_asset(f"{SEG}/DA_Atlas_Muscles_Segmented"))
    picker.set_editor_property("pick_radius", 3.0)

    if "BonePicker" not in comps:
        root_handle = SDS.k2_gather_subobject_data_for_blueprint(bp)[0]
        params = unreal.AddNewSubobjectParams(parent_handle=root_handle, new_class=unreal.AnatomyPickerComp, blueprint_context=bp)
        new_handle, fail = SDS.add_new_subobject(params)
        SDS.rename_subobject(new_handle, unreal.Text("BonePicker"))
        comps = templates(bp)
    bone_picker = comps["BonePicker"][1]
    bone_picker.set_editor_property("atlas", unreal.load_asset(f"{SEG}/DA_Atlas_Skeleton_Segmented"))
    bone_picker.set_editor_property("mesh_component_name", "SkeletonMesh")
    bone_picker.set_editor_property("required_layer_name", "Skeleton")
    bone_picker.set_editor_property("highlight_material", picker.get_editor_property("highlight_material"))
    bone_picker.set_editor_property("pick_radius", 2.5)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    print("  saved:", unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False))
