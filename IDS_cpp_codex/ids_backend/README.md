# IDS Backend (GUI-independent)

This folder contains a first extraction of IDS camera control logic from
`industrial_gui.cpp`, without Qt/Win32 GUI dependencies.

## Purpose
Provide a reusable backend that can later be integrated into `M3T`
(`IdsColorCamera`) and tested independently.

## Components
- `ids_single_camera_backend.h/.cpp`
  - init/close IDS peak library
  - open one camera by index
  - apply runtime camera config (ROI/FPS/exposure/gain/pixel format)
  - start/stop stream
  - grab frame into raw byte buffer
- `ids_backend_smoke.cpp`
  - simple CLI to validate open -> stream -> read N frames -> stop
- `build_backend_smoke.sh`
  - Linux build script (autodetects IDS peak include/lib path)

## Build
```bash
cd IDS_cpp_codex/ids_backend
source ./setup_ids_peak_env.sh
./build_backend_smoke.sh
```

## Run
```bash
./ids_backend_smoke [camera_index] [n_frames]
```

Example:
```bash
source ./setup_ids_peak_env.sh
./ids_backend_smoke 0 20
```

## Notes
- This is the first extraction step and currently supports index-based camera
  selection.
- Serial-based selection and richer metadata will be added in the next step
  before wiring into M3T.
- `setup_ids_peak_env.sh` prefers system `.deb` installation paths and can also
  be called with a custom IDS root path.
