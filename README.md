# 3DObjectTracking (M3T + IDS RGB)

Repository focalizzato su `M3T` con integrazione IDS peak per tracking 3D RGB-only (monocamera oggi, estendibile a multicamera).

## Struttura
- `M3T/`: libreria tracker ed esempi
- `ids_backend/`: backend IDS riusabile (senza GUI), usato da `m3t::IdsColorCamera`

## Prerequisiti (Ubuntu)
- CMake (>= 3.16 consigliato)
- Compilatore C++17 (`g++`)
- OpenCV con modulo CUDA coerente con la tua toolchain (se richiesto dalla tua build locale)
- IDS peak SDK installato a sistema (`.deb`) oppure in cartella custom
- (Opzionale) CUDA 11.8 se la tua OpenCV è stata compilata contro CUDA 11.8

## Setup ambiente IDS
```bash
cd /home/ramslab/3DObjectTracking
source ids_backend/setup_ids_peak_env.sh
```

Il comando esporta variabili come `IDS_PEAK_INCLUDE`, `IDS_PEAK_LIB`, `GENICAM_GENTL64_PATH`.

## Configurazione e build (caso monocamera IDS)
```bash
cd /home/ramslab/3DObjectTracking
source ids_backend/setup_ids_peak_env.sh

cmake -S M3T -B build_m3t \
  -DUSE_IDS=ON \
  -DIDS_PEAK_INCLUDE_DIR="$IDS_PEAK_INCLUDE" \
  -DIDS_PEAK_LIBRARY="$IDS_PEAK_LIB/libids_peak.so" \
  -DUSE_AZURE_KINECT=OFF \
  -DUSE_REALSENSE=OFF \
  -DUSE_GTEST=OFF

cmake --build build_m3t -j$(nproc)
```

Se serve forzare CUDA 11.8 in fase CMake:
```bash
-DCUDAToolkit_ROOT=/usr/local/cuda-11.8 \
-DCUDA_TOOLKIT_ROOT_DIR=/usr/local/cuda-11.8
```

## Esecuzione esempio monocamera IDS
```bash
cd /home/ramslab/3DObjectTracking
source ids_backend/setup_ids_peak_env.sh

./build_m3t/examples/run_on_ids_camera_sequence \
  /home/ramslab/3DObjectTracking/M3T/examples/ids_camera_example.yaml \
  /home/ramslab/3DObjectTracking/M3T/data/ids_objects \
  cube
```

## Note configurazione camera (`ids_camera_example.yaml`)
- `apply_camera_settings: 1`: impone parametri da YAML (FPS, gain, exposure, pixel format)
- `apply_camera_settings: 0`: mantiene i parametri correnti impostati in IDS Cockpit/UserSet
- `swap_rb_channels: 1`: corregge swap Rosso/Blu se i colori appaiono invertiti

## Git e build locali
- Le directory di build (`build_m3t/`, `build/`, `M3T/build/`) sono ignorate da Git.
- Ogni sviluppatore compila localmente; non si versionano artefatti di build/binari.
