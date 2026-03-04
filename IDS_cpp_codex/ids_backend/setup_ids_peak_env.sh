#!/usr/bin/env bash
set -euo pipefail

# Usage:
#   source ./setup_ids_peak_env.sh [/absolute/path/to/ids-peak_root]

IDS_ROOT_DEFAULT="/home/ramslab/3DObjectTracking/third_party/ids-peak_2.20.0.0-408_amd64"
IDS_ROOT="${1:-$IDS_ROOT_DEFAULT}"

if [[ ! -d "$IDS_ROOT" ]]; then
  echo "[ERROR] IDS root not found: $IDS_ROOT" >&2
  return 1 2>/dev/null || exit 1
fi

export IDS_PEAK_INCLUDE="$IDS_ROOT/include"
export IDS_PEAK_LIB="$IDS_ROOT/lib/x86_64-linux-gnu"
export GENICAM_GENTL64_PATH="$IDS_ROOT/lib/x86_64-linux-gnu/ids-peak/cti"
export LD_LIBRARY_PATH="$IDS_PEAK_LIB:${LD_LIBRARY_PATH:-}"

echo "IDS_ROOT=$IDS_ROOT"
echo "IDS_PEAK_INCLUDE=$IDS_PEAK_INCLUDE"
echo "IDS_PEAK_LIB=$IDS_PEAK_LIB"
echo "GENICAM_GENTL64_PATH=$GENICAM_GENTL64_PATH"
