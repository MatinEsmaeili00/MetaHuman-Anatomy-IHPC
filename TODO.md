# TODO / Progress

Status key: `[x]` done · `[~]` in progress · `[ ]` not started · `[!]` blocked / needs a decision

Target project: `Claude/MetaHumanTestCOPY2` (IHPC UI). Source of the functionality: `Claude/MetaHumanTestCopy2Mac` (Abby).

## 0. Setup and investigation
- [x] MCP access: Unreal (both projects, ports 8000 / 8001), Blender (MCP for Blender 1.8), GitHub (`gh`)
- [x] Both local projects match their GitHub repos exactly (Source, Config, `.uproject`)
- [x] Common ancestor of the C++: `MetaHumanTestCopy2MacB1` (COPY2 = ancestor + IHPC UI; Copy2Mac = ancestor + 23 Abby changes)
- [x] Asset conflicts found: `BP_PlayerController` (both changed), `Main.umap` (both changed); `BP_MH_CH_Abby8` only changed by Abby
- [x] Level `Main` (COPY2): 186 segmented muscle actors (`Muscles` folder, 93 muscles × left/right), `SKELeton` static mesh actor, `BP_MH_CH_Abby8`
- [x] Inventory of `BlenderContent/` ([docs/02](docs/02-blender-content.md)): `Skelly_Base.blend` is the source of the `SM_SM_*` muscles and has a per-bone named skeleton fitted to an Abby A-pose model
- [x] This repo, README, TODO
- [ ] Full system documentation ([docs/00](docs/00-system-overview.md)), kept up to date as the work lands

## 1. Merge the C++ (Abby → COPY2)
- [x] Back up every COPY2 file that gets changed (`Saved/ClaudeBackup/`)
- [x] Copy the `MetaHumanAnatomyLayers` plugin into COPY2, enable it in the `.uproject`
- [x] Source: take Abby's versions of the 23 changed files, keep `IHPC_UI/`, re-apply the IHPC additions to `CPP_PlayerController` (`MainHUDWidgetClass`, `MainHUDZOrder`, `GetMainHUD`) and `Build.cs` (`UMG`, `Slate`, `SlateCore`)
- [x] Adapt `IHPCUILibrary` to the rewritten `UAnimationTimelineComp` (`bIsPlaying` → `IsPlaying()`)
- [x] Rewrite `UCPP_UW_IHPCLayerSwitcher`: Body / Muscles / Skeleton buttons call `UAnatomyViewComp::ShowLayer` (no opacity slider, no Fade values, no tag lookups); re-bind on pawn change
- [x] Build `MetaHumanTestEditor` (Mac, Development) with no errors

## 2. Port the Abby assets
- [x] Copy: `Abby_Animation/Enhanced`, `Abby_Animation/Posture`, `MadisonTesting/BeauMHCharacter` (Beau), `Widget/WB_MuscleInfo`, and Abby's `WB_PawnSwitcher`, `WB_AnatomyViewSwitcher`, `WB_AmationTimeline`, `BP_SpeedSlider`, `BP_MH_CH_Abby8`
- [~] `BP_PlayerController`: Abby's version + the IHPC settings (Main HUD = `WBP_IHPC_HUD`, Anatomy View Switcher = `WBP_IHPC_LayerSwitcher`, no prototype timeline)
- [x] Keep COPY2's `Main` level (IHPC fixes + `Muscles` folder + `SKELeton`)
- [ ] `BP_GameMode`: Abby's (Default Pawn = `BP_MH_CH_Abby8`; COPY2 spawned `BP_MH_CH_Abby9`) — copied, verify in PIE
- [ ] Check references: nothing points at a missing asset

## 3. Segmented anatomy that follows the animation
- [ ] Export Abby's body (MetaHuman skeleton + weights) and the segmented muscles / skeleton to Blender
- [ ] Fit the anatomy to Abby's reference pose
- [ ] Muscles: skin to Abby's skeleton (weights transferred from her body), one ID per muscle
- [ ] Skeleton: each bone piece rigidly bound to one MetaHuman bone, one ID per bone
- [ ] Import as skeletal meshes with MetaHuman bone names (Leader Pose on `Body`, like the face)
- [ ] Atlas data: readable name, side, group and info text for every muscle and bone (labels for hover / click)

## 4. Wire everything to the IHPC UI
- [ ] Layer switcher buttons → `AnatomyView` on the possessed pawn (Abby or Beau)
- [ ] IHPC timeline bar → `UAnimationTimelineComp` (play / pause / speed / scrub / restart / markers)
- [ ] Muscle (and bone) hover name + click info popup in the matching layer
- [ ] Analysis panel "Active Muscles" list from the atlas
- [ ] Pawn switching keeps working with the new UI

## 5. Verify and document
- [ ] Play-In-Editor: every button, walk / jog / run, both characters, screenshots
- [ ] [docs/00-system-overview.md](docs/00-system-overview.md): full system and structure
- [ ] Mirror changed files into `project/`, push
