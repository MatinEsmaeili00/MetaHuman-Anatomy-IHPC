"""Label every vertex of the anatomy muscle mesh with a major muscle (per side), from its dominant bone and its
position relative to the skeleton; smooth labels over connected mesh pieces; write a JSON atlas + preview images.
FBX space: Z up, +X = character's left, -Y = character's front, centimetres."""
import collections
import json
import math
import re
import struct
import sys
import zlib

sys.path.insert(0, sys.argv[2])
import fbxmesh  # noqa: E402

FBX, OUT = sys.argv[1], sys.argv[3]
d = fbxmesh.load(FBX)
P, tris, bones, text = d["P"], d["tris"], d["bones"], d["text"]
V = len(P)

# --- bone bind positions (clusters carry TransformLink; bones without weights come from the bind Pose) -----------
pos = {n: fbxmesh.bind_pos(b) for n, b in bones.items()}
models = {m.group(1): m.group(2) for m in re.finditer(r'Model: (\d+), "Model::([^"]+)", "LimbNode"', text)}
for m in re.finditer(r"PoseNode:\s*\{\s*Node: (\d+)\s*Matrix: \*16 \{\s*a: ([^}]*)\}", text):
    name = models.get(m.group(1))
    if name and name not in pos:
        v = [float(x) for x in m.group(2).replace("\n", "").split(",")]
        pos[name] = (v[12], v[13], v[14])

# --- dominant bone per vertex -----------------------------------------------------------------------------------
best_w = [0.0] * V
dom = [None] * V
for name, b in bones.items():
    for i, w in zip(b["indexes"], b["weights"]):
        if w > best_w[i]:
            best_w[i], dom[i] = w, name

MIDX = (pos["spine_03"][0] + pos["spine_04"][0] + pos["neck_01"][0]) / 3.0
SH = {s: pos["upperarm_" + s] for s in "lr"}
EL = {s: pos["lowerarm_" + s] for s in "lr"}


def spine_y(z):
    chain = [pos[b] for b in ("pelvis", "spine_01", "spine_02", "spine_03", "spine_04", "spine_05", "neck_01", "neck_02")]
    for a, b in zip(chain, chain[1:]):
        if a[2] <= z <= b[2]:
            t = (z - a[2]) / max(b[2] - a[2], 1e-6)
            return a[1] + t * (b[1] - a[1])
    return chain[0][1] if z < chain[0][2] else chain[-1][1]


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def norm(a):
    l = math.sqrt(dot(a, a)) or 1.0
    return (a[0] / l, a[1] / l, a[2] / l)


FRONT = (0.0, -1.0, 0.0)


def label(i):
    x, y, z = P[i]
    b = dom[i] or ""
    side = "l" if x > MIDX else "r"
    dx = abs(x - MIDX)
    fy = -(y - spine_y(z))                 # + = in front of the spine at that height
    if b.startswith("FACIAL") or b == "head":
        return "jaw", side
    if b.startswith("hand") or b.startswith(("thumb", "index", "middle", "ring", "pinky")):
        return "hand", side
    if b.startswith("lowerarm"):
        return "forearm", side
    in_arm = b.startswith("upperarm") or (b.startswith("clavicle") and dx > abs(SH[side][0] - MIDX) - 1.0 and z < SH[side][2] + 6)
    if in_arm:
        axis = norm(sub(EL[side], SH[side]))
        rel = sub(P[i], SH[side])
        t = dot(rel, axis) / math.sqrt(dot(sub(EL[side], SH[side]), sub(EL[side], SH[side])))
        if t < 0.33 or z > SH[side][2] - 2:
            return "deltoid", side
        radial = sub(rel, tuple(axis[k] * dot(rel, axis) for k in range(3)))
        return ("biceps" if dot(radial, FRONT) > 0 else "triceps"), side
    if b.startswith("thigh"):
        rel = sub(P[i], pos["thigh_" + side])
        lateral = rel[0] if side == "l" else -rel[0]
        if -rel[1] > 2.5:
            return "quadriceps", side
        if rel[1] > 2.5:
            return "hamstrings", side
        return ("adductors" if lateral < 0 else "tfl"), side
    if b.startswith("neck") and z > pos["clavicle_l"][2] + 2:
        return ("scm" if fy > 1.0 else "trapezius"), side
    if b == "pelvis" or (b.startswith("spine_01") and z < pos["spine_01"][2]):
        if fy < -1:                                  # back of the pelvis / lower back
            if z < pos["pelvis"][2] - 2:
                return "gluteus_maximus", side
            return ("erector_spinae" if dx < 8 else "gluteus_medius"), side
        if dx > 12:
            return "gluteus_medius", side
        return ("hip_flexors" if z < pos["pelvis"][2] + 2 else "rectus_abdominis" if dx < 6.5 else "external_oblique"), side
    # torso
    if fy > 4.0:                                   # front
        if z > pos["spine_04"][2] - 6 and dx > 2.5:
            return "pectoralis_major", side
        if dx > 13 and z > pos["spine_02"][2]:
            return "serratus_anterior", side
        return ("rectus_abdominis" if dx < 6.5 else "external_oblique"), side
    if fy < -1.0:                                  # back
        if z > pos["spine_04"][2] + 2 or (dx < 7 and z > pos["spine_03"][2] + 4):
            return "trapezius", side
        if dx > 6.5:
            return "latissimus_dorsi", side
        return "erector_spinae", side
    # flank between front and back
    if z > pos["spine_04"][2]:
        return ("pectoralis_major" if fy > 0 else "trapezius"), side
    return ("serratus_anterior" if z > pos["spine_02"][2] + 4 else "external_oblique"), side


raw = [label(i) for i in range(V)]

# --- smooth: each connected mesh piece takes its majority label -------------------------------------------------
parent = list(range(V))


def find(a):
    while parent[a] != a:
        parent[a] = parent[parent[a]]
        a = parent[a]
    return a


for t in tris:
    r = find(t[0])
    for v in t[1:]:
        rv = find(v)
        if rv != r:
            parent[rv] = r
pieces = collections.defaultdict(list)
for v in range(V):
    pieces[find(v)].append(v)
final = list(raw)
mixed = 0
for verts in pieces.values():
    if len(verts) < 4:
        continue
    maj, n = collections.Counter(raw[v] for v in verts).most_common(1)[0]
    if n >= 0.6 * len(verts):               # clearly one muscle: whole piece takes it
        for v in verts:
            final[v] = maj
    else:
        mixed += 1                          # piece spans several muscles: keep per-vertex labels
# light neighbour smoothing inside mixed pieces (removes speckle at region borders)
nbrs = collections.defaultdict(set)
for t in tris:
    for a in t:
        nbrs[a].update(t)
for _ in range(3):
    nxt = list(final)
    for v in range(V):
        if nbrs[v]:
            c = collections.Counter(final[u] for u in nbrs[v])
            lab, n = c.most_common(1)[0]
            if n > len(nbrs[v]) * 0.5:
                nxt[v] = lab
    final = nxt
print("mixed pieces kept per-vertex:", mixed)

keys = sorted(set(final))
ids = {k: n + 1 for n, k in enumerate(keys)}        # 0 = none
counts = collections.Counter(final)
print("labels:", len(keys))
for k in keys:
    print("  %3d %-26s %6d" % (ids[k], "%s_%s" % k, counts[k]))

# --- samples for picking: up to 160 vertices per muscle (FBX space) with their dominant bone --------------------
samples = collections.defaultdict(list)
by_label = collections.defaultdict(list)
for v in range(V):
    by_label[final[v]].append(v)
for k, vs in by_label.items():
    step = max(1, len(vs) // 160)
    for v in vs[::step][:160]:
        samples[k].append(dict(bone=dom[v], p=[round(c, 3) for c in P[v]]))

json.dump(dict(ids={"%s_%s" % k: ids[k] for k in keys},
               samples={"%s_%s" % k: samples[k] for k in keys},
               vertex_ids=[ids[final[v]] for v in range(V)],
               bones={n: list(p) for n, p in pos.items()}),
          open(OUT + "/muscle_labels.json", "w"))

# --- preview: front and back orthographic splats ---------------------------------------------------------------
W, H = 520, 440
def png(path, img):
    raw = b"".join(b"\x00" + bytes(row) for row in img)
    def chunk(t, p):
        return struct.pack(">I", len(p)) + t + p + struct.pack(">I", zlib.crc32(t + p) & 0xFFFFFFFF)
    open(path, "wb").write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", W, H, 8, 2, 0, 0, 0))
                           + chunk(b"IDAT", zlib.compress(raw, 6)) + chunk(b"IEND", b""))

palette = {}
for n, k in enumerate(sorted({k[0] for k in keys})):
    h = (n * 0.61803) % 1.0
    r, g, b = [int(255 * (0.35 + 0.65 * max(0, min(1, abs((h * 6 + o) % 6 - 3) - 1)))) for o in (0, 4, 2)]
    palette[k] = (r, g, b)
for view, sign in (("front", 1), ("back", -1)):
    img = [[30] * (W * 3) for _ in range(H)]
    depth = [[-1e9] * W for _ in range(H)]
    for v in range(V):
        x, y, z = P[v]
        u0 = int(W / 2 + sign * (x - MIDX) * 4.0)
        w0 = int(H - 10 - (z - 85) * 4.0)
        dpt = -y * sign
        c = palette[final[v][0]]
        shade = 0.72 if final[v][1] == "r" else 1.0
        for u in (u0, u0 + 1, u0 - 1):
            for w in (w0, w0 + 1, w0 - 1):
                if 0 <= u < W and 0 <= w < H and dpt > depth[w][u]:
                    depth[w][u] = dpt
                    img[w][u * 3:u * 3 + 3] = [int(c[0] * shade), int(c[1] * shade), int(c[2] * shade)]
    png(OUT + "/labels_%s.png" % view, img)
print("legend:", {k: palette[k] for k in palette})
