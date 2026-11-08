#!/bin/zsh
# Open MetaHumanTestCOPY2 in its own Unreal Editor with
#   - the MCP server (port 8001, auto-started by Config/DefaultEditorPerProjectUserSettings.ini)
#   - the file-based Python bridge (tools/ue/claude_bridge.py) started via -ExecCmds="py ..."
set -e
ENGINE_APP="${ENGINE_APP:-/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor.app}"
PROJECT="${PROJECT:-/Users/mesmae6/Perforce/maclab_digitalhuman_main/Claude/MetaHumanTestCOPY2B/MetaHumanTest.uproject}"
BRIDGE="$(cd "$(dirname "$0")" && pwd)/claude_bridge.py"
if pgrep -f "UnrealEditor $PROJECT" >/dev/null; then echo "COPY2 editor already running"; exit 1; fi
# -ExecutePythonScript quits the editor when the script returns (UE 5.8), so ExecCmds + py instead.
open -n -a "$ENGINE_APP" --args "$PROJECT" "-ExecCmds=py $BRIDGE"
echo launched
