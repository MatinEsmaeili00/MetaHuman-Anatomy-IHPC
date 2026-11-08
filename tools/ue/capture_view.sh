#!/bin/zsh
# Capture the level viewport from a given camera (no annotations):
#   tools/ue/capture_view.sh out.png X Y Z PITCH YAW [port]
OUT="$1"; X=$2; Y=$3; Z=$4; P=$5; W=$6; PORT="${7:-8001}"
UE_MCP_IMAGE="$OUT" python3 "$(dirname "$0")/ue_mcp.py" "$PORT" call EditorToolset.EditorAppToolset.CaptureViewport \
"{\"captureTransform\":{\"location\":{\"x\":$X,\"y\":$Y,\"z\":$Z},\"rotation\":{\"pitch\":$P,\"yaw\":$W,\"roll\":0},\"scale\":{\"x\":1,\"y\":1,\"z\":1}},\"annotations\":{\"gridSpacing\":0,\"gridExtent\":0,\"gridHeight\":0,\"maxLabelDistance\":0,\"classFilter\":null,\"maxLabels\":0},\"bShowUI\":false}"
