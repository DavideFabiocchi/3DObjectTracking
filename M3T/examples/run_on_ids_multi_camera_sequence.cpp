// SPDX-License-Identifier: MIT

#include <filesystem/filesystem.h>
#include <m3t/body.h>
#include <m3t/ids_camera.h>
#include <m3t/link.h>
#include <m3t/normal_viewer.h>
#include <m3t/region_modality.h>
#include <m3t/region_model.h>
#include <m3t/renderer_geometry.h>
#include <m3t/static_detector.h>
#include <m3t/tracker.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

int main(int argc, char *argv[]) {
  if (argc < 6) {
    std::cerr << "Usage: " << argv[0]
              << " <objects_directory> --cameras <cam_yaml_1> [cam_yaml_2 ...]"
                 " --bodies <body_name_1> [body_name_2 ...]"
              << std::endl;
    return -1;
  }

  const std::filesystem::path objects_directory{argv[1]};
  std::vector<std::filesystem::path> camera_yaml_paths;
  std::vector<std::string> body_names;

  enum class ParseMode { None, Cameras, Bodies };
  ParseMode mode = ParseMode::None;

  for (int i = 2; i < argc; ++i) {
    const std::string token{argv[i]};
    if (token == "--cameras") {
      mode = ParseMode::Cameras;
      continue;
    }
    if (token == "--bodies") {
      mode = ParseMode::Bodies;
      continue;
    }

    if (mode == ParseMode::Cameras) {
      camera_yaml_paths.emplace_back(token);
    } else if (mode == ParseMode::Bodies) {
      body_names.emplace_back(token);
    } else {
      std::cerr << "Unexpected token: " << token
                << ". Expected --cameras or --bodies" << std::endl;
      return -1;
    }
  }

  if (camera_yaml_paths.empty() || body_names.empty()) {
    std::cerr << "Both camera list and body list must be non-empty" << std::endl;
    return -1;
  }

  auto tracker_ptr{std::make_shared<m3t::Tracker>("tracker")};
  auto renderer_geometry_ptr{
      std::make_shared<m3t::RendererGeometry>("renderer_geometry")};

  std::vector<std::shared_ptr<m3t::IdsColorCamera>> color_camera_ptrs;
  color_camera_ptrs.reserve(camera_yaml_paths.size());

  for (size_t i = 0; i < camera_yaml_paths.size(); ++i) {
    auto camera_ptr = std::make_shared<m3t::IdsColorCamera>(
        "ids_color_" + std::to_string(i), camera_yaml_paths[i]);
    color_camera_ptrs.push_back(camera_ptr);
  }

  // Add one viewer per camera for easier debugging.
  for (size_t i = 0; i < color_camera_ptrs.size(); ++i) {
    auto viewer_ptr{std::make_shared<m3t::NormalColorViewer>(
        "color_viewer_" + std::to_string(i), color_camera_ptrs[i],
        renderer_geometry_ptr)};
    tracker_ptr->AddViewer(viewer_ptr);
  }

  for (const auto &body_name : body_names) {
    const std::filesystem::path body_yaml = objects_directory / (body_name + ".yaml");
    auto body_ptr{std::make_shared<m3t::Body>(body_name, body_yaml)};
    renderer_geometry_ptr->AddBody(body_ptr);

    auto region_model_ptr{std::make_shared<m3t::RegionModel>(
        body_name + "_region_model", body_ptr,
        objects_directory / (body_name + "_region_model.bin"))};

    auto link_ptr{std::make_shared<m3t::Link>(body_name + "_link", body_ptr)};

    for (size_t cam_idx = 0; cam_idx < color_camera_ptrs.size(); ++cam_idx) {
      auto region_modality_ptr{std::make_shared<m3t::RegionModality>(
          body_name + "_region_modality_cam_" + std::to_string(cam_idx), body_ptr,
          color_camera_ptrs[cam_idx], region_model_ptr)};
      link_ptr->AddModality(region_modality_ptr);
    }

    auto optimizer_ptr{
        std::make_shared<m3t::Optimizer>(body_name + "_optimizer", link_ptr)};
    tracker_ptr->AddOptimizer(optimizer_ptr);

    const std::filesystem::path detector_yaml =
        objects_directory / (body_name + "_detector.yaml");
    auto detector_ptr{std::make_shared<m3t::StaticDetector>(
        body_name + "_detector", detector_yaml, optimizer_ptr)};
    tracker_ptr->AddDetector(detector_ptr);
  }

  if (!tracker_ptr->SetUp()) return -1;
  if (!tracker_ptr->RunTrackerProcess(true, false)) return -1;
  return 0;
}
