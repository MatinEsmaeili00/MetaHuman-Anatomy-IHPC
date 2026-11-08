"""Write the per-vertex muscle IDs into the FBX vertex colours: R = (id + 0.5) / 255, G = 0, B = 0, A = 1.

Measured in UE 5.8 (see docs/04-neck-beau-muscle-picking.md):
  * the VertexColor material node returns byte / 255 with no sRGB conversion, so the material decodes
    round(VertexColor.R * 255);
  * the FBX importer TRUNCATES float * 255 to the byte. Written as id / 255, float error gives e.g. 22.99999 -> 22,
    so about half the IDs came out one too low. Writing the middle of the byte (id + 0.5) / 255 survives truncation."""
import json, re, sys

src, labels, dst = sys.argv[1], sys.argv[2], sys.argv[3]
text = open(src).read()
ids = json.load(open(labels))["vertex_ids"]
assert max(ids) < 256
ci = text.index("LayerElementColor: 0")
m = re.compile(r"Colors: \*(\d+) \{\s*a: ([^}]*)\}").search(text, ci)
n = int(m.group(1))
assert n == len(ids) * 4, (n, len(ids))
vals = ",".join("%.9f,0,0,1" % ((i + 0.5) / 255.0) for i in ids)
new = text[:m.start()] + "Colors: *%d {\n\t\t\t\ta: %s\n\t\t\t} " % (n, vals) + text[m.end():]
open(dst, "w").write(new)
print("wrote", dst, "max id", max(ids))
