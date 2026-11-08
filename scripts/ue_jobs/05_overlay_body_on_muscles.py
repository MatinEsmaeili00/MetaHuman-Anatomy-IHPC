# 05 - Visual fit check: put Abby's Body (reference pose, no animation) on the muscle set's pivot (320, 80, 0),
#      with a see-through material, and point the editor camera at it from the front and the side.
#      MODE is substituted: "spawn" | "front" | "side" | "remove"
import unreal

MODE = "__MODE__"
LABEL = "ZZ_FitCheck_AbbyBody"
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
existing = [a for a in actors.get_all_level_actors() if a.get_actor_label() == LABEL]

if MODE == "spawn" and not existing:
    mesh = unreal.load_asset("/Game/MetaHumans/MH_CH_Abby5/Body/SKM_MH_CH_Abby5_BodyMesh")
    a = actors.spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector(320, 80, 0), unreal.Rotator(0, 0, 0))
    a.set_actor_label(LABEL)
    comp = a.skeletal_mesh_component
    comp.set_skeletal_mesh_asset(mesh)
    xray = unreal.load_asset("/MetaHumanAnatomyLayers/Anatomy/Materials/M_Anatomy_XRay")
    for i in range(comp.get_num_materials()):
        comp.set_material(i, xray)
    print("spawned", a.get_name())
elif MODE == "remove":
    for a in existing:
        actors.destroy_actor(a)
    print("removed", len(existing))
elif MODE in ("front", "side", "back"):
    les = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    # body faces +Y; front camera sits at +Y looking back along -Y
    if MODE == "front":
        loc, rot = unreal.Vector(320, 80 + 330, 85), unreal.Rotator(0, -90, 0)
    elif MODE == "back":
        loc, rot = unreal.Vector(320, 80 - 330, 85), unreal.Rotator(0, 90, 0)
    else:
        loc, rot = unreal.Vector(320 - 330, 80, 85), unreal.Rotator(0, 0, 0)
    les.set_level_viewport_camera_info(loc, rot)
    print("camera", MODE)
