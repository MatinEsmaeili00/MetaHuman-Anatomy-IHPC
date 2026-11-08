# 16 - Turn an assembled MetaHuman Character (MHC -> Assemble, e.g. BP_MHC_Madi) into a switchable anatomy pawn.
#
#      The new pawn is a copy of BP_Beau_Pawn (CPP_AbbyCharacter with spring-arm camera, MuscleMesh, SkeletonMesh,
#      AnatomyView, AnatomyPicker, BonePicker, AnimationTimelineComp, CPP_MouseFunctionComp), with the MetaHuman parts
#      (Body, Face, grooms, outfit) taken from the assembled Blueprint, component by component (same names).
#      Body keeps ABP_Beau_Timeline: every MHC-assembled character uses the same skeleton (metahuman_base_skel), so
#      Beau's upright walk / jog / run and the retargeted action clips (/Game/MadisonAnimations/MetaHuman) play on it.
#      The muscle / skeleton layers follow the new body through Leader Pose (no re-fitting needed).
#
#      Then: adds the pawn to BP_PlayerController > Switchable Pawn Classes, and makes sure the level's
#      BP_PlayerPlaceHolderManagerA has a placeholder for it (spawn spot), adding one if needed.
#
#      Parameters (substituted, or edit here):  NAME (e.g. Madi), ASSEMBLED (assembled BP), PAWN (new pawn path)
import unreal

NAME = "__NAME__"
ASSEMBLED = "__ASSEMBLED__"
PAWN = "__PAWN__"
TEMPLATE = "/Game/MadisonTesting/BeauMHCharacter/BP_Beau_Pawn"
CONTROLLER = "/Game/Blueprints/BP_PlayerController"

EAL = unreal.EditorAssetLibrary
SDS = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
LIB = unreal.SubobjectDataBlueprintFunctionLibrary
METAHUMAN_PARTS = ["Body", "Face", "Hair", "Eyebrows", "Eyelashes", "Fuzz", "Beard", "Mustache", "SkeletalMesh"]


def templates(bp):
    out = {}
    for h in SDS.k2_gather_subobject_data_for_blueprint(bp):
        o = LIB.get_object(LIB.get_data(h))
        if o and not o.get_name().startswith("Default__"):
            out[o.get_name().replace("_GEN_VARIABLE", "")] = (h, o)
    return out


# 1) the pawn Blueprint
if not EAL.does_asset_exist(PAWN):
    EAL.duplicate_asset(TEMPLATE, PAWN)
    print("duplicated", TEMPLATE, "->", PAWN)
pawn_bp = unreal.load_asset(PAWN)
src = templates(unreal.load_asset(ASSEMBLED))
dst = templates(pawn_bp)

# 2) MetaHuman parts from the assembled Blueprint
for part in METAHUMAN_PARTS:
    s = src.get(part, (None, None))[1]
    d = dst.get(part, (None, None))
    if d[1] is None:
        if s is not None:
            print(f"  WARNING: {part} exists in {ASSEMBLED} but not in the pawn template - not copied")
        continue
    d_handle, d_obj = d
    if s is None:
        # e.g. no outfit: remove the template's outfit component instead of leaving Beau's clothes on
        root = SDS.k2_gather_subobject_data_for_blueprint(pawn_bp)[0]
        n = SDS.delete_subobject(root, d_handle, pawn_bp)
        print(f"  {part}: not in the assembled character -> removed from the pawn ({n})")
        continue
    if isinstance(d_obj, unreal.SkeletalMeshComponent):
        d_obj.set_editor_property("skeletal_mesh_asset", s.get_editor_property("skeletal_mesh_asset"))
        d_obj.set_editor_property("override_materials", list(s.get_editor_property("override_materials")))
        if part != "Body":   # Body keeps the timeline Anim BP of the template (ABP_Beau_Timeline)
            d_obj.set_editor_property("anim_class", s.get_editor_property("anim_class"))
        m = d_obj.get_editor_property("skeletal_mesh_asset")
        print(f"  {part}: {m.get_name() if m else None}")
    elif type(d_obj).__name__ == "GroomComponent":
        d_obj.set_editor_property("groom_asset", s.get_editor_property("groom_asset"))
        d_obj.set_editor_property("binding_asset", s.get_editor_property("binding_asset"))
        g = d_obj.get_editor_property("groom_asset")
        print(f"  {part}: {g.get_name() if g else None}")

# 3) anatomy layers: Skin -> Muscles -> Skeleton with equal fade lengths, so reveal depth 0.5 = muscles fully shown
dst = templates(pawn_bp)
view = dst["AnatomyView"][1]
layers = list(view.get_editor_property("layers"))
if layers:
    layers[0].set_editor_property("fade_length", 1.0)
    view.set_editor_property("layers", layers)

unreal.BlueprintEditorLibrary.compile_blueprint(pawn_bp)
print("saved pawn:", EAL.save_loaded_asset(pawn_bp, False))

# 4) controller: add to the pawn switcher list
pc_bp = unreal.load_asset(CONTROLLER)
cdo = unreal.get_default_object(pc_bp.generated_class())
classes = list(cdo.get_editor_property("switchable_pawn_classes"))
cls = pawn_bp.generated_class()
if cls not in classes:
    classes.append(cls)
    cdo.set_editor_property("switchable_pawn_classes", classes)
    unreal.BlueprintEditorLibrary.compile_blueprint(pc_bp)
    print("saved controller:", EAL.save_loaded_asset(pc_bp, False))
print("switchable:", [c.get_name() for c in cdo.get_editor_property("switchable_pawn_classes")])

# 5) level: one placeholder (spawn spot) per switchable pawn
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = EAS.get_all_level_actors()
manager = next((a for a in actors if a.get_class().get_name() == "BP_PlayerPlaceHolderManagerA_C"), None)
if manager:
    holders = list(manager.get_editor_property("player_place_holders"))
    index = classes.index(cls)
    if index >= len(holders) and holders:
        last = holders[-1].get_actor_location()
        prev = holders[-2].get_actor_location() if len(holders) > 1 else last + unreal.Vector(360, 340, 0)
        step = last - prev
        loc = last + step
        new = EAS.spawn_actor_from_class(holders[-1].get_class(), loc, holders[-1].get_actor_rotation())
        new.set_actor_label(f"BP_PlayerPlaceHolder_{NAME}")
        holders.append(new)
        manager.set_editor_property("player_place_holders", holders)
        unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
        print("added placeholder", new.get_actor_label(), loc)
    print("placeholders:", [h.get_actor_label() for h in holders], "-> pawn index", index, "spawns at", holders[index % len(holders)].get_actor_label())
