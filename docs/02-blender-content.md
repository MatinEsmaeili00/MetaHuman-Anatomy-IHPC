# 02 — What's in `BlenderContent/`

Inventoried with headless Blender 5.2.3 (`tools/blender/blend_inventory.py`, `--factory-startup`, nothing modified).

| File | Size | What it is | Useful here? |
|---|---|---|---|
| **`Skelly_Base.blend`** | 145 MB | **Full-body anatomy rig fitted to Abby.** Contains `AbbyAPoseModel_LOD0` (Abby's body in A-pose, 61k verts), a **segmented skeleton** (≈200 named bone meshes: `Femur.l/.r`, `Tibia`, `Fibula`, `Clavicle`, every vertebra `Atlas`, `Axis`, `C3`…`L5`, `Sacrum`, `Coccyx`, `Pelvis`, ribs, `Sternum`, carpals, metacarpals, phalanges, tarsals, cuneiforms, skull bones…) parented to a 59-bone `Armature` (`Pelvis`, `Femur.l`, `Tibia.l`, `Humerus.l`, `Radius.l`, `Ulna.l`, `T1`…`L5`, `C3`…`Atlas`, `Head`, `Jaw`, `Clavicle.l`…), and **108 muscle meshes** in per-muscle collections (`Biceps Brachii.R`, `Adductor Brevis M.R`, … 88 right, 20 left), each with its own small muscle rig (`… System.R`: bone / extensor / flexor / counter bones) and soft-body + subdivision modifiers. | **Yes — the most useful file.** It is the **source of the `SM_SM_*` muscles** in the Unreal level (same names: `Abductor Pollicis Longus.R` → `SM_SM_Abductorpollicislongusr`) and gives them proper display names. Its skeleton is **already segmented, already named per bone and already fitted to Abby's A-pose**, so no labelling by hand. |
| `CTScanFiles.blend` | 55 MB | CT segmentation of a torso (`volume-22 segmentation_*`): sacrum, pelvis, scapulae, clavicles, femurs, C7, T1–T12, L1–L5, S1 vertebrae, spinal cord, plus hand-made `Ribs`, `Sternum`, `Coastal` cartilage, `CTEthmoid`. 32 meshes, ~1 M verts, no rig. | Reference / high-detail scan data. Not fitted to Abby, very dense; not a drop-in replacement. |
| `MRISkull.blend` | 317 MB | Skull work: Visible-Human-Female CT head segmentations (`skull`, `head`, `mandible`, `Marrow`, sinuses, nasal passage, teeth), MRI face exports (1.6 M verts), and skull bones (`Frontal`, `Parietal`, `Sphenoid`, `Ethmoid`, `Maxilla`, `Zygomatic`, `Nasal`, `Lacrimal`, `Vomer`) fitted to the MRI in three passes (`Fitted`, `Fitted2`, `MRI_Fitted_Bones`). 54 meshes, 6.4 M verts. | Only for a detailed head / skull layer later. Far too dense for real-time as is. |
| `dicom_pointcloud_importer-20260722-121142.zip` | 1.3 MB | A Blender add-on (DICOM → point cloud / isosurface / segmentation operators, vendored `pydicom`). | Tooling that produced the CT meshes. Not needed in Unreal. |
| `WavingSkel.png` | 7 MB | A render of the skeleton. | Reference image. |
| `references/` | 33 MB | Reference images: skull cross-section, `MainSkel.jpg`, `MainSkel2.jpg`, `sphenoid.png`, `spinal-anatomy.jpg`, two renders. | Reference only. |

## Scale and placement in `Skelly_Base.blend`

| Part | World bounds (Blender units) |
|---|---|
| `AbbyAPoseModel_LOD0` | x −6.95 … 6.95, z 1.77 … 24.87 (23.1 tall) |
| Skeleton pieces | x −7.13 … 7.13 (roughly), z ≈ 1.8 … 24.9 |
| Muscles | x −7.69 … 3.30 (mostly the right side), z −0.6 … 23.3 |
| `Armature` joints | x −6.59 … 6.59, z 1.94 … 24.58 |

The anatomy lives inside the Abby model at about 1/7 scale, so mapping it to Abby in Unreal is a single
scale + offset (to be measured against her exported body).

## Recommendation

* **Muscles:** use the `SM_SM_*` set in the level as asked (both sides are there). Take the readable names from
  `Skelly_Base.blend`'s collection names.
* **Skeleton:** the Unreal `SKELeton` asset is a generic 1.82 m skeleton with its arms down, one mesh and one material:
  its bones would have to be split, named and re-posed to fit Abby by hand. `Skelly_Base.blend` has the same kind of
  segmented skeleton **already named per bone and already fitted to Abby's A-pose**, so it is the better source for an
  accurate, easy-to-label skeleton layer.
