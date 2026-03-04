// SPDX-License-Identifier: MIT

#include <m3t/ids_camera.h>

#include <ids_single_camera_backend.h>
#include <m3t/common.h>

#include <iostream>

namespace m3t {

IdsColorCamera::IdsColorCamera(const std::string &name, int camera_index)
    : ColorCamera{name}, camera_index_{camera_index} {
  backend_ptr_ = std::make_unique<ids_backend::IdsSingleCameraBackend>();
}

IdsColorCamera::IdsColorCamera(const std::string &name,
                               const std::filesystem::path &metafile_path)
    : ColorCamera{name, metafile_path} {
  backend_ptr_ = std::make_unique<ids_backend::IdsSingleCameraBackend>();
}

IdsColorCamera::~IdsColorCamera() {
  if (backend_ptr_) backend_ptr_->Shutdown();
}

bool IdsColorCamera::SetUp() {
  set_up_ = false;

  if (!metafile_path_.empty()) {
    if (!LoadMetaData()) return false;
  }

  if (intrinsics_.width <= 0 || intrinsics_.height <= 0) {
    std::cerr << "[IdsColorCamera] Invalid intrinsics for camera " << name_
              << ". Provide intrinsics in metafile." << std::endl;
    return false;
  }

  std::string err;
  if (!backend_ptr_->Initialize(&err)) {
    std::cerr << "[IdsColorCamera] Initialize failed: " << err << std::endl;
    return false;
  }

  if (!backend_ptr_->OpenByIndex(camera_index_, &err)) {
    std::cerr << "[IdsColorCamera] OpenByIndex(" << camera_index_
              << ") failed: " << err << std::endl;
    return false;
  }

  ids_backend::IdsCameraConfig cfg;
  cfg.roi_offset_x = roi_offset_x_;
  cfg.roi_offset_y = roi_offset_y_;
  cfg.roi_width = roi_width_;
  cfg.roi_height = roi_height_;
  cfg.fps = fps_;
  cfg.gain = gain_;
  cfg.exposure_ms = exposure_ms_;
  cfg.pixel_format = pixel_format_;

  if (!backend_ptr_->ApplyConfig(cfg, &err)) {
    std::cerr << "[IdsColorCamera] ApplyConfig failed: " << err << std::endl;
    return false;
  }

  if (!backend_ptr_->StartStreaming(&err)) {
    std::cerr << "[IdsColorCamera] StartStreaming failed: " << err
              << std::endl;
    return false;
  }

  SaveMetaDataIfDesired();
  set_up_ = true;
  return UpdateImage(true);
}

bool IdsColorCamera::UpdateImage(bool synchronized) {
  if (!set_up_) {
    std::cerr << "Set up IDS color camera " << name_ << " first" << std::endl;
    return false;
  }

  ids_backend::IdsFrame frame;
  std::string err;
  int timeout_ms = synchronized ? -1 : 0;
  if (!backend_ptr_->GrabFrame(timeout_ms, &frame, &err)) {
    std::cerr << "[IdsColorCamera] GrabFrame failed: " << err << std::endl;
    return false;
  }

  if (frame.width <= 0 || frame.height <= 0 || frame.bytes.empty()) {
    std::cerr << "[IdsColorCamera] Invalid frame received" << std::endl;
    return false;
  }

  if (frame.bytes_per_pixel <= 1) {
    cv::Mat one_channel(frame.height, frame.width, CV_8UC1,
                        const_cast<uint8_t *>(frame.bytes.data()));

    if (frame.pixel_format_name == "BayerRG8") {
      cv::cvtColor(one_channel, image_, cv::COLOR_BayerRG2BGR);
    } else if (frame.pixel_format_name == "BayerBG8") {
      cv::cvtColor(one_channel, image_, cv::COLOR_BayerBG2BGR);
    } else if (frame.pixel_format_name == "BayerGR8") {
      cv::cvtColor(one_channel, image_, cv::COLOR_BayerGR2BGR);
    } else if (frame.pixel_format_name == "BayerGB8") {
      cv::cvtColor(one_channel, image_, cv::COLOR_BayerGB2BGR);
    } else {
      cv::cvtColor(one_channel, image_, cv::COLOR_GRAY2BGR);
    }
  } else if (frame.bytes_per_pixel == 3) {
    cv::Mat three_channel(frame.height, frame.width, CV_8UC3,
                          const_cast<uint8_t *>(frame.bytes.data()));
    if (frame.pixel_format_name == "RGB8")
      cv::cvtColor(three_channel, image_, cv::COLOR_RGB2BGR);
    else
      image_ = three_channel.clone();
  } else if (frame.bytes_per_pixel == 4) {
    cv::Mat four_channel(frame.height, frame.width, CV_8UC4,
                         const_cast<uint8_t *>(frame.bytes.data()));
    if (frame.pixel_format_name == "RGBA8")
      cv::cvtColor(four_channel, image_, cv::COLOR_RGBA2BGR);
    else
      cv::cvtColor(four_channel, image_, cv::COLOR_BGRA2BGR);
  } else {
    std::cerr << "[IdsColorCamera] Unsupported bytes_per_pixel: "
              << frame.bytes_per_pixel << std::endl;
    return false;
  }

  SaveImageIfDesired();
  return true;
}

void IdsColorCamera::set_camera_index(int camera_index) {
  camera_index_ = camera_index;
  set_up_ = false;
}

void IdsColorCamera::set_roi_offset_x(int roi_offset_x) {
  roi_offset_x_ = roi_offset_x;
  set_up_ = false;
}

void IdsColorCamera::set_roi_offset_y(int roi_offset_y) {
  roi_offset_y_ = roi_offset_y;
  set_up_ = false;
}

void IdsColorCamera::set_roi_width(int roi_width) {
  roi_width_ = roi_width;
  set_up_ = false;
}

void IdsColorCamera::set_roi_height(int roi_height) {
  roi_height_ = roi_height;
  set_up_ = false;
}

void IdsColorCamera::set_fps(double fps) {
  fps_ = fps;
  set_up_ = false;
}

void IdsColorCamera::set_gain(double gain) {
  gain_ = gain;
  set_up_ = false;
}

void IdsColorCamera::set_exposure_ms(double exposure_ms) {
  exposure_ms_ = exposure_ms;
  set_up_ = false;
}

void IdsColorCamera::set_pixel_format(const std::string &pixel_format) {
  pixel_format_ = pixel_format;
  set_up_ = false;
}

int IdsColorCamera::camera_index() const { return camera_index_; }
int IdsColorCamera::roi_offset_x() const { return roi_offset_x_; }
int IdsColorCamera::roi_offset_y() const { return roi_offset_y_; }
int IdsColorCamera::roi_width() const { return roi_width_; }
int IdsColorCamera::roi_height() const { return roi_height_; }
double IdsColorCamera::fps() const { return fps_; }
double IdsColorCamera::gain() const { return gain_; }
double IdsColorCamera::exposure_ms() const { return exposure_ms_; }
const std::string &IdsColorCamera::pixel_format() const { return pixel_format_; }

bool IdsColorCamera::LoadMetaData() {
  cv::FileStorage fs;
  if (!OpenYamlFileStorage(metafile_path_, &fs)) return false;

  if (!ReadRequiredValueFromYaml(fs, "intrinsics", &intrinsics_)) {
    std::cerr << "Could not read required intrinsics from " << metafile_path_
              << std::endl;
    return false;
  }

  ReadOptionalValueFromYaml(fs, "camera2world_pose", &camera2world_pose_);
  ReadOptionalValueFromYaml(fs, "save_directory", &save_directory_);
  ReadOptionalValueFromYaml(fs, "save_index", &save_index_);
  ReadOptionalValueFromYaml(fs, "save_image_type", &save_image_type_);
  ReadOptionalValueFromYaml(fs, "save_images", &save_images_);

  ReadOptionalValueFromYaml(fs, "camera_index", &camera_index_);
  ReadOptionalValueFromYaml(fs, "roi_offset_x", &roi_offset_x_);
  ReadOptionalValueFromYaml(fs, "roi_offset_y", &roi_offset_y_);
  ReadOptionalValueFromYaml(fs, "roi_width", &roi_width_);
  ReadOptionalValueFromYaml(fs, "roi_height", &roi_height_);
  ReadOptionalValueFromYaml(fs, "fps", &fps_);
  ReadOptionalValueFromYaml(fs, "gain", &gain_);
  ReadOptionalValueFromYaml(fs, "exposure_ms", &exposure_ms_);
  ReadOptionalValueFromYaml(fs, "pixel_format", &pixel_format_);

  fs.release();

  if (save_directory_.is_relative())
    save_directory_ = metafile_path_.parent_path() / save_directory_;
  world2camera_pose_ = camera2world_pose_.inverse();
  return true;
}

double IdsColorCamera::applied_fps_hz() const {
  return backend_ptr_ ? backend_ptr_->applied_fps_hz() : 0.0;
}

}  // namespace m3t
