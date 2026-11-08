# 09 - Fit check of the segmented layers in the reference pose: Abby's body + face (x-ray) with the segmented
#      skeleton at X = 0 and with the segmented muscles at X = 150, both at Y = -600. MODE: "spawn" | "remove"
import unreal

MODE = "__MODE__"
TAG = "ZZ_FitCheck_"
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in actors.get_all_level_actors():
    if a.get_actor_label().startswith(TAG):
        actors.destroy_actor(a)
if MODE == "spawn":
    bp = unreal.load_asset("/Game/MetaHumans/MH_CH_Abby5/BP_MH_CH_Abby8")
    subsys = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    face_mesh = None
    for h in subsys.k2_gather_subobject_data_for_blueprint(bp):
        o = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h))
        if o and o.get_name().startswith("Face") and isinstance(o, unreal.SkeletalMeshComponent):
            face_mesh = o.get_editor_property("skeletal_mesh_asset")
    body = unreal.load_asset("/Game/MetaHumans/MH_CH_Abby5/Body/SKM_MH_CH_Abby5_BodyMesh")
    xray = unreal.load_asset("/MetaHumanAnatomyLayers/Anatomy/Materials/M_Anatomy_XRay")
    layers = {"Skeleton": (0, "/MetaHumanAnatomyLayers/Anatomy/Segmented/SKM_Anatomy_Skeleton_Segmented"),
              "Muscles": (150, "/MetaHumanAnatomyLayers/Anatomy/Segmented/SKM_Anatomy_Muscles_Segmented")}
    for name, (x, path) in layers.items():
        loc = unreal.Vector(x, -600, 0)
        for label, mesh, mat in ((name, unreal.load_asset(path), None), (name + "_Body", body, xray), (name + "_Face", face_mesh, xray)):
            if mesh is None:
                continue
            a = actors.spawn_actor_from_class(unreal.SkeletalMeshActor, loc, unreal.Rotator(0, 0, 0))
            a.set_actor_label(TAG + label)
            c = a.skeletal_mesh_component
            c.set_skeletal_mesh_asset(mesh)
            if mat:
                for i in range(c.get_num_materials()):
                    c.set_material(i, mat)
            b = mesh.get_bounds()
            print(label, mesh.get_name(), "bounds z %.1f..%.1f x %.1f..%.1f" % (
                b.origin.z - b.box_extent.z, b.origin.z + b.box_extent.z, b.origin.x - b.box_extent.x, b.origin.x + b.box_extent.x))
print("done", MODE)
