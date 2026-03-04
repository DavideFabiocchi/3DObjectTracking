# IDS Objects (quick start)

This folder contains assets and YAML metafiles for running IDS RGB tracking with M3T.

## Files
- `cube.obj`
- `cube.yaml`
- `cube_detector.yaml`

## Run command
```bash
cd /home/ramslab/3DObjectTracking
source IDS_cpp_codex/ids_backend/setup_ids_peak_env.sh

./build_m3t/examples/run_on_ids_camera_sequence \
  /home/ramslab/3DObjectTracking/M3T/examples/ids_camera_example.yaml \
  /home/ramslab/3DObjectTracking/M3T/data/ids_objects \
  cube
```

## Notes
- `cube_detector.yaml` currently initializes the object at `z = 0.50 m` in camera frame.
- If the object is not visible or tracking does not converge, adjust `link2world_pose`.
