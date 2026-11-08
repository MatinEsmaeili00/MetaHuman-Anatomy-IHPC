# MetaHuman Anatomy Layers (UE 5.8 plugin)

Muscle and skeleton layers that follow a MetaHuman body's animation. There's no extra Anim Blueprint and no per-frame cost.
It also contains an **Anatomy View** component that peels the character from skin to muscles to skeleton with one slider value, like an anatomy app, and an **Anatomy Picker** component that highlights the muscle under the mouse and reports which one was clicked.

## How it works

The Chaos Flesh "Emil" anatomy meshes use anatomical limb bone names (`humerus`, `ulna`, `femur`, `tibia`, `ankle`). MetaHuman bodies use `upperarm`, `lowerarm`, `thigh`, `calf`, `foot`. This plugin ships copies of the anatomy meshes with those limb bones **renamed to MetaHuman names**. Twist bones are paired by their measured position along each limb; for example, Emil `ulna_twist_01` maps to MetaHuman `lowerarm_twist_02`, so a name-based guess would have been wrong.

With matching bone names, the anatomy meshes use **Leader Pose** on the MetaHuman `Body`, just like the face and clothing. They then:

- follow any animation on the body: Anim BP, Sequencer, LiveLink, mocap;
- take the body's proportions, so the muscles and bones sit inside the skin;
- cost nothing extra at runtime (no extra pose evaluation).

## Contents

| Asset | What |
|---|---|
| `/MetaHumanAnatomyLayers/Anatomy/Meshes/SKM_Anatomy_Muscles_UpperBody` | Upper-body muscle mesh (MetaHuman bone names) |
| `/MetaHumanAnatomyLayers/Anatomy/Meshes/SKM_Anatomy_Skeleton_UpperBody` | Upper-body skeleton (bones) mesh |
| `/MetaHumanAnatomyLayers/Anatomy/Meshes/SKM_Anatomy_Muscles_UpperBody_Skeleton` | Their shared Skeleton asset |
| `/MetaHumanAnatomyLayers/Anatomy/Materials`, `/Textures` | Self-contained copies of the muscle and bone materials |
| `/MetaHumanAnatomyLayers/Anatomy/Materials/M_Anatomy_MuscleFade` | Masked muscle material with a dithered `Fade` (1 = visible, 0 = gone), used while the muscle layer fades |
| `/MetaHumanAnatomyLayers/Anatomy/Materials/M_Anatomy_XRay` | Translucent fresnel "x-ray" shell the skin turns into once it has faded below 15% |
| `/MetaHumanAnatomyLayers/Anatomy/Materials/M_Anatomy_Hidden` | Always-invisible material for slots that can't fade |
| `/MetaHumanAnatomyLayers/Anatomy/Materials/M_Anatomy_MuscleHighlight` | Translucent, unlit glow (depth test off) that lights only the muscle whose vertex-colour ID equals `HoverId` (cyan) or `SelectedId` (orange) |
| `/MetaHumanAnatomyLayers/Anatomy/Data/DA_MuscleAtlas_UpperBody` | `UMuscleAtlas`: 34 muscles (17 per side) with name, side, region, function, role in running, injury note, and 5,192 bone-relative sample points for picking |
| `Source/MetaHumanAnatomyLayers` → **`UAnatomyViewComp`** ("Anatomy View") | Component for the pawn: `SetRevealDepth(0..1)`, `ShowLayer(i)`, `OnRevealDepthChanged`. Layers are data (`Layers` array: Skin / Muscles / Skeleton by default) |
| `Source/MetaHumanAnatomyLayers` → **`UAnatomyPickerComp`** ("Anatomy Picker") | Component for the pawn: `UpdateHover(ray)`, `SelectHovered()`, `ClearSelection()`, `GetMuscleInfo(id)`, events `OnHoverChanged` / `OnSelectionChanged` |
| `Content/Python/anatomy_layers/build.py` | Editor tools: `build_all()` re-creates the meshes and materials from an anatomy source; `wire_blueprint()` attaches the layers **and the Anatomy View and Anatomy Picker components** to a MetaHuman Blueprint |
| `Tools/muscle_atlas/` | Offline muscle labelling: `label_muscles.py` (vertex → muscle), `write_ids.py` (IDs into the FBX vertex colours), `muscle_info.json` (the info text), `muscle_atlas_samples.json` |

Nothing in `Anatomy/` references `/Game`, so the folder works in any project. It's a C++ plugin now (one runtime module, no dependencies beyond Core / Engine), so the project needs to be a C++ project; Unreal builds the module the first time you open it.

## Anatomy View component

Add **Anatomy View** to the pawn (`wire_blueprint` does it). From UI or Blueprint:

- `SetRevealDepth(Depth)`: 0 = whole character, 0.5 = muscles, 1 = skeleton (with the default three layers). Use it for a slider.
- `ShowLayer(Index)`: animates to a layer. Use it for buttons.
- `OnRevealDepthChanged`: keeps a slider in sync while a button animates.

How each layer fades:

- **Skin:** MetaHuman skin, eye, teeth and hair materials already have a `Fade` scalar; the component drives it through dynamic material instances. Below 15% the skin switches to `M_Anatomy_XRay`, so the outline stays visible. Hair strands hide below 50%.
- **Muscles:** swap to `M_Anatomy_MuscleFade` while fading (layer `FadeMaterial`) and show their detailed material at full opacity.
- **Hidden meshes:** fully faded meshes are hidden. A hidden `Body` keeps ticking its pose, so the Leader Pose layers keep animating.

**Your own layers:** edit `Layers` on the component. Each entry lists component names or tags (outside → in); the first layer also takes every visible mesh nobody else claims. Each layer can also set its own `FadeMaterial` and `GhostMaterial`.

## Anatomy Picker component

Add **Anatomy Picker** next to Anatomy View (`wire_blueprint` does it). Each tick, your player controller passes it the mouse ray (`DeprojectMousePositionToWorld` → `UpdateHover`) and, on a click, calls `SelectHovered()`. Widgets bind to `OnHoverChanged` / `OnSelectionChanged` and read `GetMuscleInfo(Id)`.

- **Picking:** every muscle's sample points are stored relative to a bone. At runtime they're placed on the animated bones, and the front-most muscle within `PickRadius` (3.5 cm) of the ray wins. There's no collision or physics asset, and it follows any animation.
- **Only in muscle view:** picking is active while the `Muscles` layer is at least `MinLayerOpacity` (0.5) visible and the skin has already turned to x-ray.
- **Highlight:** the picker spawns a copy of the muscle mesh (`MuscleHighlight`, tagged `AnatomyIgnore`). It follows the same leader pose and uses `M_Anatomy_MuscleHighlight`, and is visible only while a muscle is hovered or selected. An Overlay Material would have been simpler, but overlays don't render on Leader Pose followers.

**Muscle IDs in vertex colours:** red = `(id + 0.5) / 255` in the FBX. The importer truncates float × 255, and the material reads the byte / 255 with no sRGB conversion, so it decodes `round(VertexColor.R × 255)`. To relabel:

1. Run `label_muscles.py <muscles.fbx> Tools/muscle_atlas <labels.json>`.
2. Run `write_ids.py <muscles.fbx> <labels.json> <muscles_ids.fbx>`.
3. Re-import the mesh from `<muscles_ids.fbx>` with vertex colours set to *Replace*.
4. Rebuild the atlas asset (repo jobs 55 and 56).

## Use it in another project

1. Copy `Plugins/MetaHumanAnatomyLayers/` into the other project's `Plugins/` folder, then enable it under Edit → Plugins.
2. In your MetaHuman Blueprint, add two **Skeletal Mesh** components **as children of `Body`**:
   - Skeletal Mesh Asset: `SKM_Anatomy_Muscles_UpperBody` / `SKM_Anatomy_Skeleton_UpperBody`
   - Anim Class: empty; Use Bounds From Leader Pose Component: on
3. Leader Pose: the stock MetaHuman construction script (`EnableMasterPose` on Body's children) sets it automatically. Otherwise call **Set Leader Pose Component (Body)** on both in BeginPlay or the construction script.

With Python enabled, step 2 (plus the Anatomy View component) is one call:

```python
from anatomy_layers import build
build.wire_blueprint("/Game/MetaHumans/MyHuman/BP_MyHuman")   # creates or updates MuscleMesh + SkeletonMesh under Body
```

## Rebuild from source

`build.build_all(metahuman_body="/Game/MetaHumans/<Name>/Body/SKM_<Name>_BodyMesh")` exports the Emil anatomy meshes to ASCII FBX, renames the bones in the file (so the new Skeleton gets MetaHuman names from the start), re-imports them into this plugin and re-points the materials. It needs the Chaos Flesh example content (`/Game/ExampleContent/5_5_ChaosFlesh`) in the project.

## Known limits

- These are **upper-body** anatomy meshes (the Emil example covers the torso, arms and upper legs).
- Emil is a larger male anatomy, so some muscles poke slightly through a smaller body's skin unless the skin is faded or translucent.
- Emil's finger bones are a little longer than a typical MetaHuman's, so fingertips can poke out.
- On a much heavier body (for example Beau), the anatomy looks slimmer than the skin, because the bones only set the proportions, not the girth.
- Muscle labels are rule-based (dominant bone and position), so a few vertices near muscle borders carry a neighbour's ID.
