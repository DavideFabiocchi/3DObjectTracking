#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT_BIN="$ROOT_DIR/ids_backend_smoke"

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

IDS_PEAK_INCLUDE="${IDS_PEAK_INCLUDE:-}"
IDS_PEAK_LIB="${IDS_PEAK_LIB:-}"

if [[ -z "$IDS_PEAK_INCLUDE" ]]; then
  IDS_PEAK_INCLUDE="$(find_latest_dir /usr/include/ids_peak-*)"
fi
if [[ -z "$IDS_PEAK_LIB" ]]; then
  IDS_PEAK_LIB="$(find_latest_dir /usr/lib/ids_peak-*)"
fi

if [[ -z "$IDS_PEAK_INCLUDE" || ! -d "$IDS_PEAK_INCLUDE" ]]; then
  echo "[ERROR] IDS include dir not found. Set IDS_PEAK_INCLUDE." >&2
  exit 1
fi
if [[ -z "$IDS_PEAK_LIB" || ! -d "$IDS_PEAK_LIB" ]]; then
  echo "[ERROR] IDS lib dir not found. Set IDS_PEAK_LIB." >&2
  exit 1
fi

echo "IDS_PEAK_INCLUDE=$IDS_PEAK_INCLUDE"
echo "IDS_PEAK_LIB=$IDS_PEAK_LIB"

g++ -std=c++17 -O2 -pthread \
  "$ROOT_DIR/ids_single_camera_backend.cpp" \
  "$ROOT_DIR/ids_backend_smoke.cpp" \
  -I"$IDS_PEAK_INCLUDE" \
  -I"$IDS_PEAK_INCLUDE/ids-peak" \
  -L"$IDS_PEAK_LIB" -Wl,-rpath,"$IDS_PEAK_LIB" \
  -lids_peak \
  -o "$OUT_BIN"

echo "[OK] Built $OUT_BIN"
