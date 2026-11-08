# 04 - Measure Abby's Body (reference pose, component space) against the segmented muscle set in Main,
#      to find the transform that puts the muscles on Abby.
import unreal

GS = unreal
bp = unreal.load_asset("/Game/MetaHumans/MH_CH_Abby5/BP_MH_CH_Abby8")
gen = unreal.BlueprintEditorLibrary.generated_class(bp)

# Body component template in the Blueprint (SCS)
subsys = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
handles = subsys.k2_gather_subobject_data_for_blueprint(bp)
body_tmpl = None
for h in handles:
    obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h))
    if obj and obj.get_name().startswith("Body") and isinstance(obj, unreal.SkeletalMeshComponent):
        body_tmpl = obj
        break
mesh = body_tmpl.get_editor_property("skeletal_mesh_asset")
print("Body mesh:", mesh.get_path_name())
print("Body relative transform:", body_tmpl.get_editor_property("relative_location"), body_tmpl.get_editor_property("relative_rotation"),
      body_tmpl.get_editor_property("relative_scale3d"))
print("Body skeleton:", mesh.get_editor_property("skeleton").get_path_name())
b = mesh.get_bounds()
print("Body mesh bounds origin", b.origin, "extent", b.box_extent)

# Reference-pose joint positions (component space)
dm = unreal.DynamicMesh()
opts = unreal.GeometryScriptCopyMeshFromAssetOptions()
req = unreal.GeometryScriptMeshReadLOD()
dm, outcome = unreal.GeometryScript_AssetUtils.copy_mesh_from_skeletal_mesh(mesh, dm, opts, req)
print("copy body ->", outcome, "verts:", unreal.GeometryScript_MeshQueries.get_vertex_count(dm))
bbox = unreal.GeometryScript_MeshQueries.get_mesh_bounding_box(dm)
print("Body dynamic-mesh bbox min", bbox.min, "max", bbox.max)

names = ["pelvis", "spine_01", "spine_05", "neck_01", "head", "clavicle_l", "upperarm_l", "lowerarm_l", "hand_l",
         "clavicle_r", "upperarm_r", "hand_r", "thigh_l", "calf_l", "foot_l", "ball_l", "thigh_r", "foot_r"]
for n in names:
    _, found, info = unreal.GeometryScript_BoneWeights.get_bone_info(dm, n)
    if found:
        t = info.world_transform.translation
        print(f"  {n:12} ({t.x:7.2f}, {t.y:7.2f}, {t.z:7.2f})")

# The muscle set in the level
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
acts = [a for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.StaticMeshActor)
        if str(a.get_folder_path()) == "Muscles"]
lo = [1e9] * 3; hi = [-1e9] * 3
for a in acts:
    o, e = a.get_actor_bounds(False)
    for i, k in enumerate("xyz"):
        lo[i] = min(lo[i], getattr(o, k) - getattr(e, k)); hi[i] = max(hi[i], getattr(o, k) + getattr(e, k))
print("muscles:", len(acts), "world bbox min", [round(v, 2) for v in lo], "max", [round(v, 2) for v in hi])
xf = {(str(a.get_actor_location()), str(a.get_actor_rotation()), str(a.get_actor_scale3d())) for a in acts}
print("distinct muscle actor transforms:", len(xf), list(xf)[:2])
abby = [a for a in unreal.GameplayStatics.get_all_actors_of_class(world, gen)]
for a in abby:
    print("Abby8 in level:", a.get_actor_label(), a.get_actor_location(), a.get_actor_rotation())
