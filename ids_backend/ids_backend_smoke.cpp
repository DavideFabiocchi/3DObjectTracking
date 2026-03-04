#include "ids_single_camera_backend.h"

#include <cstdlib>
#include <iostream>

int main(int argc, char** argv) {
  int camera_index = 0;
  int n_frames = 10;

  if (argc > 1) camera_index = std::atoi(argv[1]);
  if (argc > 2) n_frames = std::atoi(argv[2]);

  ids_backend::IdsSingleCameraBackend cam;
  std::string err;

  if (!cam.Initialize(&err)) {
    std::cerr << "Initialize failed: " << err << std::endl;
    return 1;
  }

  auto names = cam.ListDiscoveredCameras(&err);
  if (!err.empty()) {
    std::cerr << "List cameras warning: " << err << std::endl;
  }
  std::cout << "Discovered cameras: " << names.size() << std::endl;

  if (!cam.OpenByIndex(camera_index, &err)) {
    std::cerr << "OpenByIndex failed: " << err << std::endl;
    return 2;
  }

  ids_backend::IdsCameraConfig cfg;
  if (!cam.ApplyConfig(cfg, &err)) {
    std::cerr << "ApplyConfig failed: " << err << std::endl;
    return 3;
  }

  if (!cam.StartStreaming(&err)) {
    std::cerr << "StartStreaming failed: " << err << std::endl;
    return 4;
  }

  std::cout << "Streaming started. Applied FPS: " << cam.applied_fps_hz()
            << ", TickHz: " << cam.timestamp_tick_hz() << std::endl;

  for (int i = 0; i < n_frames; ++i) {
    ids_backend::IdsFrame frame;
    if (!cam.GrabFrame(1000, &frame, &err)) {
      std::cerr << "GrabFrame failed at frame " << i << ": " << err
                << std::endl;
      cam.StopStreaming();
      cam.Shutdown();
      return 5;
    }

    std::cout << "Frame " << i << " -> " << frame.width << "x"
              << frame.height << ", bpp=" << frame.bytes_per_pixel
              << ", ts=" << frame.timestamp_ticks << std::endl;
  }

  cam.StopStreaming();
  cam.Shutdown();
  std::cout << "Smoke test completed." << std::endl;
  return 0;
}
