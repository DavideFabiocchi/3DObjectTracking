// SPDX-License-Identifier: MIT

#ifndef M3T_INCLUDE_M3T_IDS_CAMERA_H_
#define M3T_INCLUDE_M3T_IDS_CAMERA_H_

#include <filesystem/filesystem.h>
#include <m3t/camera.h>

#include <memory>
#include <string>

namespace ids_backend {
class IdsSingleCameraBackend;
struct IdsCameraConfig;
}  // namespace ids_backend

namespace m3t {

/**
 * \brief Color camera implementation based on IDS peak SDK.
 *
 * \details This class is intentionally RGB-only and designed to be used as a
 * drop-in replacement for other `ColorCamera` implementations in M3T.
 */
class IdsColorCamera : public ColorCamera {
 public:
  IdsColorCamera(const std::string &name, int camera_index = 0);
  IdsColorCamera(const std::string &name,
                 const std::filesystem::path &metafile_path);
  ~IdsColorCamera();

  bool SetUp() override;
  bool UpdateImage(bool synchronized) override;

  void set_camera_index(int camera_index);
  void set_roi_offset_x(int roi_offset_x);
  void set_roi_offset_y(int roi_offset_y);
  void set_roi_width(int roi_width);
  void set_roi_height(int roi_height);
  void set_fps(double fps);
  void set_gain(double gain);
  void set_exposure_ms(double exposure_ms);
  void set_pixel_format(const std::string &pixel_format);
  void set_apply_camera_settings(bool apply_camera_settings);
  void set_swap_rb_channels(bool swap_rb_channels);

  int camera_index() const;
  int roi_offset_x() const;
  int roi_offset_y() const;
  int roi_width() const;
  int roi_height() const;
  double fps() const;
  double gain() const;
  double exposure_ms() const;
  const std::string &pixel_format() const;
  bool apply_camera_settings() const;
  bool swap_rb_channels() const;
  double applied_fps_hz() const;

 private:
  bool LoadMetaData();

  int camera_index_ = 0;
  int roi_offset_x_ = 0;
  int roi_offset_y_ = 0;
  int roi_width_ = 1984;
  int roi_height_ = 1264;
  double fps_ = 60.0;
  double gain_ = 1.0;
  double exposure_ms_ = 3.0;
  std::string pixel_format_ = "BGR8";
  bool apply_camera_settings_ = true;
  bool swap_rb_channels_ = false;

  std::unique_ptr<ids_backend::IdsSingleCameraBackend> backend_ptr_;
};

}  // namespace m3t

#endif  // M3T_INCLUDE_M3T_IDS_CAMERA_H_
