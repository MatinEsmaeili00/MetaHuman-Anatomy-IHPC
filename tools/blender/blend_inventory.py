import bpy, json, sys, collections
out = {"file": bpy.data.filepath, "scenes": [s.name for s in bpy.data.scenes],
       "unit_scale": bpy.context.scene.unit_settings.scale_length}
objs = []
for o in bpy.data.objects:
    d = {"name": o.name, "type": o.type, "parent": o.parent.name if o.parent else None,
         "collections": [c.name for c in o.users_collection],
         "dims": [round(v, 3) for v in o.dimensions], "loc": [round(v, 3) for v in o.matrix_world.translation],
         "hidden": o.hide_get() if o.name in bpy.context.view_layer.objects else None}
    if o.type == "MESH":
        m = o.data
        d.update(verts=len(m.vertices), faces=len(m.polygons),
                 mats=[s.material.name if s.material else None for s in o.material_slots][:12],
                 vgroups=len(o.vertex_groups), modifiers=[x.type for x in o.modifiers],
                 attrs=[a.name for a in m.attributes][:12])
        if o.vertex_groups: d["vgroup_sample"] = [g.name for g in o.vertex_groups][:15]
    if o.type == "ARMATURE":
        d.update(bones=len(o.data.bones), bone_sample=[b.name for b in o.data.bones][:40])
    objs.append(d)
out["objects"] = objs
out["counts"] = dict(collections.Counter(o.type for o in bpy.data.objects))
out["collections"] = {c.name: len(c.objects) for c in bpy.data.collections}
out["materials"] = len(bpy.data.materials); out["images"] = [i.name for i in bpy.data.images][:30]
out["actions"] = [a.name for a in bpy.data.actions][:30]
json.dump(out, open(sys.argv[sys.argv.index("--") + 1], "w"), indent=1)
