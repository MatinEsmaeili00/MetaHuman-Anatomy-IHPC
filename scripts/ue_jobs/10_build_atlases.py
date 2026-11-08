# 10 - Label atlases for the segmented layers (UMuscleAtlas data assets, read by UAnatomyPickerComp):
#      DA_Atlas_Muscles_Segmented  (186 muscles / ligaments / tendons, from Saved/ClaudeBridge/segmented_muscles.json)
#      DA_Atlas_Skeleton_Segmented (200 bones, from Saved/ClaudeBridge/segmented_skeleton.json)
#      Each entry: Id (= the vertex-colour part ID), Key, Name, Side, Region, Function / Running / Injury text, and
#      the picking samples (bone + offset in that bone's reference-pose space).
import json
import os
import re

import unreal

REPO = "/Users/mesmae6/GitHub/MetaHuman-Anatomy-IHPC"
SAVED = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "ClaudeBridge")
OUT_DIR = "/MetaHumanAnatomyLayers/Anatomy/Segmented"
EAL = unreal.EditorAssetLibrary

muscle_info = json.load(open(os.path.join(REPO, "data", "muscle_info.json")))
bone_info = json.load(open(os.path.join(REPO, "data", "bone_info.json")))


def make_entries(parts, info_for):
    entries = []
    for p in parts:
        region, function, running, injury = info_for(p)
        e = unreal.MuscleInfo()
        e.set_editor_property("id", p["id"])
        e.set_editor_property("key", p["key"])
        e.set_editor_property("name", p["name"])
        e.set_editor_property("side", p["side"])
        e.set_editor_property("region", region)
        e.set_editor_property("function", function)
        e.set_editor_property("running", running)
        e.set_editor_property("injury", injury)
        samples = []
        for s in p.get("samples", []):
            ms = unreal.MuscleSample()
            ms.set_editor_property("bone", s["bone"])
            ms.set_editor_property("offset", unreal.Vector(*s["offset"]))
            samples.append(ms)
        e.set_editor_property("samples", samples)
        entries.append(e)
    return entries


def muscle_text(p):
    region, f, r, i = muscle_info.get(p["name"], [p.get("region") or "", "", "", ""])
    return region, f, r, i


def bone_text(p):
    for pattern, f, r, i in bone_info:
        if re.search(pattern, p["name"], re.IGNORECASE):
            return p.get("region") or "", f, r, i
    return p.get("region") or "", "", "", ""


def write_atlas(name, parts, info_for):
    path = f"{OUT_DIR}/{name}"
    atlas = unreal.load_asset(path) if EAL.does_asset_exist(path) else \
        unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, OUT_DIR, unreal.MuscleAtlas, unreal.DataAssetFactory())
    atlas.set_editor_property("muscles", make_entries(parts, info_for))
    EAL.save_loaded_asset(atlas)
    missing = [p["name"] for p in parts if not info_for(p)[1]]
    print(f"{path}: {len(parts)} entries, {sum(len(p.get('samples', [])) for p in parts)} samples, "
          f"no info text: {sorted(set(missing))[:10]}")


m = json.load(open(os.path.join(SAVED, "segmented_muscles.json")))["parts"]
s = json.load(open(os.path.join(SAVED, "segmented_skeleton.json")))["parts"]
write_atlas("DA_Atlas_Muscles_Segmented", m, muscle_text)
write_atlas("DA_Atlas_Skeleton_Segmented", s, bone_text)
