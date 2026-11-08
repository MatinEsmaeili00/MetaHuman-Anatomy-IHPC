# MetaHuman Anatomy × IHPC UI

Merging two branches of the **MetaHumanTest** Unreal Engine 5.8 project into one:

| Source | GitHub | Local copy (Perforce workspace) | What it brings |
|---|---|---|---|
| **Abby anatomy branch** | [MatinEsmaeili00/Abby-MetaHuman-Animation](https://github.com/MatinEsmaeili00/Abby-MetaHuman-Animation) | `Claude/MetaHumanTestCopy2Mac` | Anatomy layers (skin → muscles → skeleton) that follow Abby's walk / jog / run, animation timeline component, upright posture + running-neck fix, Beau as a second person, muscle hover / click info |
| **IHPC UI branch** | [mblumpkin/MetaHumanTest](https://github.com/mblumpkin/MetaHumanTest) | `Claude/MetaHumanTestCOPY2` | The Figma "Avatar Injury Prevention Program" UI: HUD + menu, analysis panel, timeline / playback bar, **Body / Muscles / Skeleton layer switcher** |

**Target:** `Claude/MetaHumanTestCOPY2` gets all of the Abby branch's functionality, driven by the **new IHPC UI**
(the layer-switcher buttons switch the skin layers, no opacity slider), and the anatomy layers use the
**segmented muscles** (`SM_SM_*`, the `Muscles` folder in `Main`) and a **segmented skeleton**, so every muscle and
bone is its own labelled part.

- **Progress / to-do:** [TODO.md](TODO.md)
- **Full system documentation (architecture, classes, assets, data flow):** [docs/00-system-overview.md](docs/00-system-overview.md)
- **Investigation (what each project has, what conflicts):** [docs/01-investigation.md](docs/01-investigation.md)
- **What's in the BlenderContent folder:** [docs/02-blender-content.md](docs/02-blender-content.md)
- **Merge steps (C++ + assets):** [docs/03-merge.md](docs/03-merge.md)

## Repo layout

| Path | What |
|---|---|
| `project/` | Mirror of the merged project's text files (`.uproject`, `Config/`, `Source/`, the plugin) and only the `.uasset` files this work created or changed. The full project (~7 GB) stays in Perforce. |
| `docs/` | Documentation, findings, screenshots |
| `scripts/ue_jobs/` | Unreal editor Python jobs, in the order they were run |
| `scripts/blender_jobs/` | Blender Python jobs (mesh processing, weights, IDs) |
| `tools/ue/` | `ue_mcp.py` (tiny client for the editor's built-in MCP server), `uescript.py` (run a toolset script) |
| `tools/blender/` | Blender inventory / alignment helpers |

## How Claude talks to the tools

* **Unreal:** UE 5.8's experimental *Unreal MCP* plugin (`ModelContextProtocol` + `AllToolsets`). Each project serves
  its own port: `MetaHumanTestCopy2Mac` → `127.0.0.1:8000/mcp`, `MetaHumanTestCOPY2` → `127.0.0.1:8001/mcp`
  (`Config/DefaultEditorPerProjectUserSettings.ini`, auto-start on). Full editor Python goes through a file-based
  bridge (see the Abby repo's `tools/claude_bridge.py`).
* **Blender:** *MCP for Blender* add-on (localhost:9876) for the live session, and headless `Blender -b` for batch jobs.
* **GitHub:** `gh` CLI.
