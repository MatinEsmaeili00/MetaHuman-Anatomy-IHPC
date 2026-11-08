# 07 - Abby's Body skeleton in the reference pose (component space): every bone's name, parent and world transform.
#      Written to Saved/ClaudeBridge/abby_ref_pose.json; the skeleton-fitting step maps the Skelly bones onto these joints.
import json, os
import unreal

mesh = unreal.load_asset("/Game/MetaHumans/MH_CH_Abby5/Body/SKM_MH_CH_Abby5_BodyMesh")
dm = unreal.DynamicMesh()
dm, _ = unreal.GeometryScript_AssetUtils.copy_mesh_from_skeletal_mesh(
    mesh, dm, unreal.GeometryScriptCopyMeshFromAssetOptions(), unreal.GeometryScriptMeshReadLOD())
_, bones = unreal.GeometryScript_BoneWeights.get_all_bones_info(dm)
rows = []
for b in bones:
    t = b.world_transform
    q = t.rotation
    rows.append({"name": str(b.name), "index": b.index, "parent": b.parent_index,
                 "pos": [t.translation.x, t.translation.y, t.translation.z],
                 "quat": [q.x, q.y, q.z, q.w]})
path = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "ClaudeBridge", "abby_ref_pose.json")
json.dump(rows, open(path, "w"), indent=1)
print(len(rows), "bones ->", path)
print([r["name"] for r in rows][:80])
