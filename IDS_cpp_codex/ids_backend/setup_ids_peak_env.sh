#!/usr/bin/env bash
set -euo pipefail

# Usage:
#   source ./setup_ids_peak_env.sh [optional_ids_root]
# If no argument is provided, system installation paths are preferred.

find_latest_dir() {
  local pattern="$1"
  local latest=""
  for d in $pattern; do
    if [[ -d "$d" ]]; then
      latest="$d"
    fi
  done
  echo "$latest"
}

IDS_ROOT="${1:-}"

if [[ -n "$IDS_ROOT" ]]; then
  if [[ ! -d "$IDS_ROOT" ]]; then
    echo "[ERROR] IDS root not found: $IDS_ROOT" >&2
    return 1 2>/dev/null || exit 1
  fi
  export IDS_PEAK_INCLUDE="$IDS_ROOT/include"
  export IDS_PEAK_LIB="$IDS_ROOT/lib/x86_64-linux-gnu"
  export GENICAM_GENTL64_PATH="$IDS_ROOT/lib/x86_64-linux-gnu/ids-peak/cti"
else
  # Prefer system package paths (ids-peak installed via .deb)
  export IDS_PEAK_INCLUDE="${IDS_PEAK_INCLUDE:-$(find_latest_dir /usr/include/ids_peak-*)}"
  export IDS_PEAK_LIB="${IDS_PEAK_LIB:-$(find_latest_dir /usr/lib/ids_peak-*)}"
  export GENICAM_GENTL64_PATH="${GENICAM_GENTL64_PATH:-/usr/lib/ids/cti}"

  if [[ -z "$IDS_PEAK_INCLUDE" || ! -d "$IDS_PEAK_INCLUDE" ]]; then
    echo "[ERROR] IDS include dir not found. Set IDS_PEAK_INCLUDE or pass IDS root." >&2
    return 1 2>/dev/null || exit 1
  fi
  if [[ -z "$IDS_PEAK_LIB" || ! -d "$IDS_PEAK_LIB" ]]; then
    echo "[ERROR] IDS lib dir not found. Set IDS_PEAK_LIB or pass IDS root." >&2
    return 1 2>/dev/null || exit 1
  fi
  if [[ ! -d "$GENICAM_GENTL64_PATH" ]]; then
    echo "[ERROR] GenTL path not found: $GENICAM_GENTL64_PATH" >&2
    return 1 2>/dev/null || exit 1
  fi
fi

export LD_LIBRARY_PATH="$IDS_PEAK_LIB:${LD_LIBRARY_PATH:-}"

echo "IDS_ROOT=${IDS_ROOT:-<system>}"
echo "IDS_PEAK_INCLUDE=$IDS_PEAK_INCLUDE"
echo "IDS_PEAK_LIB=$IDS_PEAK_LIB"
echo "GENICAM_GENTL64_PATH=$GENICAM_GENTL64_PATH"
