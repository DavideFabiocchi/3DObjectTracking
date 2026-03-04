#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <peak/peak.hpp>

namespace ids_backend {

struct IdsCameraConfig {
  int roi_offset_x = 0;
  int roi_offset_y = 0;
  int roi_width = 1984;
  int roi_height = 1264;
  double fps = 60.0;
  double gain = 1.0;
  double exposure_ms = 3.0;
  std::string pixel_format = "BGR8";  // fallback to Mono8 if unsupported
};

struct IdsFrame {
  std::vector<uint8_t> bytes;
  int width = 0;
  int height = 0;
  int bytes_per_pixel = 0;
  bool is_mono8 = false;
  uint64_t timestamp_ticks = 0;
};

class IdsSingleCameraBackend {
 public:
  IdsSingleCameraBackend();
  ~IdsSingleCameraBackend();

  IdsSingleCameraBackend(const IdsSingleCameraBackend&) = delete;
  IdsSingleCameraBackend& operator=(const IdsSingleCameraBackend&) = delete;

  bool Initialize(std::string* err);
  void Shutdown();

  std::vector<std::string> ListDiscoveredCameras(std::string* err) const;

  bool OpenByIndex(int camera_index, std::string* err);
  bool ApplyConfig(const IdsCameraConfig& cfg, std::string* err);

  bool StartStreaming(std::string* err);
  void StopStreaming();

  bool GrabFrame(int timeout_ms, IdsFrame* frame, std::string* err);

  bool is_initialized() const;
  bool is_open() const;
  bool is_streaming() const;

  int camera_index() const;
  double applied_fps_hz() const;
  double timestamp_tick_hz() const;

 private:
  bool PrepareBuffers(std::string* err);
  void RevokeBuffers();

  mutable std::mutex api_mutex_;

  bool initialized_ = false;
  bool opened_ = false;
  std::atomic<bool> streaming_{false};
  int camera_index_ = -1;

  IdsCameraConfig config_{};
  double applied_fps_hz_ = 0.0;
  double timestamp_tick_hz_ = 1e9;

  std::shared_ptr<peak::core::Device> device_;
  std::shared_ptr<peak::core::NodeMap> node_map_;
  std::shared_ptr<peak::core::DataStream> data_stream_;
};

}  // namespace ids_backend
