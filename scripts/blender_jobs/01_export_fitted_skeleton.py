"""Blender job 01 - Fit the segmented Skelly_Base skeleton onto Abby's MetaHuman reference pose and export it.

Run headless (nothing in Skelly_Base.blend is saved):
  Blender -b --factory-startup Skelly_Base.blend --python 01_export_fitted_skeleton.py -- \
      <abby_ref_pose.json> <out.fbx> <out_parts.json>

What it does
  1. Takes every bone mesh from the skeleton collections of Skelly_Base.blend (199 named pieces; the unused
     "Troracic Verts" sculpt set at the origin, the duplicate "Thyroid.001" and the hidden interosseous membrane are
     left out). Modifiers are evaluated (mirror -> both sides of the ribs) except Subdivision (real-time budget).
  2. Mirrored pieces that hold both sides (ribs) are split into left / right by their loose parts.
  3. Global fit G: Skelly space -> Abby component space (UE cm; +X = character's left, +Y = front, +Z = up):
     uniform scale + translation, least squares on hips, shoulders and the head joint.
  4. Per limb segment, a similarity transform puts the Skelly joints exactly on Abby's joints
     (femur -> thigh..calf, tibia/fibula -> calf..foot, foot -> foot..ball, humerus -> upperarm..lowerarm,
     radius/ulna -> lowerarm..hand, hand -> hand..middle_01, clavicle -> clavicle..upperarm). Torso, skull and
     scapulae use G.
  5. Every output piece gets its own material "P###" (the part ID survives the FBX round trip as the section /
     material index) and one row in the parts table: id, name, side, region, the Abby bone it follows rigidly.
  6. Writes one binary FBX. Coordinates are set so that a default Blender FBX export + default Unreal import lands
     the mesh in Abby's component space: Blender (x, y, z) m = UE (x, -y, z) cm / 100.
"""
import json
import math
import re
import sys

import bmesh
import bpy
from mathutils import Matrix, Quaternion, Vector

argv = sys.argv[sys.argv.index("--") + 1:]
REF_JSON, OUT_FBX, OUT_PARTS = argv[0], argv[1], argv[2]

MH = {r["name"]: Vector(r["pos"]) for r in json.load(open(REF_JSON))}

SKELETON_COLLECTIONS = [
    "Skull", "Cervical Vertebrae", "Thoracic Vertebrae", "Lumbar Vertebrae", "Spine", "Ribcage", "Limbs",
    "Wrist", "Metacarpals", "ProximalPhalanxes", "MiddlePhalanxes", "DistalPhalanxes",
    "Feet", "Cuneiforms", "Metatarsals", "ProximalPhalanxesOfTheToes", "MiddlePhalanxesOfTheToes",
    "DistalPhalanxesOfTheToes",
]
EXCLUDE = {"Thyroid.001", "Interosseous Membrane"}

arm = bpy.data.objects["Armature"]
AW = arm.matrix_world


def bone_head(name):
    return AW @ arm.data.bones[name].head_local


def bone_tail(name):
    return AW @ arm.data.bones[name].tail_local


# ---- 3. global fit G (Skelly -> UE cm): u = s * (x, -y, z) + t ---------------------------------------------------
def flip(p):
    return Vector((p.x, -p.y, p.z))


PAIRS = [(bone_head("Femur.l"), MH["thigh_l"]), (bone_head("Femur.r"), MH["thigh_r"]),
         (bone_head("Humerus.l"), MH["upperarm_l"]), (bone_head("Humerus.r"), MH["upperarm_r"]),
         (bone_head("Head"), MH["head"])]
X = [flip(a) for a, _ in PAIRS]
Y = [b for _, b in PAIRS]
mx = sum(X, Vector()) / len(X)
my = sum(Y, Vector()) / len(Y)
S = sum((x - mx).dot(y - my) for x, y in zip(X, Y)) / sum((x - mx).length_squared for x in X)
T = my - S * mx


def G(p):
    return S * flip(p) + T


print("global fit: scale %.4f  t %s  residuals %s" % (S, tuple(round(v, 2) for v in T),
      [round((G(a) - b).length, 2) for a, b in PAIRS]))


# ---- 4. segment transforms (in UE space, applied after G) ---------------------------------------------------------
def rotation_between(u, v):
    u, v = u.normalized(), v.normalized()
    return u.rotation_difference(v)


class Segment:
    """Puts joint a0 on b0 and points a0->a1 along b0->b1. The bone is stretched ALONG its length only
    (k_axial), so its thickness stays the same; stretch=False keeps the length too (rotation + translation)."""
    def __init__(self, a0, a1, b0, b1, stretch=True):
        self.a0, self.b0 = G(a0), b0
        u, v = G(a1) - self.a0, b1 - b0
        self.axis = u.normalized()
        self.q = rotation_between(u, v)
        self.k = v.length / u.length if stretch else 1.0
        self.err_before = ((G(a0) - b0).length, (G(a1) - b1).length)

    def __call__(self, p):  # p in Skelly space
        d = G(p) - self.a0
        along = self.axis * d.dot(self.axis)
        return self.b0 + self.q @ (along * self.k + (d - along))


def mid(a, b):
    return (a + b) / 2


SEG = {}
for s, S_ in (("l", "l"), ("r", "r")):
    knee = mid(bone_tail(f"Femur.{s}"), bone_head(f"Tibia.{s}"))
    ankle = bone_head(f"Tibia.{s}.001")
    ball = bone_head(f"Tibia.{s}.003")
    SEG[f"thigh_{s}"] = Segment(bone_head(f"Femur.{s}"), knee, MH[f"thigh_{s}"], MH[f"calf_{s}"])
    SEG[f"calf_{s}"] = Segment(knee, ankle, MH[f"calf_{s}"], MH[f"foot_{s}"])
    SEG[f"foot_{s}"] = Segment(ankle, ball, MH[f"foot_{s}"], MH[f"ball_{s}"])
    SEG[f"upperarm_{s}"] = Segment(bone_head(f"Humerus.{s}"), bone_tail(f"Humerus.{s}"), MH[f"upperarm_{s}"], MH[f"lowerarm_{s}"])
    SEG[f"lowerarm_{s}"] = Segment(bone_head(f"Radius.{s}"), bone_head(f"Wrist.{s}"), MH[f"lowerarm_{s}"], MH[f"hand_{s}"])
    # hand: orient only (Skelly's HandRotate tail isn't at the knuckles, so its length would wrongly shrink the hand)
    SEG[f"hand_{s}"] = Segment(bone_head(f"Wrist.{s}"), bone_tail(f"HandRotate.{s}"), MH[f"hand_{s}"], MH[f"middle_01_{s}"], stretch=False)
    # clavicle / scapula: the MetaHuman clavicle bone starts next to the spine, not at the sternum, so they keep G
for k, seg in SEG.items():
    print("segment %-12s scale %.3f  joint error before %.1f / %.1f cm" % (k, seg.k, *seg.err_before))


# ---- naming / grouping ---------------------------------------------------------------------------------------------
FINGER = {"Thumb": "thumb", "Index": "index", "Middle": "middle", "Ring": "ring", "Little": "pinky"}
TOE_ORD = {"First": "big toe", "Second": "second toe", "Third": "third toe", "Fourth": "fourth toe", "Fifth": "little toe"}
TOE_MID = {"A": "second toe", "B": "third toe", "C": "fourth toe", "D": "little toe"}
METACARPAL = {"First": "thumb_01", "Second": "index_metacarpal", "Third": "middle_metacarpal",
              "Fourth": "ring_metacarpal", "Fifth": "pinky_metacarpal"}


def words(name):
    return re.sub(r"(?<=[a-z])(?=[A-Z0-9])", " ", name).strip()


def classify(base, side):
    """-> (display name, region, segment key or None for G, rigid MetaHuman bone)."""
    s = side  # "l" / "r" / ""
    m = re.match(r"(Proximal|Middle|Distal)PhalanxOfThe(\w+?)(Finger)?$", base)
    if m:
        part, finger = m.group(1), m.group(2)
        mh = FINGER[finger]
        if mh == "thumb":  # MetaHuman thumb_01 = metacarpal, thumb_02 = proximal, thumb_03 = distal (no middle)
            idx = {"Proximal": 2, "Distal": 3}[part]
        else:
            idx = {"Proximal": 1, "Middle": 2, "Distal": 3}[part]
        label = "thumb" if mh == "thumb" else f"{finger.lower()} finger"
        return f"{part} phalanx of the {label}", "Hand", f"hand_{s}", f"{mh}_0{idx}_{s}"
    m = re.match(r"(First|Second|Third|Fourth|Fifth)Metacarpal$", base)
    if m:
        return f"{m.group(1)} metacarpal", "Hand", f"hand_{s}", f"{METACARPAL[m.group(1)]}_{s}"
    if base in ("Capitate", "Hamate", "Lunate", "Pisiform", "Scaphoid", "Trapezium", "Trapezoid", "Triquetrum"):
        return base, "Wrist (carpal bones)", f"hand_{s}", f"hand_{s}"
    m = re.match(r"(First|Second|Third|Fourth|Fifth)(Proximal|Distal)$", base)
    if m:
        return f"{m.group(2)} phalanx of the {TOE_ORD[m.group(1)]}", "Foot (toes)", f"foot_{s}", f"ball_{s}"
    m = re.match(r"([ABCD])Middle$", base)
    if m:
        return f"Middle phalanx of the {TOE_MID[m.group(1)]}", "Foot (toes)", f"foot_{s}", f"ball_{s}"
    m = re.match(r"(Inner|Outer)Sesamoid\.Toe$", base)
    if m:
        return f"{'Medial' if m.group(1) == 'Inner' else 'Lateral'} sesamoid of the big toe", "Foot (toes)", f"foot_{s}", f"ball_{s}"
    m = re.match(r"Metatarsal(First|Second|Third|Fourth|Fifth)$", base)
    if m:
        return f"{m.group(1)} metatarsal", "Foot", f"foot_{s}", f"foot_{s}"
    m = re.match(r"Cuneiform(Lateral|Medial|Middle)$", base)
    if m:
        return f"{'Intermediate' if m.group(1) == 'Middle' else m.group(1)} cuneiform", "Foot (tarsal bones)", f"foot_{s}", f"foot_{s}"
    if base in ("Calcaneus", "Cuboid", "Navicular", "Talus"):
        return base, "Foot (tarsal bones)", f"foot_{s}", f"foot_{s}"
    LIMB = {"Femur": ("Femur", "Thigh", "thigh", "thigh"), "Patella": ("Patella (kneecap)", "Knee", "calf", "calf"),
            "Tibia": ("Tibia", "Lower leg", "calf", "calf"), "Fibula": ("Fibula", "Lower leg", "calf", "calf"),
            "Humerus": ("Humerus", "Upper arm", "upperarm", "upperarm"),
            "Radius": ("Radius", "Forearm", "lowerarm", "lowerarm"), "Ulna": ("Ulna", "Forearm", "lowerarm", "lowerarm"),
            "Clavicle": ("Clavicle (collarbone)", "Shoulder girdle", None, "clavicle"),
            "Scapula": ("Scapula (shoulder blade)", "Shoulder girdle", None, "clavicle")}
    if base in LIMB:
        n, reg, seg, bone = LIMB[base]
        return n, reg, (f"{seg}_{s}" if seg else None), f"{bone}_{s}"
    VERT = {"Atlas": ("Atlas (C1)", "neck_02"), "Axis": ("Axis (C2)", "neck_02"), "C3": ("C3 vertebra", "neck_02"),
            "C4": ("C4 vertebra", "neck_01"), "C5": ("C5 vertebra", "neck_01"), "C6": ("C6 vertebra", "neck_01"),
            "C7": ("C7 vertebra", "neck_01")}
    if base in VERT:
        return VERT[base][0], "Cervical spine (neck)", None, VERT[base][1]
    m = re.match(r"T(\d+)$", base)
    if m:
        i = int(m.group(1))
        return f"T{i} vertebra", "Thoracic spine", None, "spine_05" if i <= 4 else ("spine_04" if i <= 8 else "spine_03")
    m = re.match(r"L(\d)$", base)
    if m:
        i = int(m.group(1))
        return f"L{i} vertebra", "Lumbar spine", None, "spine_02" if i <= 2 else "spine_01"
    if base in ("Sacrum", "Coccyx"):
        return base, "Pelvis", None, "pelvis"
    if base == "Pelvis":
        return "Hip bone (pelvis)", "Pelvis", None, "pelvis"
    m = re.match(r"(True|False|Floating)Rib(\d)$", base)
    if m:
        kind, i = m.group(1), int(m.group(2))
        num = i if kind == "True" else (7 + i if kind == "False" else 10 + i)
        bone = "spine_05" if num <= 3 else ("spine_04" if num <= 7 else "spine_03")
        return f"Rib {num} ({kind.lower()} rib)", "Rib cage", None, bone
    if base == "Sternum":
        return "Sternum (breastbone)", "Rib cage", None, "spine_04"
    if base == "CoastalCartilage":
        return "Costal cartilage", "Rib cage", None, "spine_04"
    if base == "Thyroid":
        return "Thyroid cartilage", "Neck", None, "neck_01"
    SKULL = {"EthmoidSinus": "Ethmoid bone", "Frontal": "Frontal bone", "Incus": "Incus (ear)", "Lacrimal": "Lacrimal bone",
             "Malleus": "Malleus (ear)", "Mandible": "Mandible (jaw)", "Maxilla": "Maxilla", "Nasal": "Nasal bone",
             "Occipital": "Occipital bone", "Parietal": "Parietal bone", "Sphenoid": "Sphenoid bone",
             "Stapes": "Stapes (ear)", "Temporal": "Temporal bone", "Vomer": "Vomer", "Zygomatic": "Zygomatic bone"}
    if base in SKULL:
        return SKULL[base], "Skull", None, "head"
    raise KeyError(base)


# ---- 1/2. collect, evaluate, split ---------------------------------------------------------------------------------
pieces = []
seen = set()
for cname in SKELETON_COLLECTIONS:
    for o in bpy.data.collections[cname].objects:
        if o.type == "MESH" and o.name not in EXCLUDE and o.name not in seen:
            seen.add(o.name)
            pieces.append(o)
for o in pieces:
    for m in o.modifiers:
        if m.type == "SUBSURF":
            m.show_viewport = False
bpy.context.view_layer.update()
dg = bpy.context.evaluated_depsgraph_get()

out_objects, parts = [], []


def emit(name, side, bm, mw):
    """bm: bmesh in object space of the source; mw: its world matrix. Transforms into UE space -> Blender export space."""
    base = name
    display, region, seg_key, bone = classify(base, side)
    seg = SEG.get(seg_key) if seg_key else None
    for v in bm.verts:
        p = mw @ v.co
        u = seg(p) if seg else G(p)
        v.co = Vector((u.x / 100.0, -u.y / 100.0, u.z / 100.0))
    me = bpy.data.meshes.new(f"P{len(parts) + 1:03d}")
    bm.to_mesh(me)
    bm.free()
    part_id = len(parts) + 1
    mat = bpy.data.materials.new(f"P{part_id:03d}")
    me.materials.clear()
    me.materials.append(mat)
    for poly in me.polygons:
        poly.material_index = 0
    ob = bpy.data.objects.new(f"P{part_id:03d}_{name}{'_' + side if side else ''}", me)
    bpy.context.scene.collection.objects.link(ob)
    out_objects.append(ob)
    side_txt = {"l": "Left", "r": "Right", "": "Centre"}[side]
    parts.append({"id": part_id, "key": f"{name}{'.' + side if side else ''}", "name": display, "side": side_txt,
                  "region": region, "bone": bone, "segment": seg_key or "torso", "verts": len(me.vertices)})


for o in pieces:
    m = re.match(r"(.+)\.(l|r)$", o.name)
    base, side = (m.group(1), m.group(2)) if m else (o.name, "")
    ev = o.evaluated_get(dg)
    me = ev.to_mesh()
    bm = bmesh.new()
    bm.from_mesh(me)
    ev.to_mesh_clear()
    mirrored = any(md.type == "MIRROR" for md in o.modifiers)
    if mirrored and not side and base not in ("Sternum", "CoastalCartilage", "Thyroid"):
        # split into loose parts, group by which side of the midline each part's centre is on
        bm.verts.ensure_lookup_table()
        islands, todo = [], set(bm.verts)
        while todo:
            seed = todo.pop()
            stack, isl = [seed], {seed}
            while stack:
                v = stack.pop()
                for e in v.link_edges:
                    w = e.other_vert(v)
                    if w not in isl:
                        isl.add(w); stack.append(w)
            todo -= isl
            islands.append(isl)
        groups = {"l": set(), "r": set(), "": set()}
        for isl in islands:
            cx = sum((o.matrix_world @ v.co).x for v in isl) / len(isl)
            groups["l" if cx > 1e-4 else ("r" if cx < -1e-4 else "")] |= isl
        for sd, keep in groups.items():
            if not keep:
                continue
            b2 = bm.copy()
            b2.verts.ensure_lookup_table()
            keep_idx = {v.index for v in keep}
            bmesh.ops.delete(b2, geom=[v for v in b2.verts if v.index not in keep_idx], context="VERTS")
            emit(base, sd, b2, o.matrix_world.copy())
        bm.free()
    else:
        emit(base, side, bm, o.matrix_world.copy())

print("parts:", len(parts), "verts:", sum(p["verts"] for p in parts))

# ---- export --------------------------------------------------------------------------------------------------------
bpy.ops.object.select_all(action="DESELECT")
for ob in out_objects:
    ob.select_set(True)
bpy.context.view_layer.objects.active = out_objects[0]
bpy.ops.object.join()
joined = bpy.context.view_layer.objects.active
joined.name = "SM_Anatomy_Skeleton_Segmented_Source"
bpy.ops.export_scene.fbx(filepath=OUT_FBX, use_selection=True, object_types={"MESH"}, use_mesh_modifiers=False,
                         mesh_smooth_type="FACE", add_leaf_bones=False, bake_anim=False, apply_unit_scale=True,
                         global_scale=1.0, axis_forward="-Z", axis_up="Y", path_mode="AUTO")
json.dump({"global_fit": {"scale": S, "t": list(T)}, "parts": parts}, open(OUT_PARTS, "w"), indent=1)
print("wrote", OUT_FBX, OUT_PARTS)
