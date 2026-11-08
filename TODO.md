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
- [x] Full system documentation ([docs/00](docs/00-system-overview.md))

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
- [x] IHPC timeline bar → `UAnimationTimelineComp` (play / pause / speed / seek verified through `UIHPCUILibrary`)
- [x] Muscle and bone hover name + click info popup in the matching layer ([tests](docs/05-testing.md))
- [x] Denser picking samples (6,820 muscles / 5,455 bones), radius 2 / 1.5 cm
- [ ] Optional: analysis panel "Active Muscles" list from the atlas (still the IHPC demo data)
- [x] Pawn switching keeps working with the new UI (Beau: layers, switcher re-sync, picking)

## 5. Verify and document
- [x] Play-In-Editor: layer buttons, pickers, timeline, both characters, screenshots ([docs/05](docs/05-testing.md))
- [x] [docs/00-system-overview.md](docs/00-system-overview.md): full system and structure
- [x] Mirror changed files into `project/`, push

## 6. Actions menu, new characters, layer amounts, highlight (2026-11-08, COPY2B) → [docs/06](docs/06-actions-characters-highlight.md)
- [x] MCP (`unreal-copy2`, port 8001) + GitHub access checked; editor bridge restarted in COPY2B
- [x] Team's manual C++ changes reviewed, kept and compiled (they had never been built in COPY2B)
- [x] Actions menu: `Action_<Name>` buttons → `UAnimationTimelineComp::PlayAction` / `StopAction`; green = playing; greyed out without clips
- [x] Madison clips retargeted to `metahuman_base_skel` (`AS_*_MH`, job 15); per-pawn `ActionClips` (job 17)
- [x] Madi: `MHC_Madi` assembled (job 19), `BP_Madi_Pawn` (job 16), in the pawn switcher, walk + muscles + bones + actions verified
- [!] Grant: `MHC_Grant` has no texture sources → needs the MetaHuman cloud login, then `tools/ue/add_metahuman_pawn.sh Grant /Game/MadisonTesting/GrantMHCharacter/MHC_Grant`
- [x] Layer buttons = skin amounts 1 / 0.5 / 0; `Skin.FadeLength` = 1 so 0.5 = muscles fully shown
- [x] Fix: layer buttons froze after one frame (slider `SetValue` echo in `UCPP_UW_AnatomyViewSwitcher`)
- [x] Fix: both pickers shared one highlight mesh (name clash) → muscles never glowed
- [x] Fix: part IDs in the vertex colours were sRGB-converted at build → glow on the wrong part; rewritten in place (job 18), job 08 updated
- [x] PIE tests with real clicks; screenshots; docs; mirror + push

## 7. Next steps (not done yet)
- [ ] Grant (login, then one command — see above)
- [ ] Inner-thigh muscle stretches in deep wide stances (weights transferred from Abby's body)
- [ ] Optional: in-place / camera-facing versions of the action clips; Madi-specific retargeted walk
- [ ] Trunk muscles: the level's set has no abdominals / obliques / lats / erector spinae / serratus / intercostals / neck / face muscles
- [ ] Triangle-accurate picking (CPU-skinned mesh ray test) instead of sample points
- [ ] Hide or move the source `Muscles` actors, the generic `SKELeton` and the extra placed Abby in `Main`
- [ ] Remove the stale `/Game/UEDPIE_0_Main` soft reference from `BP_MH_CH_Abby8` (inherited, Blueprint graph)
- [ ] Bone material: one dark patch on the left femur (UV-dependent material on a Skelly UV island); a UV-free bone material would fix it
- [ ] Perforce: the merged COPY2 files were changed outside Perforce (no `p4` CLI here); reconcile in P4V
