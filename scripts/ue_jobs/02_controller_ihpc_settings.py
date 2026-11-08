# 02 - BP_PlayerController = the Abby branch's version + the IHPC settings that COPY2's version had:
#      Main HUD = WBP_IHPC_HUD, Anatomy View Switcher = WBP_IHPC_LayerSwitcher (replaces WB_AnatomyViewSwitcher),
#      no prototype timeline (the IHPC timeline bar inside the HUD replaces WB_AmationTimeline).
#      Kept from Abby: speed slider (gait speed), pawn switcher, muscle info widget, pawn list.
import unreal

def cls(path):
    bp = unreal.load_asset(path)
    return unreal.BlueprintEditorLibrary.generated_class(bp)

pc_bp = unreal.load_asset("/Game/Blueprints/BP_PlayerController")
cdo = unreal.get_default_object(unreal.BlueprintEditorLibrary.generated_class(pc_bp))
cdo.set_editor_property("main_hud_widget_class", cls("/Game/UserInterface/IHPC/Widgets/WBP_IHPC_HUD"))
cdo.set_editor_property("anatomy_view_switcher_widget_class", cls("/Game/UserInterface/IHPC/Widgets/WBP_IHPC_LayerSwitcher"))
cdo.set_editor_property("timeline_widget_class", None)
unreal.BlueprintEditorLibrary.compile_blueprint(pc_bp)
print("saved:", unreal.EditorAssetLibrary.save_loaded_asset(pc_bp, only_if_is_dirty=False))

cdo = unreal.get_default_object(unreal.BlueprintEditorLibrary.generated_class(pc_bp))
for p in ["main_hud_widget_class", "anatomy_view_switcher_widget_class", "timeline_widget_class", "speed_slider_widget",
          "pawn_switcher_widget_class", "muscle_info_widget_class"]:
    v = cdo.get_editor_property(p)
    print(f"  {p} = {v.get_name() if v else None}")

# The layer switcher's C++ changed (Fade values -> layer names): recompile and save its Widget Blueprint too.
ls_bp = unreal.load_asset("/Game/UserInterface/IHPC/Widgets/WBP_IHPC_LayerSwitcher")
unreal.BlueprintEditorLibrary.compile_blueprint(ls_bp)
ls_cdo = unreal.get_default_object(unreal.BlueprintEditorLibrary.generated_class(ls_bp))
print("LayerSwitcher layers:", ls_cdo.get_editor_property("body_layer_name"), ls_cdo.get_editor_property("muscles_layer_name"),
      ls_cdo.get_editor_property("skeleton_layer_name"), "| badge:", ls_cdo.get_editor_property("show_skeleton_badge"))
print("saved:", unreal.EditorAssetLibrary.save_loaded_asset(ls_bp, only_if_is_dirty=False))
