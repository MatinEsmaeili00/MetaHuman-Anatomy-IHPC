"""Build anatomy layers (muscles / skeleton) that follow a MetaHuman body through Leader Pose.

Anatomy meshes such as the Chaos Flesh "Emil" example use anatomical limb bone names (humerus, ulna,
femur, tibia, ankle). Leader Pose matches bones *by name*, so this module:

1. copies the anatomy materials and textures into this plugin and re-points them at each other;
2. round-trips the anatomy meshes through FBX into this plugin with their **own new skeleton**,
   so nothing references /Game content (the plugin can be dropped into any project);
3. renames the limb bones to MetaHuman names. Twist bones are paired with MetaHuman twist bones by
   their measured position along the limb, not by guessing from names;
4. optionally wires the layers into a MetaHuman Blueprint (child of Body, Leader Pose = Body).

Run inside the Unreal Editor (Python):
    from anatomy_layers import build
    build.build_all()
    build.wire_blueprint("/Game/MetaHumans/MH_CH_Abby5/BP_MH_CH_Abby8")
"""
import os

import unreal

PLUGIN_MOUNT = "/MetaHumanAnatomyLayers"
DEST_DIR = PLUGIN_MOUNT + "/Anatomy"
MESH_DIR = DEST_DIR + "/Meshes"

SOURCE_EMIL = "/Game/ExampleContent/5_5_ChaosFlesh/Characters/Emil"
# Materials / textures: plain duplicates, re-pointed at each other afterwards.
MATERIAL_COPIES = {
    SOURCE_EMIL + "/Materials/M_Emil_MSS_Msl_Top1": DEST_DIR + "/Materials/M_Anatomy_Muscle",
    SOURCE_EMIL + "/Materials/M_blockColour1": DEST_DIR + "/Materials/M_Anatomy_BlockColour",
    SOURCE_EMIL + "/Materials/Mi_bone1": DEST_DIR + "/Materials/MI_Anatomy_Bone",
    SOURCE_EMIL + "/Textures/T_Emil_MSS_Muscle_Top_Mask1": DEST_DIR + "/Textures/T_Anatomy_Muscle_Mask",
    SOURCE_EMIL + "/Textures/T_cylinder_depth1": DEST_DIR + "/Textures/T_Anatomy_CylinderDepth",
}
# Meshes: (source mesh, new asset name). The first one creates the shared skeleton.
MESH_SOURCES = [
    (SOURCE_EMIL + "/Meshes/SKM_Emil_upperBodyMuscles", "SKM_Anatomy_Muscles_UpperBody"),
    (SOURCE_EMIL + "/Meshes/SKM_skeleton_upper_body", "SKM_Anatomy_Skeleton_UpperBody"),
]
ANATOMY_MESHES = [f"{MESH_DIR}/{name}" for _, name in MESH_SOURCES]
DEFAULT_METAHUMAN_BODY = "/Game/MetaHumans/MH_CH_Abby5/Body/SKM_MH_CH_Abby5_BodyMesh"

# Anatomy limb bone -> MetaHuman bone ({s} = l / r)
CORE_MAP = {
    "humerus_{s}": "upperarm_{s}",
    "ulna_{s}": "lowerarm_{s}",
    "femur_{s}": "thigh_{s}",
    "tibia_{s}": "calf_{s}",
    "ankle_{s}": "foot_{s}",
}
# Twist groups: (anatomy parent, anatomy child, anatomy twists, MH parent, MH child, MH twists)
TWIST_GROUPS = [
    ("humerus_{s}", "ulna_{s}", ["humerus_{s}_twist_01", "humerus_{s}_twist_02"],
     "upperarm_{s}", "lowerarm_{s}", ["upperarm_twist_01_{s}", "upperarm_twist_02_{s}"]),
    ("ulna_{s}", "hand_{s}", ["ulna_{s}_twist_00", "ulna_{s}_twist_01", "ulna_{s}_twist_02"],
     "lowerarm_{s}", "hand_{s}", ["lowerarm_twist_01_{s}", "lowerarm_twist_02_{s}"]),
    ("femur_{s}", "tibia_{s}", ["femur_twist_01_{s}", "femur_twist_02_{s}"],
     "thigh_{s}", "calf_{s}", ["thigh_twist_01_{s}", "thigh_twist_02_{s}"]),
]

EAL = unreal.EditorAssetLibrary


def _obj_path(package_path):
    return f"{package_path}.{package_path.rsplit('/', 1)[-1]}"


def _temp_dir():
    d = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()),
                     "MetaHumanAnatomyLayers")
    os.makedirs(d, exist_ok=True)
    return d


# ------------------------------------------------------------------------------------- materials
def copy_materials(copies=None, overwrite=False):
    """Duplicate materials/textures into the plugin and re-point references between the copies."""
    copies = copies or MATERIAL_COPIES
    for src, dst in copies.items():
        if EAL.does_asset_exist(dst):
            if not overwrite:
                continue
            EAL.delete_asset(dst)
        if not EAL.duplicate_asset(src, dst):
            raise RuntimeError(f"duplicate failed: {src} -> {dst}")
        print(f"copied {src} -> {dst}")
    remap = {_obj_path(s): unreal.load_asset(d) for s, d in copies.items()}

    def mapped(obj):
        return remap.get(obj.get_path_name()) if obj else None

    for dst in copies.values():
        asset = unreal.load_asset(dst)
        if isinstance(asset, unreal.MaterialInstanceConstant):
            parent = mapped(asset.get_editor_property("parent"))
            if parent:
                unreal.MaterialEditingLibrary.set_material_instance_parent(asset, parent)
            for tp in asset.get_editor_property("texture_parameter_values"):
                new = mapped(tp.parameter_value)
                if new:
                    unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(
                        asset, tp.parameter_info.name, new)
            unreal.MaterialEditingLibrary.update_material_instance(asset)
        elif isinstance(asset, unreal.Material):
            _repoint_material_textures(asset, mapped)
        EAL.save_loaded_asset(asset, only_if_is_dirty=False)
    return mapped


def _repoint_material_textures(material, mapped):
    mel = unreal.MaterialEditingLibrary
    changed = False
    for node in mel.get_material_expressions(material) or []:
        if isinstance(node, unreal.MaterialExpressionTextureBase):
            new = mapped(node.get_editor_property("texture"))
            if new:
                node.set_editor_property("texture", new)
                changed = True
    if changed:
        mel.recompile_material(material)
    print(f"material {material.get_name()}: textures {[t.get_name() for t in mel.get_used_textures(material)]}")


# ---------------------------------------------------------------------------------- mesh import
def export_fbx(mesh, filename, ascii=True):
    task = unreal.AssetExportTask()
    task.object = mesh
    task.filename = filename
    task.automated = True
    task.prompt = False
    task.replace_identical = True
    task.exporter = unreal.SkeletalMeshExporterFBX()
    opts = unreal.FbxExportOption()
    opts.set_editor_property("ascii", ascii)
    task.options = opts
    if not unreal.Exporter.run_asset_export_task(task):
        raise RuntimeError(f"FBX export failed for {mesh.get_path_name()}")
    return filename


def rename_bones_in_ascii_fbx(src_file, dst_file, bone_map):
    """Rename FBX nodes ("Model::x", "NodeAttribute::x", "SubDeformer::x", ...) in an ASCII FBX file.

    Renaming before import means the new Skeleton asset is created with MetaHuman names from the start.
    Editing names afterwards would need a skeleton merge, which Unreal only allows interactively.
    """
    import re
    with open(src_file, "r", encoding="utf-8", errors="surrogateescape") as f:
        text = f.read()
    if not text.lstrip().startswith(";"):
        raise RuntimeError(f"{src_file} is not an ASCII FBX")
    counts = {}

    def repl(m):
        old = m.group(2)
        new = bone_map.get(old)
        if new is None:
            return m.group(0)
        counts[old] = counts.get(old, 0) + 1
        return f"{m.group(1)}{new}{m.group(3)}"

    text = re.sub(r'(::)([A-Za-z0-9_]+)(")', repl, text)
    missing = [n for n in bone_map if n not in counts]
    with open(dst_file, "w", encoding="utf-8", errors="surrogateescape") as f:
        f.write(text)
    print(f"renamed {len(counts)} bones in {os.path.basename(dst_file)}" + (f" (not found: {missing})" if missing else ""))
    return counts


def import_fbx(filename, folder, name, skeleton=None):
    options = unreal.FbxImportUI()
    options.automated_import_should_detect_type = False
    options.import_mesh = True
    options.import_as_skeletal = True
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
    options.original_import_type = unreal.FBXImportType.FBXIT_SKELETAL_MESH
    options.import_materials = False
    options.import_textures = False
    options.import_animations = False
    options.create_physics_asset = False
    if skeleton is not None:
        options.skeleton = skeleton
    task = unreal.AssetImportTask()
    task.filename = filename
    task.destination_path = folder
    task.destination_name = name
    task.replace_existing = True
    task.automated = True
    task.save = True
    task.options = options
    task.factory = unreal.FbxFactory()
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    mesh = unreal.load_asset(f"{folder}/{name}")
    if not isinstance(mesh, unreal.SkeletalMesh):
        raise RuntimeError(f"import failed: {filename} -> {folder}/{name}")
    return mesh


def assign_materials(mesh, src, material_map):
    """Give the mesh the source mesh's slot names, using the plugin's material copies."""
    src_mats = list(src.get_editor_property("materials"))
    mats = list(mesh.get_editor_property("materials"))
    out = []
    for i, m in enumerate(mats):
        if i < len(src_mats):
            m = unreal.SkeletalMaterial(
                material_interface=material_map(src_mats[i].material_interface) or src_mats[i].material_interface,
                material_slot_name=src_mats[i].material_slot_name,
                uv_channel_data=m.uv_channel_data)
        out.append(m)
    mesh.set_editor_property("materials", out)
    EAL.save_loaded_asset(mesh, only_if_is_dirty=False)
    print(f"{mesh.get_name()} materials: {[(str(x.material_slot_name), x.material_interface.get_path_name()) for x in out]}")


def import_meshes(material_map, bone_map, overwrite=False):
    skeleton = None
    meshes = []
    for src_path, name in MESH_SOURCES:
        dst = f"{MESH_DIR}/{name}"
        if EAL.does_asset_exist(dst) and not overwrite:
            mesh = unreal.load_asset(dst)
        else:
            src = unreal.load_asset(src_path)
            raw = export_fbx(src, os.path.join(_temp_dir(), name + "_source.fbx"))
            fbx = os.path.join(_temp_dir(), name + ".fbx")
            rename_bones_in_ascii_fbx(raw, fbx, bone_map)
            mesh = import_fbx(fbx, MESH_DIR, name, skeleton)
            EAL.save_loaded_asset(mesh.skeleton, only_if_is_dirty=False)
            EAL.save_loaded_asset(mesh, only_if_is_dirty=False)
            print(f"imported {name} (skeleton {mesh.skeleton.get_path_name()})")
        assign_materials(mesh, unreal.load_asset(src_path), material_map)
        if skeleton is None:
            skeleton = mesh.skeleton
        meshes.append(mesh)
    return meshes


# ------------------------------------------------------------------------------------- bone map
def _global_positions(mesh):
    mod = unreal.SkeletonModifier()
    mod.set_skeletal_mesh(mesh)
    return {str(n): mod.get_bone_transform(n, True).translation for n in mod.get_all_bone_names()}


def _fraction(p, a, b):
    ab = b - a
    denom = ab.dot(ab)
    return (p - a).dot(ab) / denom if denom > 1e-6 else 0.0


def compute_bone_map(anatomy_mesh, metahuman_mesh, verbose=True):
    """Return {anatomy_bone: metahuman_bone} for every anatomy bone that needs renaming."""
    a_pos = _global_positions(anatomy_mesh)
    m_pos = _global_positions(metahuman_mesh)
    result = {}
    for side in ("l", "r"):
        for a, m in CORE_MAP.items():
            a, m = a.format(s=side), m.format(s=side)
            if a in a_pos and m in m_pos and a != m:
                result[a] = m
        for a_par, a_child, a_tw, m_par, m_child, m_tw in TWIST_GROUPS:
            a_par, a_child, m_par, m_child = (x.format(s=side) for x in (a_par, a_child, m_par, m_child))
            a_tw = [x.format(s=side) for x in a_tw if x.format(s=side) in a_pos]
            m_tw = [x.format(s=side) for x in m_tw if x.format(s=side) in m_pos]
            if not (a_tw and m_tw and all(b in a_pos for b in (a_par, a_child))
                    and all(b in m_pos for b in (m_par, m_child))):
                continue
            ta = {x: _fraction(a_pos[x], a_pos[a_par], a_pos[a_child]) for x in a_tw}
            tm = {x: _fraction(m_pos[x], m_pos[m_par], m_pos[m_child]) for x in m_tw}
            used_a, used_m = set(), set()
            for _, x, y in sorted((abs(ta[x] - tm[y]), x, y) for x in ta for y in tm):
                if x in used_a or y in used_m:
                    continue
                used_a.add(x)
                used_m.add(y)
                result[x] = y
                if verbose:
                    print(f"  twist {x} (t={ta[x]:.2f}) -> {y} (t={tm[y]:.2f})")
    clash = set(result.values()) & (set(a_pos) - set(result))
    if clash:
        raise RuntimeError(f"rename would clash with existing anatomy bones: {sorted(clash)}")
    return result


def rename_bones(mesh, bone_map):
    mod = unreal.SkeletonModifier()
    mod.set_skeletal_mesh(mesh)
    existing = {str(n) for n in mod.get_all_bone_names()}
    todo = {a: m for a, m in bone_map.items() if a in existing}
    if not todo:
        print(f"{mesh.get_name()}: already renamed")
        return 0
    if not mod.rename_bones(list(todo.keys()), list(todo.values())):
        raise RuntimeError(f"rename_bones failed on {mesh.get_name()}")
    if not mod.commit_skeleton_to_skeletal_mesh():
        raise RuntimeError(f"commit failed on {mesh.get_name()}")
    EAL.save_loaded_asset(mesh, only_if_is_dirty=False)
    print(f"{mesh.get_name()}: renamed {len(todo)} bones")
    return len(todo)


# ---------------------------------------------------------------------------------------- build
HIDDEN_MATERIAL = DEST_DIR + "/Materials/M_Anatomy_Hidden"
DITHER_FUNCTION = "/Engine/Functions/Engine_MaterialFunctions02/Utility/DitherTemporalAA"


def create_hidden_material(path=HIDDEN_MATERIAL):
    """Always-invisible material, used by UAnatomyViewComp for slots that have no fade parameter
    (e.g. the MetaHuman eye occlusion shell) while their layer fades."""
    mel = unreal.MaterialEditingLibrary
    if EAL.does_asset_exist(path):
        mat = unreal.load_asset(path)
    else:
        folder, name = path.rsplit("/", 1)
        mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
        mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
        zero = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 200)
        zero.set_editor_property("r", 0.0)
        mel.connect_material_property(zero, "", unreal.MaterialProperty.MP_OPACITY_MASK)
        mat.set_editor_property("two_sided", True)
    mat.set_editor_property("used_with_skeletal_mesh", True)
    mat.set_editor_property("used_with_hair_strands", True)  # also lands on groom slots (facial hair)
    mel.recompile_material(mat)
    EAL.save_loaded_asset(mat, only_if_is_dirty=False)
    return mat

XRAY_MATERIAL = DEST_DIR + "/Materials/M_Anatomy_XRay"


def create_xray_material(path=XRAY_MATERIAL, overwrite=False):
    """Translucent, unlit fresnel "x-ray" shell. UAnatomyViewComp swaps the skin to it once the skin has
    faded below the layer's GhostBelowOpacity, so the body outline (e.g. the legs) stays readable."""
    mel = unreal.MaterialEditingLibrary
    if EAL.does_asset_exist(path):
        if not overwrite:
            return unreal.load_asset(path)
        EAL.delete_asset(path)
    folder, name = path.rsplit("/", 1)
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("used_with_skeletal_mesh", True)

    def expr(cls, x, y, **props):
        e = mel.create_material_expression(mat, cls, x, y)
        for k, v in props.items():
            e.set_editor_property(k, v)
        return e

    fresnel = expr(unreal.MaterialExpressionFresnel, -900, 0, exponent=2.5)
    tint = expr(unreal.MaterialExpressionVectorParameter, -900, -250, parameter_name="Tint",
                default_value=unreal.LinearColor(0.45, 0.72, 1.0, 1.0), group="XRay")
    rim_bright = expr(unreal.MaterialExpressionScalarParameter, -900, 200, parameter_name="RimBrightness", default_value=2.0, group="XRay")
    core_bright = expr(unreal.MaterialExpressionScalarParameter, -900, 300, parameter_name="CoreBrightness", default_value=0.08, group="XRay")
    rim_opacity = expr(unreal.MaterialExpressionScalarParameter, -900, 450, parameter_name="RimOpacity", default_value=0.6, group="XRay")
    core_opacity = expr(unreal.MaterialExpressionScalarParameter, -900, 550, parameter_name="CoreOpacity", default_value=0.04, group="XRay")

    rim = expr(unreal.MaterialExpressionMultiply, -600, 150)
    mel.connect_material_expressions(fresnel, "", rim, "A")
    mel.connect_material_expressions(rim_bright, "", rim, "B")
    glow = expr(unreal.MaterialExpressionAdd, -450, 150)
    mel.connect_material_expressions(rim, "", glow, "A")
    mel.connect_material_expressions(core_bright, "", glow, "B")
    emissive = expr(unreal.MaterialExpressionMultiply, -300, 0)
    mel.connect_material_expressions(tint, "", emissive, "A")
    mel.connect_material_expressions(glow, "", emissive, "B")
    mel.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    rim_a = expr(unreal.MaterialExpressionMultiply, -600, 450)
    mel.connect_material_expressions(fresnel, "", rim_a, "A")
    mel.connect_material_expressions(rim_opacity, "", rim_a, "B")
    alpha = expr(unreal.MaterialExpressionAdd, -450, 450)
    mel.connect_material_expressions(rim_a, "", alpha, "A")
    mel.connect_material_expressions(core_opacity, "", alpha, "B")
    mel.connect_material_property(alpha, "", unreal.MaterialProperty.MP_OPACITY)

    mel.recompile_material(mat)
    EAL.save_loaded_asset(mat, only_if_is_dirty=False)
    return mat


MUSCLE_FADE_MATERIAL = DEST_DIR + "/Materials/M_Anatomy_MuscleFade"


def create_muscle_fade_material(path=MUSCLE_FADE_MATERIAL, overwrite=False):
    """Fresh masked muscle material (muscle red + tendon mask) with a dithered "Fade" (1 = visible, 0 = gone).
    UAnatomyViewComp uses it for the Muscles layer only while that layer is fading; at full opacity the
    detailed original muscle material is shown. (Masking added later to the Emil-derived material never
    clipped, so the fade lives in a material built from scratch.)"""
    mel = unreal.MaterialEditingLibrary
    if EAL.does_asset_exist(path):
        if not overwrite:
            return unreal.load_asset(path)
        EAL.delete_asset(path)
    folder, name = path.rsplit("/", 1)
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    mat.set_editor_property("opacity_mask_clip_value", 0.5)  # DitherTemporalAA: coverage == Fade at clip 0.5
    mat.set_editor_property("used_with_skeletal_mesh", True)

    def expr(cls, x, y, **props):
        e = mel.create_material_expression(mat, cls, x, y)
        for k, v in props.items():
            e.set_editor_property(k, v)
        return e

    mask = expr(unreal.MaterialExpressionTextureSampleParameter2D, -900, 0, parameter_name="Mask",
                texture=unreal.load_asset(DEST_DIR + "/Textures/T_Anatomy_Muscle_Mask"))
    muscle = expr(unreal.MaterialExpressionVectorParameter, -900, -300, parameter_name="muscle_COLOR",
                  default_value=unreal.LinearColor(0.20, 0.025, 0.02, 1.0))
    tendon = expr(unreal.MaterialExpressionVectorParameter, -900, -150, parameter_name="Tendons_COLOR",
                  default_value=unreal.LinearColor(0.55, 0.50, 0.45, 1.0))
    base = expr(unreal.MaterialExpressionLinearInterpolate, -500, -200)
    mel.connect_material_expressions(muscle, "", base, "A")
    mel.connect_material_expressions(tendon, "", base, "B")
    mel.connect_material_expressions(mask, "R", base, "Alpha")
    mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)

    r_muscle = expr(unreal.MaterialExpressionScalarParameter, -900, 250, parameter_name="muscle_ROUGHNESS", default_value=0.45)
    r_tendon = expr(unreal.MaterialExpressionScalarParameter, -900, 350, parameter_name="Tendons_ROUGHNESS", default_value=0.3)
    rough = expr(unreal.MaterialExpressionLinearInterpolate, -500, 300)
    mel.connect_material_expressions(r_muscle, "", rough, "A")
    mel.connect_material_expressions(r_tendon, "", rough, "B")
    mel.connect_material_expressions(mask, "R", rough, "Alpha")
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)

    fade = expr(unreal.MaterialExpressionScalarParameter, -700, 600, parameter_name="Fade", default_value=1.0, group="Anatomy")
    dither = expr(unreal.MaterialExpressionMaterialFunctionCall, -450, 600)
    dither.set_material_function(unreal.load_asset(DITHER_FUNCTION))
    mel.connect_material_expressions(fade, "", dither, "Alpha Threshold")
    mel.connect_material_property(dither, "", unreal.MaterialProperty.MP_OPACITY_MASK)

    mel.recompile_material(mat)
    EAL.save_loaded_asset(mat, only_if_is_dirty=False)
    return mat


HIGHLIGHT_MATERIAL = DEST_DIR + "/Materials/M_Anatomy_MuscleHighlight"


def create_highlight_material(path=HIGHLIGHT_MATERIAL, overwrite=False):
    """Highlight material for the muscle mesh: the vertex colour's red channel holds the muscle ID (byte = id,
    read in the material as round(R * 255), no sRGB conversion). The muscle whose ID equals HoverId glows in
    HoverColor, SelectedId in SelectedColor; everything else is invisible. UAnatomyPickerComp draws it with a
    highlight copy of the muscle mesh. Depth test is off: the copy sits exactly on the opaque muscles, so with depth
    testing it would z-fight and mostly disappear."""
    mel = unreal.MaterialEditingLibrary
    if EAL.does_asset_exist(path):
        if not overwrite:
            return unreal.load_asset(path)
        EAL.delete_asset(path)
    folder, name = path.rsplit("/", 1)
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("used_with_skeletal_mesh", True)
    mat.set_editor_property("disable_depth_test", True)

    def expr(cls, x, y, **props):
        e = mel.create_material_expression(mat, cls, x, y)
        for k, v in props.items():
            e.set_editor_property(k, v)
        return e

    def op(cls, a, b, x, y, an="", bn=""):
        e = expr(cls, x, y)
        mel.connect_material_expressions(a, an, e, "A")
        if b is not None:
            mel.connect_material_expressions(b, bn, e, "B")
        return e

    vc = expr(unreal.MaterialExpressionVertexColor, -1400, 0)
    k255 = expr(unreal.MaterialExpressionConstant, -1400, 120, r=255.0)
    raw = op(unreal.MaterialExpressionMultiply, vc, k255, -1250, 0, an="R")
    mid = expr(unreal.MaterialExpressionRound, -1100, 0)
    mel.connect_material_expressions(raw, "", mid, "")

    def mask(param, default, y):
        p = expr(unreal.MaterialExpressionScalarParameter, -1100, y, parameter_name=param, default_value=default, group="Highlight")
        diff = op(unreal.MaterialExpressionSubtract, mid, p, -950, y)
        a = expr(unreal.MaterialExpressionAbs, -820, y)
        mel.connect_material_expressions(diff, "", a, "")
        s = expr(unreal.MaterialExpressionSaturate, -700, y)
        mel.connect_material_expressions(a, "", s, "")
        m = expr(unreal.MaterialExpressionOneMinus, -580, y)
        mel.connect_material_expressions(s, "", m, "")
        return m

    hover = mask("HoverId", -1.0, 200)
    sel = mask("SelectedId", -1.0, 400)
    hc = expr(unreal.MaterialExpressionVectorParameter, -580, 600, parameter_name="HoverColor", default_value=unreal.LinearColor(0.2, 0.85, 1.0, 1.0), group="Highlight")
    sc = expr(unreal.MaterialExpressionVectorParameter, -580, 750, parameter_name="SelectedColor", default_value=unreal.LinearColor(1.0, 0.55, 0.1, 1.0), group="Highlight")
    eh = op(unreal.MaterialExpressionMultiply, hc, hover, -420, 600)
    es = op(unreal.MaterialExpressionMultiply, sc, sel, -420, 750)
    emis = op(unreal.MaterialExpressionAdd, eh, es, -280, 650)
    fres = expr(unreal.MaterialExpressionFresnel, -420, 900, exponent=2.0)
    boost = expr(unreal.MaterialExpressionConstant, -420, 1000, r=0.6)
    rim = op(unreal.MaterialExpressionAdd, fres, boost, -280, 950)
    glow = op(unreal.MaterialExpressionMultiply, emis, rim, -150, 700)
    mel.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    ha = expr(unreal.MaterialExpressionScalarParameter, -580, 1100, parameter_name="HoverOpacity", default_value=0.55, group="Highlight")
    sa = expr(unreal.MaterialExpressionScalarParameter, -580, 1200, parameter_name="SelectedOpacity", default_value=0.5, group="Highlight")
    oh = op(unreal.MaterialExpressionMultiply, hover, ha, -420, 1100)
    os_ = op(unreal.MaterialExpressionMultiply, sel, sa, -420, 1200)
    oa = op(unreal.MaterialExpressionMax, oh, os_, -280, 1150)
    mel.connect_material_property(oa, "", unreal.MaterialProperty.MP_OPACITY)
    add_highlight_inflate(mat)
    mel.recompile_material(mat)
    EAL.save_loaded_asset(mat, only_if_is_dirty=False)
    return mat


def add_highlight_inflate(mat):
    """Push the highlight shell out along the normals (Inflate cm) so it sits just in front of the opaque muscle
    surface it is drawn over; at exactly the same depth the opaque muscles win and the glow disappears."""
    mel = unreal.MaterialEditingLibrary
    if "Inflate" in [str(n) for n in mel.get_scalar_parameter_names(mat)]:
        return
    nrm = mel.create_material_expression(mat, unreal.MaterialExpressionVertexNormalWS, -420, 1400)
    amt = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -420, 1500)
    amt.set_editor_property("parameter_name", "Inflate")
    amt.set_editor_property("default_value", 0.2)
    amt.set_editor_property("group", "Highlight")
    off = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -250, 1450)
    mel.connect_material_expressions(nrm, "", off, "A")
    mel.connect_material_expressions(amt, "", off, "B")
    mel.connect_material_property(off, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)


def build_all(metahuman_body=DEFAULT_METAHUMAN_BODY, overwrite=False):
    material_map = copy_materials(overwrite=overwrite)
    create_hidden_material()
    create_muscle_fade_material()
    create_xray_material()
    bone_map = compute_bone_map(unreal.load_asset(MESH_SOURCES[0][0]), unreal.load_asset(metahuman_body))
    meshes = import_meshes(material_map, bone_map, overwrite=overwrite)
    for mesh in meshes:
        names = {str(n) for n in unreal.SkeletonModifier.get_all_bone_names(_modifier_for(mesh))}
        leftover = sorted(set(bone_map) & names)
        if leftover:
            raise RuntimeError(f"{mesh.get_name()} still has anatomical names: {leftover}")
    return bone_map


def _modifier_for(mesh):
    mod = unreal.SkeletonModifier()
    mod.set_skeletal_mesh(mesh)
    return mod


# ------------------------------------------------------------------------------------ blueprint
def wire_blueprint(bp_path, layers=None, body_name="Body", add_view_component=True):
    """Point anatomy components at the plugin meshes, parent them to Body and let Leader Pose drive them.

    layers: {component variable name: mesh path}. Missing components are created under Body.
    The stock MetaHuman construction script (EnableMasterPose on Body's children) sets Leader Pose.
    add_view_component: also add an "AnatomyView" component (UAnatomyViewComp: skin -> muscles -> skeleton slider).
    """
    layers = layers or {"MuscleMesh": ANATOMY_MESHES[0], "SkeletonMesh": ANATOMY_MESHES[1]}
    bp = unreal.load_asset(bp_path)
    sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    lib = unreal.SubobjectDataBlueprintFunctionLibrary

    def handles():
        out = {}
        for h in sds.k2_gather_subobject_data_for_blueprint(bp):
            data = sds.k2_find_subobject_data_from_handle(h)
            out[str(lib.get_variable_name(data))] = (h, lib.get_object(data))
        return out

    hs = handles()
    if body_name not in hs:
        raise RuntimeError(f"{bp_path} has no component named {body_name}")
    body_h = hs[body_name][0]
    for comp_name, mesh_path in layers.items():
        if comp_name not in hs:
            params = unreal.AddNewSubobjectParams(parent_handle=body_h, new_class=unreal.SkeletalMeshComponent,
                                                  blueprint_context=bp)
            new_h, fail = sds.add_new_subobject(params)
            if not fail.is_empty():
                raise RuntimeError(f"add component failed: {fail}")
            sds.rename_subobject(new_h, unreal.Text(comp_name))
            hs = handles()
        sds.attach_subobject(body_h, hs[comp_name][0])
        comp = handles()[comp_name][1]
        comp.set_editor_property("skeletal_mesh_asset", unreal.load_asset(mesh_path))
        comp.set_editor_property("anim_class", None)
        comp.set_editor_property("use_bounds_from_leader_pose_component", True)
        comp.set_editor_property("relative_location", unreal.Vector(0, 0, 0))
        comp.set_editor_property("relative_rotation", unreal.Rotator(0, 0, 0))
        comp.set_editor_property("relative_scale3d", unreal.Vector(1, 1, 1))
        print(f"{comp_name}: {mesh_path} under {body_name}")

    if add_view_component:
        # Anatomy View (skin -> muscles -> skeleton slider) and Anatomy Picker (hover / click muscles)
        for cls_name, comp_name in (("AnatomyViewComp", "AnatomyView"), ("AnatomyPickerComp", "AnatomyPicker")):
            cls = getattr(unreal, cls_name, None)
            if cls is None:
                continue
            hs = handles()
            if any(isinstance(obj, cls) for _, obj in hs.values()):
                continue
            actor_h = sds.k2_gather_subobject_data_for_blueprint(bp)[0]
            params = unreal.AddNewSubobjectParams(parent_handle=actor_h, new_class=cls, blueprint_context=bp)
            new_h, fail = sds.add_new_subobject(params)
            if not fail.is_empty():
                raise RuntimeError(f"add {comp_name} failed: {fail}")
            sds.rename_subobject(new_h, unreal.Text(comp_name))
            print(f"{comp_name} component added")
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    EAL.save_loaded_asset(bp, only_if_is_dirty=False)
    return True
