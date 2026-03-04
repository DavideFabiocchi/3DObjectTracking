# IDS Integration Plan for M3T (RGB-only, Multi-camera)

## Objective
Integrate IDS RGB cameras into M3T as `ColorCamera` sources to run multi-camera 3D object tracking with known `.obj` models, without requiring depth cameras.

## Scope
- In scope:
  - New IDS camera backend reusable outside GUI code
  - New M3T `IdsColorCamera` implementation
  - RGB-only tracking examples (single and multi-camera)
  - Optional YAML/generator integration
- Out of scope (initial phase):
  - Refactor or redesign of M3T core optimization
  - Full replacement of existing camera classes
  - Bundle adjustment / global extrinsic refinement

## Current State
- `M3T` supports color-only modalities (`RegionModality`, `TextureModality`) and can track without depth if configured accordingly.
- `IDS_cpp_codex/industrial_gui.cpp` already includes robust IDS camera control logic in `CameraController` (discovery, config, start/stop stream, frame loop).
- IDS code is currently tightly coupled with GUI application structure and must be extracted into a reusable backend.

## Integration Contract
The new IDS integration must provide:
1. Deterministic camera selection (`serial` preferred, fallback `index`).
2. Thread-safe frame acquisition returning `cv::Mat` in BGR8 or Mono8.
3. Stable lifecycle: `SetUp()` -> `UpdateImage()` repeated -> clean shutdown.
4. Runtime configuration:
   - ROI: `OffsetX`, `OffsetY`, `Width`, `Height`
   - exposure (ms), gain, fps
   - optional pixel format preference (BGR8/Mono8)
5. Intrinsics + pose consumed by M3T as standard camera metadata.
6. Build optionality: compile must succeed with `USE_IDS=OFF` on machines without IDS SDK.

## Step Plan (with strategic commits)

### Step 1 - Contract and delivery plan (this commit)
- Deliverables:
  - This document with API contract, milestones, test gates.
- Commit message:
  - `docs(m3t): define IDS RGB integration plan and test gates`

### Step 2 - Extract IDS backend from GUI code
- Deliverables:
  - New backend module under `IDS_cpp_codex/ids_backend/`
  - Core camera control moved out from `industrial_gui.cpp`
  - Simple CLI smoke executable for open/start/frame/stop
- Commit message:
  - `refactor(ids): extract reusable camera backend from industrial gui`
- Gate:
  - On IDS machine, smoke tool acquires N frames from each discovered camera.

### Step 3 - Add `IdsColorCamera` in M3T
- Deliverables:
  - `M3T/include/m3t/ids_camera.h`
  - `M3T/src/ids_camera.cpp`
  - CMake option `USE_IDS` (default OFF)
- Commit message:
  - `feat(m3t): add optional IdsColorCamera implementation`
- Gate:
  - `USE_IDS=OFF` build unchanged; `USE_IDS=ON` builds with IDS SDK.

### Step 4 - Single-camera RGB-only tracking example
- Deliverables:
  - New example `run_on_ids_camera_sequence.cpp`
  - Uses `RegionModality` (and optional `TextureModality`) without depth camera
- Commit message:
  - `feat(m3t/examples): add single IDS RGB tracking example`
- Gate:
  - Stable tracking on one camera and one object for >= 60s.

### Step 5 - Multi-camera RGB tracking example
- Deliverables:
  - New example `run_on_ids_multi_camera_sequence.cpp`
  - One modality per `(camera, body)` and merged optimization
- Commit message:
  - `feat(m3t/examples): add multi-camera IDS RGB tracking example`
- Gate:
  - 2+ cameras running simultaneously with consistent pose behavior.

### Step 6 - Calibration export bridge
- Deliverables:
  - Converter from IDS calibration outputs to M3T camera YAML files
- Commit message:
  - `feat(ids-tools): export IDS calibration outputs to M3T yaml`
- Gate:
  - Valid intrinsics/extrinsics loaded directly in M3T examples.

### Step 7 - Optional generator integration
- Deliverables:
  - `generator.h` support for `IdsColorCamera`
  - RGB-only config flow (depth camera no longer mandatory in generator path)
- Commit message:
  - `feat(m3t/generator): support IDS color cameras and RGB-only configs`
- Gate:
  - `run_generated_tracker` works from YAML with only color cameras.

### Step 8 - Hardening and cleanup
- Deliverables:
  - Runtime robustness checks (timeouts/reconnect/error messages)
  - Documentation update for setup/runbook
  - Remove unneeded binaries and GUI-only assets from `IDS_cpp_codex` after backend integration is complete
- Commit message:
  - `chore(ids): cleanup legacy gui artifacts after backend integration`
- Gate:
  - End-to-end workflow reproducible from clean clone.

## Test Strategy (per step)
- Build gates:
  - `M3T` with `USE_IDS=OFF` must always pass.
  - `M3T` with `USE_IDS=ON` must pass on IDS-enabled environment.
- Functional gates:
  - frame acquisition FPS and stability
  - no deadlock on repeated start/stop
  - correct camera mapping by serial/index
- Tracking gates:
  - object initialization success rate
  - pose continuity and jitter checks

## Future cleanup policy for IDS_cpp_codex
After Step 5-7 are validated, keep only:
- reusable backend source
- calibration/export utilities
- minimal docs required for maintenance

Remove from repo (or move to archive branch):
- compiled artifacts (`.exe`, `.pdb`, `.obj`, local logs)
- GUI-only experimental assets not used by M3T integration
