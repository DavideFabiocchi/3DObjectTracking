#include "ids_single_camera_backend.h"

#include <algorithm>

namespace ids_backend {

namespace {

bool SetPixelFormat(peak::core::NodeMap* node_map, const std::string& requested,
                    bool* is_mono8) {
  auto pixel_format =
      node_map->FindNode<peak::core::nodes::EnumerationNode>("PixelFormat");

  if (requested == "Mono8") {
    pixel_format->SetCurrentEntry("Mono8");
    *is_mono8 = true;
    return true;
  }

  try {
    pixel_format->SetCurrentEntry("BGR8");
    *is_mono8 = false;
    return true;
  } catch (...) {
    pixel_format->SetCurrentEntry("Mono8");
    *is_mono8 = true;
    return true;
  }
}

}  // namespace

IdsSingleCameraBackend::IdsSingleCameraBackend() = default;

IdsSingleCameraBackend::~IdsSingleCameraBackend() { Shutdown(); }

bool IdsSingleCameraBackend::Initialize(std::string* err) {
  std::lock_guard<std::mutex> lk(api_mutex_);
  if (initialized_) return true;

  try {
    peak::Library::Initialize();
    initialized_ = true;
    return true;
  } catch (const std::exception& e) {
    if (err) *err = e.what();
    return false;
  }
}

void IdsSingleCameraBackend::Shutdown() {
  StopStreaming();
  std::lock_guard<std::mutex> lk(api_mutex_);

  if (data_stream_) RevokeBuffers();
  node_map_.reset();
  data_stream_.reset();
  device_.reset();
  opened_ = false;
  camera_index_ = -1;

  if (initialized_) {
    try {
      peak::Library::Close();
    } catch (...) {
    }
    initialized_ = false;
  }
}

std::vector<std::string> IdsSingleCameraBackend::ListDiscoveredCameras(
    std::string* err) const {
  std::lock_guard<std::mutex> lk(api_mutex_);
  std::vector<std::string> names;

  if (!initialized_) {
    if (err) *err = "IDS library not initialized";
    return names;
  }

  try {
    auto& dm = peak::DeviceManager::Instance();
    dm.Update();
    names.reserve(dm.Devices().size());
    for (size_t i = 0; i < dm.Devices().size(); ++i) {
      names.push_back("Camera " + std::to_string(i + 1));
    }
  } catch (const std::exception& e) {
    if (err) *err = e.what();
  }
  return names;
}

bool IdsSingleCameraBackend::OpenByIndex(int camera_index, std::string* err) {
  std::lock_guard<std::mutex> lk(api_mutex_);

  if (!initialized_) {
    if (err) *err = "IDS library not initialized";
    return false;
  }
  if (streaming_.load()) {
    if (err) *err = "Stop streaming before opening a new camera";
    return false;
  }

  try {
    auto& dm = peak::DeviceManager::Instance();
    dm.Update();

    if (camera_index < 0 ||
        camera_index >= static_cast<int>(dm.Devices().size())) {
      if (err) *err = "Invalid camera index";
      return false;
    }

    device_ = dm.Devices().at(static_cast<size_t>(camera_index))
                  ->OpenDevice(peak::core::DeviceAccessType::Control);
    node_map_ = device_->RemoteDevice()->NodeMaps().at(0);
    data_stream_ = device_->DataStreams().at(0)->OpenDataStream();

    camera_index_ = camera_index;
    opened_ = true;
    return true;
  } catch (const std::exception& e) {
    if (err) *err = e.what();
    device_.reset();
    node_map_.reset();
    data_stream_.reset();
    opened_ = false;
    camera_index_ = -1;
    return false;
  }
}

bool IdsSingleCameraBackend::ApplyConfig(const IdsCameraConfig& cfg,
                                         std::string* err) {
  std::lock_guard<std::mutex> lk(api_mutex_);

  if (!opened_ || !node_map_) {
    if (err) *err = "Camera not open";
    return false;
  }

  try {
    try {
      node_map_->FindNode<peak::core::nodes::EnumerationNode>("ExposureAuto")
          ->SetCurrentEntry("Off");
    } catch (...) {
    }
    try {
      node_map_->FindNode<peak::core::nodes::EnumerationNode>("GainAuto")
          ->SetCurrentEntry("Off");
    } catch (...) {
    }

    node_map_->FindNode<peak::core::nodes::IntegerNode>("OffsetX")->SetValue(0);
    node_map_->FindNode<peak::core::nodes::IntegerNode>("OffsetY")->SetValue(0);
    node_map_->FindNode<peak::core::nodes::IntegerNode>("Width")
        ->SetValue(cfg.roi_width);
    node_map_->FindNode<peak::core::nodes::IntegerNode>("Height")
        ->SetValue(cfg.roi_height);
    node_map_->FindNode<peak::core::nodes::IntegerNode>("OffsetX")
        ->SetValue(cfg.roi_offset_x);
    node_map_->FindNode<peak::core::nodes::IntegerNode>("OffsetY")
        ->SetValue(cfg.roi_offset_y);

    node_map_->FindNode<peak::core::nodes::FloatNode>("ExposureTime")
        ->SetValue(cfg.exposure_ms * 1000.0);
    node_map_->FindNode<peak::core::nodes::FloatNode>("Gain")
        ->SetValue(cfg.gain);

    bool is_mono8 = false;
    SetPixelFormat(node_map_.get(), cfg.pixel_format, &is_mono8);

    node_map_->FindNode<peak::core::nodes::EnumerationNode>("TriggerMode")
        ->SetCurrentEntry("Off");
    try {
      node_map_
          ->FindNode<peak::core::nodes::BooleanNode>("AcquisitionFrameRateEnable")
          ->SetValue(true);
    } catch (...) {
    }

    auto fps =
        node_map_->FindNode<peak::core::nodes::FloatNode>("AcquisitionFrameRate");
    fps->SetValue(cfg.fps);
    applied_fps_hz_ = fps->Value();

    try {
      auto ts_freq =
          node_map_->FindNode<peak::core::nodes::IntegerNode>("TimestampTickFrequency")
              ->Value();
      timestamp_tick_hz_ = ts_freq > 0 ? static_cast<double>(ts_freq) : 1e9;
    } catch (...) {
      timestamp_tick_hz_ = 1e9;
    }

    config_ = cfg;
    return true;
  } catch (const std::exception& e) {
    if (err) *err = e.what();
    return false;
  }
}

bool IdsSingleCameraBackend::PrepareBuffers(std::string* err) {
  if (!data_stream_ || !node_map_) {
    if (err) *err = "Camera stream not initialized";
    return false;
  }

  try {
    RevokeBuffers();
    auto payload =
        node_map_->FindNode<peak::core::nodes::IntegerNode>("PayloadSize")->Value();
    size_t min_req = data_stream_->NumBuffersAnnouncedMinRequired();
    size_t n_buffers = std::max<size_t>(32, min_req);

    for (size_t i = 0; i < n_buffers; ++i) {
      auto b = data_stream_->AllocAndAnnounceBuffer(static_cast<size_t>(payload),
                                                    nullptr);
      data_stream_->QueueBuffer(b);
    }
    return true;
  } catch (const std::exception& e) {
    if (err) *err = e.what();
    return false;
  }
}

void IdsSingleCameraBackend::RevokeBuffers() {
  if (!data_stream_) return;

  try {
    data_stream_->Flush(peak::core::DataStreamFlushMode::DiscardAll);
  } catch (...) {
  }

  for (const auto& b : data_stream_->AnnouncedBuffers()) {
    try {
      data_stream_->RevokeBuffer(b);
    } catch (...) {
    }
  }
}

bool IdsSingleCameraBackend::StartStreaming(std::string* err) {
  std::lock_guard<std::mutex> lk(api_mutex_);

  if (!opened_ || !node_map_ || !data_stream_) {
    if (err) *err = "Camera not open";
    return false;
  }
  if (streaming_.load()) return true;

  if (!PrepareBuffers(err)) return false;

  try {
    data_stream_->StartAcquisition();
    node_map_->FindNode<peak::core::nodes::CommandNode>("AcquisitionStart")
        ->Execute();
    streaming_.store(true);
    return true;
  } catch (const std::exception& e) {
    if (err) *err = e.what();
    RevokeBuffers();
    return false;
  }
}

void IdsSingleCameraBackend::StopStreaming() {
  std::lock_guard<std::mutex> lk(api_mutex_);

  if (!streaming_.load()) return;

  try {
    if (node_map_) {
      node_map_->FindNode<peak::core::nodes::CommandNode>("AcquisitionStop")
          ->Execute();
    }
  } catch (...) {
  }
  try {
    if (data_stream_) data_stream_->StopAcquisition();
  } catch (...) {
  }

  RevokeBuffers();
  streaming_.store(false);
}

bool IdsSingleCameraBackend::GrabFrame(int timeout_ms, IdsFrame* frame,
                                       std::string* err) {
  std::lock_guard<std::mutex> lk(api_mutex_);

  if (!streaming_.load() || !data_stream_) {
    if (err) *err = "Camera is not streaming";
    return false;
  }
  if (!frame) {
    if (err) *err = "Output frame pointer is null";
    return false;
  }

  try {
    auto finished = data_stream_->WaitForFinishedBuffer(timeout_ms);
    if (!finished) {
      if (err) *err = "Null buffer returned";
      return false;
    }

    IdsFrame out;
    out.width = static_cast<int>(finished->Width());
    out.height = static_cast<int>(finished->Height());
    out.timestamp_ticks = finished->Timestamp_ticks();
    out.bytes.resize(finished->Size());
    std::memcpy(out.bytes.data(), finished->BasePtr(), finished->Size());

    int pixels = out.width * out.height;
    out.bytes_per_pixel =
        pixels > 0 ? static_cast<int>(out.bytes.size() / static_cast<size_t>(pixels))
                   : 0;
    out.is_mono8 = out.bytes_per_pixel <= 1;

    data_stream_->QueueBuffer(finished);
    *frame = std::move(out);
    return true;
  } catch (const std::exception& e) {
    if (err) *err = e.what();
    return false;
  }
}

bool IdsSingleCameraBackend::is_initialized() const { return initialized_; }

bool IdsSingleCameraBackend::is_open() const { return opened_; }

bool IdsSingleCameraBackend::is_streaming() const { return streaming_.load(); }

int IdsSingleCameraBackend::camera_index() const { return camera_index_; }

double IdsSingleCameraBackend::applied_fps_hz() const { return applied_fps_hz_; }

double IdsSingleCameraBackend::timestamp_tick_hz() const {
  return timestamp_tick_hz_;
}

}  // namespace ids_backend
