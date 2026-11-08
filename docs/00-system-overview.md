# 00 — System overview (living document)

> Updated as the work lands. Sections marked *(planned)* describe the design before it is built.

## 1. What the user sees

One MetaHuman (Abby, or Beau after switching) walks / jogs / runs in the `Main` level. The **IHPC UI** from Figma
sits on top:

* **Title bar + menu** (`WBP_IHPC_HUD`): Profile, Progress, Saved Videos, History & Timeline, Settings.
* **Analysis panel** (Saved Videos): graphs, Active Muscles, Tagged Points, video cards that jump the animation.
* **Timeline / playback bar** (bottom centre): record, restart, pause, slower, scrub, faster, play, next marker, bookmark.
* **Layer switcher** (bottom right): **Body**, **Muscles**, **Skeleton**, which peel the avatar from skin to muscles
  to skeleton *(planned: driven by `UAnatomyViewComp`)*.
* **Muscle / bone info** *(planned)*: in the Muscles (or Skeleton) layer, hovering a part highlights it and shows its
  name; clicking opens an info popup.
* **Pawn switcher**: Abby → Beau → skeleton model.

## 2. Architecture (one rule)

The **PlayerController** owns input and creates the widgets. **Widgets only forward clicks** and mirror state.
The work is done by **components on the possessed pawn**. Possessing another pawn re-binds the widgets.

```
CPP_PlayerController (BP_PlayerController)
 ├─ creates WBP_IHPC_HUD ─────────────┐  (MainHUDWidgetClass, Z 10)
 │    ├─ WBP_IHPC_Timeline ───────────┼──► UAnimationTimelineComp   (on the pawn)
 │    └─ WBP_IHPC_AnalysisPanel ──────┘        └─► ABP_*_Timeline (UCPP_LocomotionAnimInstance) — walk/jog/run at a phase
 ├─ creates WBP_IHPC_LayerSwitcher ───────► UAnatomyViewComp        (on the pawn)  Skin / Muscles / Skeleton
 ├─ creates WB_PawnSwitcher ──────────────► SwitchPawn (Abby ⇄ Beau ⇄ skeleton model)
 ├─ creates WB_MuscleInfo ◄───────────────  UAnatomyPickerComp      (on the pawn)  hover / click a muscle or bone
 └─ mouse orbit / zoom ───────────────────► UCPP_MouseFunctionComp  (on the pawn)
```

## 3. Characters (pawns)

`BP_MH_CH_Abby8` (and `BP_Beau_Pawn`, same components):

```
Root
└─ Body (SkeletalMesh, MetaHuman body, Anim Class = ABP_Abby_Timeline)   ← leader
   ├─ Face, Hair, … (MetaHuman, Leader Pose)
   ├─ MuscleMesh   (SkeletalMesh with MetaHuman bone names, Leader Pose)  ← muscles layer  (planned: segmented muscles)
   └─ SkeletonMesh (SkeletalMesh with MetaHuman bone names, Leader Pose)  ← skeleton layer (planned: segmented skeleton)
Components: AnatomyView (UAnatomyViewComp), AnatomyPicker (UAnatomyPickerComp),
            AnimationTimeline (UAnimationTimelineComp), MouseFunction (UCPP_MouseFunctionComp)
```

Leader Pose: the anatomy meshes use the **same bone names** as the MetaHuman body, so they copy its pose every frame
with no extra Anim Blueprint, whatever drives the body (Anim BP, Sequencer, Live Link).

## 4. Code map *(filled in as files are merged)*

| Module / folder | Class | Role |
|---|---|---|
| `Plugins/MetaHumanAnatomyLayers` | `UAnatomyViewComp` | Layers from the outside in; reveal depth 0 = skin … 1 = skeleton; `ShowLayer` / `ShowLayerByName` animate; per-layer fade / x-ray ghost materials |
| | `UAnatomyPickerComp` | Hover / click picking of labelled parts; highlight |
| | `UMuscleAtlas` (`DA_MuscleAtlas_*`) | Per-part name, side, info text, sample points |
| `Source/MetaHumanTest` | `UAnimationTimelineComp`, `IAnimationTimelineSource` | Play / pause / reverse / scrub / step the gait cycle |
| | `UCPP_LocomotionAnimInstance` | Walk / jog / run at an explicit phase, running-neck correction |
| | `CPP_PlayerController` | Creates widgets, input, pawn switching, picking |
| | `UCPP_UW_MuscleInfo` | Tooltip + info popup |
| `Source/MetaHumanTest/*/IHPC_UI` | `UCPP_UW_IHPCHud`, `…Timeline`, `…LayerSwitcher`, `…AnalysisPanel`, … | The Figma UI (one C++ class + one Widget Blueprint per component) |

## 5. Asset map *(filled in as assets are ported)*

| Path | What |
|---|---|
| `/Game/UserInterface/IHPC/Widgets/WBP_IHPC_*` | IHPC widgets |
| `/Game/Widget/WB_*` | Pawn switcher, muscle info, old anatomy switcher / timeline (kept, not created) |
| `/Game/Abby_Animation/Enhanced/ABP_Abby_Timeline` | Abby's timeline-driven locomotion ABP |
| `/Game/Abby_Animation/Posture/*_Upright` | Upright walk / jog / run clips |
| `/Game/MadisonTesting/BeauMHCharacter/` | Beau (second person) |
| `/MetaHumanAnatomyLayers/Anatomy/…` | Anatomy meshes, materials, atlas |
