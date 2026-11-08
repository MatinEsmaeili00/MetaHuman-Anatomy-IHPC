#!/bin/zsh
# Add a MetaHuman Character (MHC_*) as a switchable anatomy pawn, end to end, in the running editor (bridge on):
#   tools/ue/add_metahuman_pawn.sh Grant /Game/MadisonTesting/GrantMHCharacter/MHC_Grant
# 1) job 19: assemble (downloads texture sources first if needed - log in to the MetaHuman service in the editor first)
# 2) job 16: BP_<Name>_Pawn from BP_Beau_Pawn + the assembled parts, add to the pawn switcher + a spawn placeholder
# 3) job 17: Action clips + layer fade lengths on every switchable pawn
set -e
NAME="${1:?name, e.g. Grant}"; MHC="${2:?MHC asset path}"
REPO="$(cd "$(dirname "$0")/../.." && pwd)"; JOBS="$REPO/scripts/ue_jobs"; TMP="${TMPDIR:-/tmp}/addpawn_$$"
mkdir -p "$TMP"
FOLDER="${MHC%/*}"; MHCNAME="${MHC##*/}"
sed "s|__MHC__|$MHC|" "$JOBS/19_assemble_metahuman.py" > "$TMP/19.py"
python3 -I "$REPO/tools/ue/ue_bridge.py" "$TMP/19.py" --timeout 3600
sed -e "s|__NAME__|$NAME|" -e "s|__ASSEMBLED__|$FOLDER/Unpacked/$MHCNAME/BP_$MHCNAME|" -e "s|__PAWN__|$FOLDER/BP_${NAME}_Pawn|" \
  "$JOBS/16_make_metahuman_pawn.py" > "$TMP/16.py"
python3 -I "$REPO/tools/ue/ue_bridge.py" "$TMP/16.py" --timeout 900
python3 -I "$REPO/tools/ue/ue_bridge.py" "$JOBS/17_wire_actions_and_layers.py" --timeout 900
rm -rf "$TMP"
echo "done: $FOLDER/BP_${NAME}_Pawn is in the pawn switcher"
