#!/bin/zsh
# Mirror the merged project's text files + tracked .uasset files into project/ (run from anywhere).
set -e
REPO="$(cd "$(dirname "$0")/.." && pwd)"
PROJ="${PROJ:-/Users/mesmae6/Perforce/maclab_digitalhuman_main/Claude/MetaHumanTestCOPY2}"
DST="$REPO/project"
mkdir -p "$DST"
cp "$PROJ/MetaHumanTest.uproject" "$DST/"
rsync -a --delete "$PROJ/Source/" "$DST/Source/"
rsync -a --delete --exclude 'Windows/' "$PROJ/Config/" "$DST/Config/"
# Android File Server token: never publish it
sed -i '' -E 's/^(SecurityToken=).*/\1REDACTED/' "$DST/Config/DefaultEngine.ini"
# The plugin: everything except build products (its content is small enough to keep)
rsync -a --delete --exclude Binaries --exclude Intermediate --exclude '__pycache__' \
  "$PROJ/Plugins/MetaHumanAnatomyLayers/" "$DST/Plugins/MetaHumanAnatomyLayers/"
# Tracked game content
grep -v '^#' "$REPO/tools/tracked_assets.txt" | grep . | while read -r f; do
  if [ -f "$PROJ/$f" ]; then mkdir -p "$DST/$(dirname "$f")"; cp "$PROJ/$f" "$DST/$f"; else echo "missing: $f"; fi
done
echo "synced into $DST"
