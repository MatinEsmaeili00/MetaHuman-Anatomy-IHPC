#!/bin/zsh
# Screenshot of the running PIE viewport INCLUDING the game UI (console command "Shot showui"), through the
# file bridge (works even when the MCP server is busy):  tools/ue/pie_shot.sh out.png [max_px]
OUT="${1:?output png}"; MAXPX="${2:-1280}"
PROJECT_DIR="${UE_PROJECT:-/Users/mesmae6/Perforce/maclab_digitalhuman_main/Claude/MetaHumanTestCOPY2}"
SHOTS="$PROJECT_DIR/Saved/Screenshots"
before=$(find "$SHOTS" -name '*.png' 2>/dev/null | wc -l)
python3 -I "$(dirname "$0")/ue_bridge.py" -c "
import unreal
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
unreal.SystemLibrary.execute_console_command(w, 'Shot showui')
print('requested')" --timeout 60 >/dev/null
for i in $(seq 1 40); do
  newest=$(find "$SHOTS" -name '*.png' -newer "$0" -print0 2>/dev/null | xargs -0 ls -t 2>/dev/null | head -1)
  now=$(find "$SHOTS" -name '*.png' 2>/dev/null | wc -l)
  if [ "$now" -gt "$before" ] && [ -n "$newest" ]; then sleep 0.5; cp "$newest" "$OUT"; sips -Z "$MAXPX" "$OUT" >/dev/null; echo "saved $OUT"; exit 0; fi
  sleep 0.5
done
echo "no screenshot appeared in $SHOTS"; exit 1
