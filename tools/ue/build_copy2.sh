#!/bin/zsh
# Build MetaHumanTestEditor for MetaHumanTestCOPY2 (close the COPY2 editor first).
# When ANOTHER Unreal Editor is running (e.g. the Copy2Mac project), UnrealBuildTool switches to hot-reload naming
# (libUnrealEditor-MetaHumanTest-000N.dylib) and does NOT update Binaries/Mac/UnrealEditor.modules, so a fresh editor
# would load the previous library. After the build this script points each manifest at its newest library.
set -e
PROJECT_DIR="${PROJECT_DIR:-/Users/mesmae6/Perforce/maclab_digitalhuman_main/Claude/MetaHumanTestCOPY2B}"
ENGINE="${ENGINE:-/Users/Shared/Epic Games/UE_5.8}"
if pgrep -f "UnrealEditor $PROJECT_DIR/MetaHumanTest.uproject" >/dev/null; then
  echo "Close the COPY2 editor first"; exit 1
fi
"$ENGINE/Engine/Build/BatchFiles/Mac/Build.sh" MetaHumanTestEditor Mac Development -Project="$PROJECT_DIR/MetaHumanTest.uproject" -WaitMutex 2>&1 \
  | grep -E "^\[[0-9]+/[0-9]+\]|error|Result:" || true
fix_manifest() {  # $1 = Binaries/Mac dir, $2 = module name
  local dir="$1" mod="$2" newest
  newest=$(ls -t "$dir"/libUnrealEditor-$mod*.dylib 2>/dev/null | head -1)
  [ -n "$newest" ] && sed -i '' -E "s/libUnrealEditor-$mod(-[0-9]+)?\.dylib/$(basename "$newest")/" "$dir/UnrealEditor.modules" \
    && echo "$mod -> $(basename "$newest")"
}
fix_manifest "$PROJECT_DIR/Binaries/Mac" MetaHumanTest
fix_manifest "$PROJECT_DIR/Plugins/MetaHumanAnatomyLayers/Binaries/Mac" MetaHumanAnatomyLayers
