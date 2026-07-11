#!/usr/bin/env bash
# DEPRECATED — use root CMake builds instead. See docs/building.md

echo "compile.sh is deprecated. From repo root:" >&2
echo "  cmake -B cmake-build-debug -S ." >&2
echo "  cmake --build cmake-build-debug --target logo" >&2
echo "  cd game-dev/fgfx/js && npm install && npm run build  # for web" >&2
exit 1
