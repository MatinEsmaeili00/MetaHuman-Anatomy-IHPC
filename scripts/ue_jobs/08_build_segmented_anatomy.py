# 08 - Build the SEGMENTED anatomy layers as skeletal meshes that follow Abby through Leader Pose.
#
#   STEP = "muscles"  : the 186 SM_SM_* muscle actors in Main (folder "Muscles"), moved from their pivot (320, 80, 0)
#                       onto Abby's reference pose, skinned with weights transferred from Abby's body (smooth).
#   STEP = "skeleton" : the fitted Skelly_Base skeleton (scripts/blender_jobs/01_export_fitted_skeleton.py), imported
#                       from FBX, every bone piece bound 100 % to one MetaHuman bone (rigid).
#
# For both: one part ID per muscle / bone in the vertex colour (R = (id + 0.5) / 255, the convention the
# anatomy picker and highlight material already use), a single material section, the skeleton = a plugin copy of
# Abby's body skeleton (MetaHuman bone names -> Leader Pose), and picking samples (points on each part, stored
# relative to a bone in the reference pose) written to Saved/ClaudeBridge/segmented_<step>.json for the atlas job.
import json
import math
import os
import time

import unreal

STEP = "__STEP__"
REPO = "/Users/mesmae6/GitHub/MetaHuman-Anatomy-IHPC"
SAVED = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "ClaudeBridge")
OUT_DIR = "/MetaHumanAnatomyLayers/Anatomy/Segmented"
BODY_PATH = "/Game/MetaHumans/MH_CH_Abby5/Body/SKM_MH_CH_Abby5_BodyMesh"
SKELETON_COPY = OUT_DIR + "/SK_Anatomy_MetaHumanBody"
MUSCLE_PIVOT = unreal.Vector(320.0, 80.0, 0.0)
MAT_MUSCLE = "/MetaHumanAnatomyLayers/Anatomy/Materials/M_Anatomy_Muscle"
MAT_BONE = "/MetaHumanAnatomyLayers/Anatomy/Materials/MI_Anatomy_Bone"

AU = unreal.GeometryScript_AssetUtils
NAU = unreal.GeometryScript_NewAssetUtils
BW = unreal.GeometryScript_BoneWeights
Q = unreal.GeometryScript_MeshQueries
E = unreal.GeometryScript_MeshEdits
MAT = unreal.GeometryScript_Materials
VC = unreal.GeometryScript_VertexColors
L = unreal.GeometryScript_List
XF = unreal.GeometryScript_MeshTransforms
EAL = unreal.EditorAssetLibrary
t0 = time.time()


def log(*a):
    print("[%5.1fs]" % (time.time() - t0), *a)


# ---- Abby's body (reference pose, bones + weights) -----------------------------------------------------------------
body = unreal.load_asset(BODY_PATH)
body_dm = unreal.DynamicMesh()
body_dm, _ = AU.copy_mesh_from_skeletal_mesh(body, body_dm, unreal.GeometryScriptCopyMeshFromAssetOptions(),
                                             unreal.GeometryScriptMeshReadLOD())
_, bone_infos = BW.get_all_bones_info(body_dm)
BONES = {str(b.name): b for b in bone_infos}
BONE_NAMES = [str(b.name) for b in sorted(bone_infos, key=lambda b: b.index)]

if not EAL.does_asset_exist(SKELETON_COPY):
    EAL.duplicate_asset(body.get_editor_property("skeleton").get_path_name(), SKELETON_COPY)
skeleton = unreal.load_asset(SKELETON_COPY)
log("body", Q.get_vertex_count(body_dm), "verts,", len(BONES), "bones; skeleton", skeleton.get_path_name())


def vec(v):
    return [round(v.x, 4), round(v.y, 4), round(v.z, 4)]


def farthest_samples(points, n):
    """Spread n sample indices over a part (farthest point sampling)."""
    if len(points) <= n:
        return list(range(len(points)))
    cx = sum(p[0] for p in points) / len(points); cy = sum(p[1] for p in points) / len(points); cz = sum(p[2] for p in points) / len(points)
    first = max(range(len(points)), key=lambda i: (points[i][0] - cx) ** 2 + (points[i][1] - cy) ** 2 + (points[i][2] - cz) ** 2)
    chosen = [first]
    d = [(p[0] - points[first][0]) ** 2 + (p[1] - points[first][1]) ** 2 + (p[2] - points[first][2]) ** 2 for p in points]
    while len(chosen) < n:
        i = max(range(len(points)), key=d.__getitem__)
        chosen.append(i)
        pi = points[i]
        for j, p in enumerate(points):
            dd = (p[0] - pi[0]) ** 2 + (p[1] - pi[1]) ** 2 + (p[2] - pi[2]) ** 2
            if dd < d[j]:
                d[j] = dd
    return chosen


def bone_offset(bone_name, pos):
    xf = BONES[bone_name].world_transform
    return xf.inverse_transform_location(unreal.Vector(*pos))


def region_for(bone):
    b = bone.lower()
    for keys, reg in ((("thumb", "index", "middle", "ring", "pinky", "hand"), "Hand"), (("lowerarm",), "Forearm"),
                      (("upperarm",), "Upper arm"), (("clavicle",), "Shoulder"), (("ball", "foot"), "Foot"),
                      (("calf",), "Lower leg"), (("thigh",), "Thigh / hip"), (("pelvis",), "Hip / pelvis"),
                      (("spine_04", "spine_05"), "Chest / upper back"), (("spine_01", "spine_02", "spine_03"), "Abdomen / lower back"),
                      (("neck", "head"), "Neck")):
        if any(k in b for k in keys):
            return reg
    return "Body"


def finish(dm, part_of_vertex, parts, asset_name, material_path, rigid_bone_of_part=None, samples_per_part=24):
    """Weights, IDs in vertex colours, one material, skeletal mesh asset, picking samples."""
    _, vids, _ = Q.get_all_vertex_i_ds(dm) if hasattr(Q, "get_all_vertex_i_ds") else (None, None, None)
    nverts = Q.get_vertex_count(dm)
    _, plist, _ = Q.get_all_vertex_positions(dm, False)
    positions = [(v.x, v.y, v.z) for v in L.convert_vector_list_to_array(plist)]

    # bone weights
    if rigid_bone_of_part is None:
        opts = unreal.GeometryScriptTransferBoneWeightsOptions()
        opts.transfer_method = unreal.TransferBoneWeightsMethod.CLOSEST_POINT_ON_SURFACE
        opts.output_target_mesh_bones = unreal.OutputTargetMeshBones.SOURCE_BONES
        BW.transfer_bone_weights_from_mesh(body_dm, dm, opts)
        log("weights transferred from the body (smooth)")
    else:
        BW.copy_bones_from_mesh(body_dm, dm)
        BW.mesh_create_bone_weights(dm, True)
        bone_index = {}
        for pid, bone in rigid_bone_of_part.items():
            _, ok, idx = BW.get_bone_index(dm, bone)
            if not ok:
                raise RuntimeError(f"bone {bone} not in skeleton")
            bone_index[pid] = idx
        for vid in range(nverts):
            pid = part_of_vertex[vid]
            w = unreal.GeometryScriptBoneWeight()
            w.bone_index = bone_index[pid]
            w.weight = 1.0
            BW.set_vertex_bone_weights(dm, vid, [w])
        log("rigid weights set on", nverts, "vertices")

    # part IDs -> vertex colour red channel
    colors = [unreal.LinearColor(((part_of_vertex[v] + 0.5) / 255.0) if part_of_vertex[v] else 0.0, 0.0, 0.0, 1.0)
              for v in range(nverts)]
    VC.set_mesh_per_vertex_colors(dm, L.convert_array_to_color_list(colors))
    MAT.clear_material_i_ds(dm, 0) if hasattr(MAT, "clear_material_i_ds") else None

    # skeletal mesh asset
    path = f"{OUT_DIR}/{asset_name}"
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)
    o = unreal.GeometryScriptCreateNewSkeletalMeshAssetOptions()
    o.enable_recompute_normals = False
    o.enable_recompute_tangents = True
    o.materials = {"Anatomy": unreal.load_asset(material_path)}
    skm, outcome = NAU.create_new_skeletal_mesh_asset_from_mesh(dm, skeleton, path, o)
    log("created", path, outcome)
    EAL.save_asset(path)

    # picking samples per part
    verts_of_part = {}
    for v in range(nverts):
        verts_of_part.setdefault(part_of_vertex[v], []).append(v)
    for p in parts:
        vs = verts_of_part.get(p["id"], [])
        pts = [positions[v] for v in vs]
        n = min(samples_per_part, max(4, len(vs) // 40))
        picks = farthest_samples(pts, n) if pts else []
        samples, bone_totals = [], {}
        for k in picks:
            v = vs[k]
            if rigid_bone_of_part is not None:
                bone = rigid_bone_of_part[p["id"]]
            else:
                _, wts, ok = BW.get_vertex_bone_weights(dm, v)
                best = max(wts, key=lambda w: w.weight) if wts else None
                bone = BONE_NAMES[best.bone_index] if best else "pelvis"
            bone_totals[bone] = bone_totals.get(bone, 0) + 1
            off = bone_offset(bone, pts[k])
            samples.append({"bone": bone, "offset": vec(off)})
        p["samples"] = samples
        p["main_bone"] = max(bone_totals, key=bone_totals.get) if bone_totals else None
        if not p.get("region"):
            p["region"] = region_for(p["main_bone"] or "")
        p["verts"] = len(vs)
    return skm


if STEP == "muscles":
    names = json.load(open(os.path.join(REPO, "data", "muscle_names.json")))
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    acts = sorted([a for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.StaticMeshActor)
                   if str(a.get_folder_path()) == "Muscles"], key=lambda a: a.get_actor_label())
    dm = unreal.DynamicMesh()
    part_of_vertex, parts = [], []
    for i, a in enumerate(acts):
        pid = i + 1
        sm = a.static_mesh_component.static_mesh
        pdm = unreal.DynamicMesh()
        pdm, _ = AU.copy_mesh_from_static_mesh_v2(sm, pdm, unreal.GeometryScriptCopyMeshFromAssetOptions(),
                                                  unreal.GeometryScriptMeshReadLOD(), False)
        t = a.get_actor_transform()
        XF.transform_mesh(pdm, unreal.Transform(t.translation - MUSCLE_PIVOT, t.rotation.rotator(), t.scale3d))
        nv = Q.get_vertex_count(pdm)
        bb = Q.get_mesh_bounding_box(pdm)
        cx = (bb.min.x + bb.max.x) / 2
        E.append_mesh(dm, pdm, unreal.Transform())
        part_of_vertex += [pid] * nv
        label = a.get_actor_label()
        info = names[label]
        parts.append({"id": pid, "key": label, "name": info["name"], "kind": info["kind"],
                      "side": "Left" if cx > 0 else "Right", "region": None})
    assert len(part_of_vertex) == Q.get_vertex_count(dm), (len(part_of_vertex), Q.get_vertex_count(dm))
    log("muscles combined:", len(parts), "parts,", len(part_of_vertex), "verts")
    finish(dm, part_of_vertex, parts, "SKM_Anatomy_Muscles_Segmented", MAT_MUSCLE, None, samples_per_part=24)

elif STEP == "skeleton":
    info = json.load(open(os.path.join(REPO, "local", "skeleton_parts.json")))
    parts = info["parts"]
    src_dir = OUT_DIR + "/Source"
    src_path = src_dir + "/SM_Anatomy_Skeleton_Segmented_Source"
    if EAL.does_asset_exist(src_path):
        EAL.delete_asset(src_path)
    ui = unreal.FbxImportUI()
    ui.automated_import_should_detect_type = False
    ui.import_mesh = True
    ui.import_as_skeletal = False
    ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    ui.import_materials = False
    ui.import_textures = False
    ui.static_mesh_import_data.combine_meshes = True
    ui.static_mesh_import_data.auto_generate_collision = False
    ui.static_mesh_import_data.generate_lightmap_u_vs = False
    ui.static_mesh_import_data.build_nanite = False
    task = unreal.AssetImportTask()
    task.filename = os.path.join(REPO, "local", "skeleton_segmented.fbx")
    task.destination_path = src_dir
    task.destination_name = "SM_Anatomy_Skeleton_Segmented_Source"
    task.automated = True
    task.replace_existing = True
    task.save = True
    task.options = ui
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    sm = unreal.load_asset(src_path)
    slot_names = [str(m.material_slot_name) for m in sm.static_materials]
    log("imported skeleton source:", sm.get_num_vertices(0), "verts,", len(slot_names), "slots", slot_names[:4])
    dm = unreal.DynamicMesh()
    dm, _ = AU.copy_mesh_from_static_mesh_v2(sm, dm, unreal.GeometryScriptCopyMeshFromAssetOptions(),
                                             unreal.GeometryScriptMeshReadLOD(), False)
    bb = Q.get_mesh_bounding_box(dm)
    log("skeleton bounds", vec(bb.min), vec(bb.max))
    _, mids, _ = MAT.get_all_triangle_material_i_ds(dm)
    mids = L.convert_index_list_to_array(mids)
    _, tris, _ = Q.get_all_triangle_indices(dm, False)
    tris = L.convert_triangle_list_to_array(tris)
    nverts = Q.get_vertex_count(dm)
    part_of_vertex = [0] * nverts
    for tri, m in zip(tris, mids):
        pid = int(slot_names[m][1:4])  # "P017" -> 17
        part_of_vertex[tri.x] = pid
        part_of_vertex[tri.y] = pid
        part_of_vertex[tri.z] = pid
    missing = sum(1 for p in part_of_vertex if p == 0)
    log("vertex parts assigned; unassigned:", missing)
    finish(dm, part_of_vertex, parts, "SKM_Anatomy_Skeleton_Segmented", MAT_BONE,
           {p["id"]: p["bone"] for p in parts}, samples_per_part=12)

out_path = os.path.join(SAVED, f"segmented_{STEP}.json")
json.dump({"step": STEP, "parts": parts}, open(out_path, "w"), indent=1)
log("wrote", out_path)
