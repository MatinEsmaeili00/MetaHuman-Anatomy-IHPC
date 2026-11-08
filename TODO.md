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
- [x] `BP_PlayerController`: Abby's version + the IHPC settings (Main HUD = `WBP_IHPC_HUD`, Anatomy View Switcher = `WBP_IHPC_LayerSwitcher`, no prototype timeline)
- [x] Keep COPY2's `Main` level (IHPC fixes + `Muscles` folder + `SKELeton`)
- [x] `BP_GameMode`: Abby's (Default Pawn = `BP_MH_CH_Abby8`; COPY2 spawned `BP_MH_CH_Abby9`) — verified in PIE
- [x] Check references: everything resolves (one stale PIE-world soft reference in `BP_MH_CH_Abby8`, inherited, harmless)
- [x] First PIE: IHPC HUD + timeline + layer switcher drive Abby's anatomy layers ([screenshot](docs/screenshots/11_merged_muscles.png))

## 3. Segmented anatomy that follows the animation
- [x] Measure: the level's muscle set sits on Abby's reference pose 1 m to her side (pivot 320, 80, 0) ([fit](docs/screenshots/12_fit_level_muscles_in_abby.png))
- [x] Skeleton: Skelly_Base fitted to Abby in Blender (global 7.53× fit + per-segment joint fit, 200 named parts) ([docs/04](docs/04-segmented-anatomy.md))
- [x] Muscles: 186 parts skinned with weights transferred from Abby's body, one ID per muscle (Geometry Script, exact bind pose)
- [x] Skeleton: each bone rigidly bound to one MetaHuman bone, one ID per bone
- [x] Skeletal meshes on a plugin copy of Abby's skeleton, wired into Abby + Beau (Leader Pose on `Body`)
- [x] Atlases: 186 muscles + 200 bones with name, side, region, function / running / injury text, picking samples
- [x] Second picker (`BonePicker`) for the skeleton layer; controller + muscle-info widget handle several pickers (C++ rebuilt)

## 4. Wire everything to the IHPC UI
- [x] Layer switcher buttons → `AnatomyView` on the possessed pawn (Abby or Beau)
- [ ] IHPC timeline bar → `UAnimationTimelineComp` (play / pause / speed / scrub / restart / markers)
- [~] Muscle (and bone) hover name + click info popup in the matching layer (built, testing in PIE)
- [ ] Analysis panel "Active Muscles" list from the atlas
- [ ] Pawn switching keeps working with the new UI

## 5. Verify and document
- [ ] Play-In-Editor: every button, walk / jog / run, both characters, screenshots
- [ ] [docs/00-system-overview.md](docs/00-system-overview.md): full system and structure
- [ ] Mirror changed files into `project/`, push
