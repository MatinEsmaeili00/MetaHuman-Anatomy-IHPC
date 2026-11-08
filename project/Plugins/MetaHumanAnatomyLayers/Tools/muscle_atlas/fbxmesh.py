"""Minimal ASCII-FBX reader for one skinned mesh: vertices, triangles, vertex colours, skin clusters, bone bind poses."""
import re


def _array(text, name, start=0):
    m = re.compile(r"%s: \*(\d+) \{\s*a: ([^}]*)\}" % name).search(text, start)
    return [v for v in m.group(2).replace("\n", "").replace("\t", "").split(",") if v], m.start(), m.end()


def load(path):
    text = open(path).read()
    verts, _, _ = _array(text, "Vertices")
    verts = [float(v) for v in verts]
    P = [tuple(verts[i * 3:i * 3 + 3]) for i in range(len(verts) // 3)]
    pvi, _, _ = _array(text, "PolygonVertexIndex")
    tris, cur = [], []
    for i in map(int, pvi):
        if i < 0:
            cur.append(-i - 1); tris.append(tuple(cur)); cur = []
        else:
            cur.append(i)
    models = {m.group(1): m.group(2) for m in re.finditer(r'Model: (\d+), "Model::([^"]+)", "(?:LimbNode|Null|Root)"', text)}
    conns = re.findall(r'C: "OO",(\d+),(\d+)', text)
    clusters = {}
    for m in re.finditer(r'Deformer: (\d+), "SubDeformer::[^"]*", "Cluster" \{', text):
        cid = m.group(1)
        end = text.index("\n\t}", m.end())
        body = text[m.end():end]
        idx = re.search(r"Indexes: \*\d+ \{\s*a: ([^}]*)\}", body)
        w = re.search(r"Weights: \*\d+ \{\s*a: ([^}]*)\}", body)
        tl = re.search(r"TransformLink: \*16 \{\s*a: ([^}]*)\}", body)
        clusters[cid] = dict(indexes=[int(x) for x in idx.group(1).replace("\n", "").split(",") if x] if idx else [],
                             weights=[float(x) for x in w.group(1).replace("\n", "").split(",") if x] if w else [],
                             link=[float(x) for x in tl.group(1).replace("\n", "").split(",") if x])
    bone_of = {}
    for child, parent in conns:
        if child in models and parent in clusters:
            bone_of[parent] = models[child]
    bones = {}
    for cid, c in clusters.items():
        name = bone_of.get(cid)
        if name:
            bones[name] = c
    return dict(text=text, P=P, tris=tris, bones=bones, models=models)


def bind_pos(bone):
    m = bone["link"]
    return (m[12], m[13], m[14])
