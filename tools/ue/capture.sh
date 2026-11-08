#!/bin/zsh
# Save the editor window (PIE included) to a PNG:  tools/ue/capture.sh out.png [port]
OUT="${1:?output png}"; PORT="${2:-8001}"
UE_MCP_IMAGE="$OUT" python3 "$(dirname "$0")/ue_mcp.py" "$PORT" call EditorToolset.EditorAppToolset.CaptureEditorImage '{}'
