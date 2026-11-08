# 03 - Probe / drive a running PIE session of COPY2 (start PIE first, e.g. with the MCP StartPIE tool).
# ACTION is substituted before running (tools/ue/pie.sh): status | layer:body|muscles|skeleton | play | pause |
# time:<0..1> | speed:<0..1> | switch:<+1|-1> | depth:<0..1> | cam:<yaw>,<pitch>,<length>
import unreal

ACTION = "__ACTION__"

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
if world is None:
    raise SystemExit("No PIE world: start Play-In-Editor first")
pc = unreal.GameplayStatics.get_player_controller(world, 0)
pawn = pc.get_controlled_pawn() if pc else None
print("world:", world.get_name(), "| pc:", pc.get_class().get_name() if pc else None,
      "| pawn:", pawn.get_class().get_name() if pawn else None, pawn.get_actor_location() if pawn else "")

def all_widgets():
    # WidgetBlueprintLibrary isn't exposed to Python here: walk the live UserWidget objects of this PIE world.
    return [w for w in unreal.ObjectIterator(unreal.UserWidget)
            if w.get_world() == world and not w.get_name().startswith("Default__")]

def widgets_of(class_name):
    return [w for w in all_widgets() if w.get_class().get_name() == class_name]

layer_switcher = (widgets_of("WBP_IHPC_LayerSwitcher_C") or [None])[0]
hud = (widgets_of("WBP_IHPC_HUD_C") or [None])[0]

kind, _, arg = ACTION.partition(":")
if kind == "layer" and layer_switcher:
    layer = {"body": unreal.IHPCAnatomyLayer.BODY, "muscles": unreal.IHPCAnatomyLayer.MUSCLES,
             "skeleton": unreal.IHPCAnatomyLayer.SKELETON}[arg]
    layer_switcher.select_layer(layer)
elif kind == "depth" and pawn:
    # jump straight to a reveal depth (0 = skin ... 1 = skeleton), no animation
    pawn.get_component_by_class(unreal.AnatomyViewComp).set_reveal_depth(float(arg), False)
elif kind == "cam" and pawn:
    # cam:<yaw>,<pitch>,<arm length>  (world yaw/pitch of the spring arm; the camera looks along it at the pawn)
    yaw, pitch, length = (float(x) for x in arg.split(","))
    arm = pawn.get_component_by_class(unreal.SpringArmComponent)
    rot = unreal.Rotator(pitch, yaw, 0)
    if arm.get_editor_property("use_pawn_control_rotation"):
        pc.set_control_rotation(rot)
    arm.set_world_rotation(rot, False, False)
    arm.set_editor_property("target_arm_length", length)
    mouse = pawn.get_component_by_class(unreal.CPP_MouseFunctionComp)
    if mouse:
        mouse.sync_from_current_spring_arm()
    print("camera arm", arm.get_world_rotation(), length)
elif kind == "switch" and pc:
    pc.switch_pawn(int(arg))
    pawn = pc.get_controlled_pawn()
    print("now possessing:", pawn.get_class().get_name() if pawn else None)
elif kind in ("play", "pause", "time", "speed") and pawn:
    tl = pawn.get_component_by_class(unreal.AnimationTimelineComp)
    if kind == "play": tl.play_forward()
    elif kind == "pause": tl.pause()
    elif kind == "time": tl.pause(); tl.set_normalized_time(float(arg))
    elif kind == "speed": pawn.set_editor_property("speed", float(arg), unreal.PropertyAccessChangeNotifyMode.NEVER)

anat = pawn.get_component_by_class(unreal.AnatomyViewComp) if pawn else None
tl = pawn.get_component_by_class(unreal.AnimationTimelineComp) if pawn else None
if anat:
    print("anatomy: layers %s | depth %.3f | opacities %s" % (
        [str(l.layer_name) for l in anat.get_editor_property("layers")], anat.get_reveal_depth(),
        [round(anat.get_layer_opacity(i), 2) for i in range(anat.get_num_layers())]))
    for c in pawn.get_components_by_class(unreal.MeshComponent):
        if c.get_name() in ("Body", "Face", "MuscleMesh", "SkeletonMesh", "Hair"):
            m = c.get_material(0)
            print("   %-12s visible=%s mat0=%s" % (c.get_name(), c.is_visible(), m.get_name() if m else None))
else:
    print("anatomy: no AnatomyViewComp on the pawn")
if tl:
    print("timeline t=%.3f playing=%s cycle=%.3fs" % (tl.get_normalized_time(), tl.is_playing(), tl.get_cycle_length()))
if layer_switcher:
    print("layer switcher: selected=%s enabled=%s can_drive=%s" % (
        layer_switcher.get_selected_layer(), layer_switcher.get_is_enabled(), layer_switcher.can_drive_avatar_layers()))
print("HUD:", hud.get_name() if hud else None)
print("in viewport:", sorted({w.get_class().get_name() for w in all_widgets() if w.is_in_viewport()}))
