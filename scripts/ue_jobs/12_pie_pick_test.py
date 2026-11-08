# 12 - PIE picking test (start PIE first). For the requested layer ("muscles" | "skeleton"), jump to that layer,
#      aim a ray from the player camera at one sample point of a few named parts, hover + select through the
#      pawn's pickers, and read back what the muscle-info widget shows.  LAYER / NAMES are substituted.
import unreal

LAYER = "__LAYER__"
NAMES = "__NAMES__".split("|")

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc = unreal.GameplayStatics.get_player_controller(world, 0)
pawn = pc.get_controlled_pawn()
view = pawn.get_component_by_class(unreal.AnatomyViewComp)
pickers = pawn.get_components_by_class(unreal.AnatomyPickerComp)
print("pawn", pawn.get_class().get_name(), "| pickers:", [p.get_name() for p in pickers])

depth = view.get_layer_depth(view.find_layer_index("Muscles" if LAYER == "muscles" else "Skeleton"))
view.set_reveal_depth(depth, False)
print("depth %.3f opacities %s active %s" % (view.get_reveal_depth(), [round(view.get_layer_opacity(i), 2) for i in range(3)],
      {p.get_name(): p.is_picking_active() for p in pickers}))

active = [p for p in pickers if p.is_picking_active()]
if not active:
    raise SystemExit("no active picker")
picker = active[0]
atlas = picker.get_editor_property("atlas")
mesh_name = str(picker.get_editor_property("mesh_component_name"))
mesh = [c for c in pawn.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name() == mesh_name][0]
cam = pc.player_camera_manager
origin = cam.get_camera_location()

info_widget = [w for w in unreal.ObjectIterator(unreal.UserWidget) if w.get_world() == world and w.get_class().get_name() == "WB_MuscleInfo_C"]
info_widget = info_widget[0] if info_widget else None

entries = atlas.get_editor_property("muscles")
for want in NAMES:
    cands = [e for e in entries if str(e.get_editor_property("name")) == want]
    if not cands:
        print("  no atlas entry", want); continue
    e = cands[0]
    s = e.get_editor_property("samples")[len(e.get_editor_property("samples")) // 2]
    bone_xf = mesh.get_socket_transform(s.get_editor_property("bone"), unreal.RelativeTransformSpace.RTS_WORLD)
    target = bone_xf.transform_location(s.get_editor_property("offset"))
    d = target - origin
    d = d / d.length()
    for p in pickers:
        p.update_hover(origin, d)
    hovered = picker.get_hovered_muscle()
    info = picker.get_muscle_info(hovered); ok = hovered > 0
    picker.select_hovered()
    tip = popup = None
    if info_widget:
        children = {w.get_name(): w for w in unreal.ObjectIterator(unreal.Widget)
                    if w.get_outer() and w.get_outer().get_outer() == info_widget}
        W = children.get
        tip = str(W("MuscleNameText").get_text()) if W("TooltipBox").is_visible() else None
        popup = str(W("PopupTitle").get_text()) if W("PopupBox").is_visible() else None
        if popup:
            popup += " / " + " / ".join(str(W(n).get_text())[:48] for n in ("PopupRegion", "PopupFunction") if W(n).is_visible())
    print("  aim %-34s -> hovered id %3d %-36s | tooltip=%s | popup=%s" % (
        f"{want} ({e.get_editor_property('side')})", hovered, (str(info.name) + " " + str(info.side)) if ok else "-", tip, popup))
