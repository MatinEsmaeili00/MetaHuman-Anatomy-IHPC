# 01 — Investigation: what each project has and where they collide

Date: 2026-11-07. All paths are under `~/Perforce/maclab_digitalhuman_main/`.

## The projects

| Folder | Engine | Matches | Notes |
|---|---|---|---|
| `Claude/MetaHumanTestCopy2Mac` | 5.8 | [Abby-MetaHuman-Animation](https://github.com/MatinEsmaeili00/Abby-MetaHuman-Animation) `project/` mirror, byte for byte (Source, plugin) | Abby anatomy branch. MCP on port 8000 |
| `Claude/MetaHumanTestCOPY2` | 5.8 | [mblumpkin/MetaHumanTest](https://github.com/mblumpkin/MetaHumanTest) `main` (`6f8ed16`), byte for byte (Source, Config, `.uproject`) | IHPC UI branch. **Target.** MCP on port 8001 |
| `MetaHumanTestCopy2MacB1` | 5.8 | — | Snapshot of the Mac branch **before its C++ work** = common ancestor of the C++ |

### C++: three-way picture

`MetaHumanTestCopy2MacB1/Source` is the common ancestor:

* **COPY2 = ancestor + IHPC UI**: new `Public|Private/IHPC_UI/` (13 classes, ~3,700 lines), plus small additive edits to
  `CPP_PlayerController.h/.cpp` (`MainHUDWidgetClass`, `MainHUDZOrder`, `GetMainHUD`, HUD creation in `BeginPlay`)
  and `MetaHumanTest.Build.cs` (`UMG`, `Slate`, `SlateCore`).
* **Copy2Mac = ancestor + 23 Abby changes**: rewritten `UAnimationTimelineComp`, new `IAnimationTimelineSource`,
  `UCPP_LocomotionAnimInstance`, `UCPP_UW_MuscleInfo`, rewritten `UCPP_UW_AnatomyViewSwitcher` (thin forwarder to
  `UAnatomyViewComp`), controller pawn switching / muscle picking, removed editor-only `CPP_WBP_SpeedSlider`, plus the
  `MetaHumanAnatomyLayers` plugin (`UAnatomyViewComp`, `UAnatomyPickerComp`, `UMuscleAtlas`).

So the merge is: **Abby's Source + `IHPC_UI/` + the IHPC controller / Build.cs additions**, then two adaptations:

1. `IHPCUILibrary.cpp` reads `UAnimationTimelineComp::bIsPlaying`, which the rewrite replaced with `IsPlaying()`.
   Everything else it uses (`PlayForward`, `Pause`, `SetNormalizedTime`, `GetNormalizedTime`, `PlaySpeed`) still exists.
2. `UCPP_UW_IHPCLayerSwitcher` is a child of `UCPP_UW_AnatomyViewSwitcher` and moved the parent's hidden slider to
   set a `Fade` value (1 / 0.5 / 0.2) on components tagged `WholePart` / `SkeletonPart` / ... In the Abby version the
   parent forwards to `UAnatomyViewComp`, so the buttons will call `ShowLayer` directly (Skin / Muscles / Skeleton).

### Assets

Every asset the Abby work tracks (`tools/tracked_assets.txt` in the Abby repo), compared by MD5:

| Asset | Abby branch | COPY2 | Plan |
|---|---|---|---|
| `MetaHumans/MH_CH_Abby5/BP_MH_CH_Abby8` | changed (MuscleMesh / SkeletonMesh, AnatomyView, AnatomyPicker, timeline ABP) | original (never touched by an IHPC commit) | take Abby's |
| `Blueprints/BP_PlayerController` | changed (pawn list, MuscleInfo widget, speed range) | changed (HUD class, layer switcher, prototype timeline cleared) | **merge**: Abby's + IHPC settings |
| `Widget/WB_PawnSwitcher`, `WB_AnatomyViewSwitcher`, `WB_AmationTimeline`, `Blueprints/BP_SpeedSlider` | recompiled against new C++ | original | take Abby's |
| `Widget/WB_MuscleInfo` | new | absent | copy |
| `Abby_Animation/Enhanced/*` (`ABP_Abby_Timeline`, ...) | new | absent | copy |
| `Abby_Animation/Posture/*` (`RTG_Mixamo_To_Abby_Posture`, `*_Upright`) | new | absent | copy |
| `MadisonTesting/BeauMHCharacter/*` (Beau, 566 MB) | new | absent | copy |
| `Main.umap` | changed | changed (IHPC Level Blueprint fixes, `Muscles` folder) | keep COPY2's |
| `Plugins/MetaHumanAnatomyLayers` (162 MB) | new | absent | copy + extend |

## Level `Main` in COPY2 (queried through the editor's MCP server)

| Actor | Where | Size | Notes |
|---|---|---|---|
| `BP_MH_CH_Abby8` | (420, 80, 0) | — | The player character. Original version (no anatomy layers yet) |
| 186 × `SM_SM_*` in folder **Muscles** | pivot (320, 80, 0), scale 0.00098 | together 99 × 27 × 149 cm, z 2.7 → 152 | **93 muscles × 2 sides**: `…r` = right, `…r001` = the mirrored left copy (mirrored about x = 320). Arms out (A-pose). Assets in `/Game/3D_Models/Muscle/RightSideM/` |
| `SKELeton` (static mesh actor) | (−150, −710, 0) | 67 × 26 × 182.5 cm | `/Game/Skeleton3D/SKELeton/StaticMeshes/SKELeton`: **one** mesh, **one** material slot (`Humam_Skeleton_M`), 48,750 vertices, arms down |
| `Abby4` (static mesh actor) | (1130, 1520, 240) | empty bounds | — |
| `BP_PlayerPlaceHolder` 1–4 + `…ManagerA` | spawn points | — | used by the controller |

The muscle set sits 1 m to Abby's side, at her height and in an A-pose, so it was most likely laid out from Abby.
The `SKELeton` is a generic 1.82 m skeleton (arms down), so it has to be re-fitted to Abby's proportions and pose.

## Unreal MCP notes

* Native tools are only `list_toolsets`, `describe_toolset`, `call_tool`; ~60 toolsets behind them (actors, assets,
  Blueprints, static / skeletal meshes, UMG, materials, PIE, viewport capture...).
* `ProgrammaticToolset.execute_tool_script` batches calls in a sandboxed Python (`json`, `math`, `re`, ... only, no
  `unreal` module). Full editor Python still needs the file-based bridge.
* `find_actors` needs `name`, `tag` and `collision_channels` all present (`""` / `[]` are fine).
