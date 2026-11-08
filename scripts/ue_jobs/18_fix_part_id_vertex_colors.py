# 18 - Fix the part IDs in the segmented anatomy meshes' vertex colours, so the hover / click glow lights the RIGHT part.
#
#      Bug: job 08 wrote ID k as the linear value (k + 0.5) / 255 into the dynamic mesh. Writing the skeletal mesh treats
#      Geometry Script vertex colours as sRGB and converts them to linear before the 8-bit render data is made, so the
#      GPU got round(255 * sRGBToLinear((k + 0.5) / 255)) instead of k: e.g. femur 74 -> byte 147's part (right
#      cuboid), pectoralis 117 -> left upper trapezius, and IDs 1..~40 were squeezed into bytes 0..6 (unreachable).
#      M_Anatomy_MuscleHighlight compares round(VertexColor.R * 255) with HoverId / SelectedId, so the glow landed on
#      the wrong part, or nowhere.
#
#      Fix (in place: same asset, same vertices / weights / materials, Blueprints keep their references): write
#      linearToSRGB((k + 0.25) / 255) so the stored byte is exactly k (the +0.25 is safe for round and floor).
#      Idempotent: meshes that already use the new encoding are skipped. Run with PIE stopped.
import unreal

AU = unreal.GeometryScript_AssetUtils
VC = unreal.GeometryScript_VertexColors
L = unreal.GeometryScript_List
MESHES = ["/MetaHumanAnatomyLayers/Anatomy/Segmented/SKM_Anatomy_Muscles_Segmented",
          "/MetaHumanAnatomyLayers/Anatomy/Segmented/SKM_Anatomy_Skeleton_Segmented"]


def lin_to_srgb(x):
    return 12.92 * x if x <= 0.0031308 else 1.055 * x ** (1 / 2.4) - 0.055


def srgb_to_lin(x):
    return x / 12.92 if x <= 0.04045 else ((x + 0.055) / 1.055) ** 2.4


def read(skm):
    ro = unreal.GeometryScriptCopyMeshFromAssetOptions()
    ro.apply_build_settings = False
    ro.request_tangents = True
    dm = unreal.DynamicMesh()
    dm, outcome = AU.copy_mesh_from_skeletal_mesh(skm, dm, ro, unreal.GeometryScriptMeshReadLOD())
    cl = [x for x in VC.get_mesh_per_vertex_colors(dm) if type(x).__name__ == "GeometryScriptColorList"][0]
    return dm, [c.r for c in L.convert_color_list_to_array(cl)]


for path in MESHES:
    skm = unreal.load_asset(path)
    dm, reds = read(skm)
    # Which encoding is in there? old: r*255 = k + 0.5 ; new: sRGBToLinear(r)*255 = k + 0.25
    old_like = sum(1 for r in reds if abs((r * 255.0) % 1.0 - 0.5) < 0.01 or r == 0.0)
    new_like = sum(1 for r in reds if abs((srgb_to_lin(r) * 255.0) % 1.0 - 0.25) < 0.01 or r == 0.0)
    if new_like == len(reds) and old_like != len(reds):
        print(path.split("/")[-1], "already fixed - skipped")
        continue
    if old_like != len(reds):
        raise RuntimeError(f"{path}: unexpected vertex colours ({old_like}/{len(reds)} match the job-08 encoding)")
    ids = [int(r * 255.0 + 1e-3) for r in reds]
    new = [unreal.LinearColor(lin_to_srgb((k + 0.25) / 255.0) if k else 0.0, 0.0, 0.0, 1.0) for k in ids]
    VC.set_mesh_per_vertex_colors(dm, L.convert_array_to_color_list(new))
    wo = unreal.GeometryScriptCopyMeshToAssetOptions()
    wo.use_original_vertex_order = True
    wo.enable_recompute_normals = False
    wo.enable_recompute_tangents = False
    _, outcome = AU.copy_mesh_to_skeletal_mesh(dm, skm, wo, unreal.GeometryScriptMeshWriteLOD())
    _, back = read(skm)
    ok = sum(1 for r, k in zip(back, ids) if int(srgb_to_lin(r) * 255.0 + 1e-3) == k)
    print(path.split("/")[-1], outcome, f"{len(set(ids)) - (0 in ids)} part ids, {ok}/{len(ids)} vertices verified,",
          "saved:", unreal.EditorAssetLibrary.save_loaded_asset(skm, False))
