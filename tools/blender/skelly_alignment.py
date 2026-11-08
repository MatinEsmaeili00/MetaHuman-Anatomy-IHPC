import bpy, json, sys, re
from mathutils import Vector
def bbox(objs):
    lo=Vector((1e9,)*3); hi=Vector((-1e9,)*3); n=0
    for o in objs:
        if o.type!="MESH": continue
        mw=o.matrix_world
        for c in o.bound_box:
            p=mw@Vector(c); lo=Vector(map(min,lo,p)); hi=Vector(map(max,hi,p)); n+=1
    return {"min":[round(v,3) for v in lo],"max":[round(v,3) for v in hi],"size":[round(v,3) for v in hi-lo]} if n else None
colls={c.name:c for c in bpy.data.collections}
mus=[o for c in colls.values() if re.search(r"\.(R|L)$",c.name.strip()) for o in c.objects if o.type=="MESH"]
skel_names=["Cervical Vertebrae","Feet","Wrist","Limbs","Ribcage","Spine","Cuneiforms","DistalPhalanxes","Skull","DistalPhalanxesOfTheToes","Metacarpals","ProximalPhalanxesOfTheToes","Lower Arms","Lumbar Vertebrae","Metatarsals","MiddlePhalanxes","ProximalPhalanxes","Thoracic Vertebrae","Troracic Verts","Arms","MiddlePhalanxesOfTheToes"]
sk=[o for n in skel_names if n in colls for o in colls[n].all_objects if o.type=="MESH"]
abby=bpy.data.objects.get("AbbyAPoseModel_LOD0.001")
out={"abby":bbox([abby]) if abby else None,"muscles":bbox(mus),"skeleton":bbox(sk),
     "n_muscle_meshes":len(mus),"n_skel_meshes":len(sk),"skel_verts":sum(len(o.data.vertices) for o in sk),
     "abby_vgroups":len(abby.vertex_groups) if abby else None,
     "hidden_layers":{c.name:(c.hide_viewport, c.hide_render) for c in colls.values() if c.name in skel_names+["Collection","BlockoutBones"]},
     "skel_list":sorted(o.name for o in sk)}
arm=bpy.data.objects.get("Armature"); out["armature_bbox"]=None
if arm:
    pts=[arm.matrix_world@b.head_local for b in arm.data.bones]+[arm.matrix_world@b.tail_local for b in arm.data.bones]
    out["armature_bbox"]={"min":[round(min(p[i] for p in pts),3) for i in range(3)],"max":[round(max(p[i] for p in pts),3) for i in range(3)]}
    out["armature_bones"]=[b.name for b in arm.data.bones]
json.dump(out,open(sys.argv[sys.argv.index("--")+1],"w"),indent=1)
