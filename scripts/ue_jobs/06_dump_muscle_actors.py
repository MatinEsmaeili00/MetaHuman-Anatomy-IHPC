# 06 - Dump every segmented muscle actor in Main (folder "Muscles"): label, static mesh, world bounds, vertex count.
#      Written to Saved/ClaudeBridge/muscle_actors.json for the alignment / build steps.
import json, os
import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
acts = [a for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.StaticMeshActor)
        if str(a.get_folder_path()) == "Muscles"]
rows = []
for a in acts:
    sm = a.static_mesh_component.static_mesh
    o, e = a.get_actor_bounds(False)
    t = a.get_actor_transform()
    rows.append({"label": a.get_actor_label(), "mesh": sm.get_path_name(),
                 "verts": sm.get_num_vertices(0), "tris": sm.get_num_triangles(0),
                 "center": [o.x, o.y, o.z], "extent": [e.x, e.y, e.z],
                 "loc": [t.translation.x, t.translation.y, t.translation.z],
                 "scale": [t.scale3d.x, t.scale3d.y, t.scale3d.z],
                 "materials": [m.get_path_name() if m else None for m in a.static_mesh_component.get_materials()]})
path = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "ClaudeBridge", "muscle_actors.json")
json.dump(rows, open(path, "w"), indent=1)
print(len(rows), "muscle actors ->", path)
print("total verts", sum(r["verts"] for r in rows), "tris", sum(r["tris"] for r in rows))
print("materials:", sorted({m for r in rows for m in r["materials"] if m})[:10])
