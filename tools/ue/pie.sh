#!/bin/zsh
# Drive the running PIE session of COPY2:  tools/ue/pie.sh status | layer:muscles | play | pause | time:0.5 | speed:0.9 | switch:1
REPO="$(cd "$(dirname "$0")/../.." && pwd)"
TMP="${TMPDIR:-/tmp}/pie_probe_$$.py"
sed "s/__ACTION__/${1:-status}/" "$REPO/scripts/ue_jobs/03_pie_probe.py" > "$TMP"
python3 -I "$REPO/tools/ue/ue_bridge.py" "$TMP" --timeout "${2:-60}"
rm -f "$TMP"
