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
#include <m3t/texture_modality.h>
#include <m3t/tracker.h>

#include <memory>
#include <string>
#include <vector>

int main(int argc, char *argv[]) {
  if (argc < 4) {
    std::cerr << "Usage: " << argv[0]
              << " <ids_camera_yaml> <objects_directory> <body_name_1> [body_name_2 ...]"
              << std::endl;
    return -1;
  }

  const std::filesystem::path ids_camera_yaml{argv[1]};
  const std::filesystem::path objects_directory{argv[2]};

  std::vector<std::string> body_names;
  for (int i = 3; i < argc; ++i) body_names.push_back(std::string{argv[i]});

  constexpr bool kUseTextureModality = false;
  constexpr bool kVisualizePoseResult = false;

  auto tracker_ptr{std::make_shared<m3t::Tracker>("tracker")};
  auto renderer_geometry_ptr{
      std::make_shared<m3t::RendererGeometry>("renderer_geometry")};

  auto color_camera_ptr{
      std::make_shared<m3t::IdsColorCamera>("ids_color", ids_camera_yaml)};

  auto color_viewer_ptr{std::make_shared<m3t::NormalColorViewer>(
      "color_viewer", color_camera_ptr, renderer_geometry_ptr)};
  tracker_ptr->AddViewer(color_viewer_ptr);

  auto color_silhouette_renderer_ptr{
      std::make_shared<m3t::FocusedSilhouetteRenderer>(
          "color_silhouette_renderer", renderer_geometry_ptr,
          color_camera_ptr)};

  for (const auto &body_name : body_names) {
    // Body
    const std::filesystem::path body_yaml = objects_directory / (body_name + ".yaml");
    auto body_ptr{std::make_shared<m3t::Body>(body_name, body_yaml)};
    renderer_geometry_ptr->AddBody(body_ptr);
    color_silhouette_renderer_ptr->AddReferencedBody(body_ptr);

    // Models
    auto region_model_ptr{std::make_shared<m3t::RegionModel>(
        body_name + "_region_model", body_ptr,
        objects_directory / (body_name + "_region_model.bin"))};

    // Modalities (RGB only)
    auto region_modality_ptr{std::make_shared<m3t::RegionModality>(
        body_name + "_region_modality", body_ptr, color_camera_ptr,
        region_model_ptr)};
    if (kVisualizePoseResult) region_modality_ptr->set_visualize_pose_result(true);

    std::shared_ptr<m3t::TextureModality> texture_modality_ptr;
    if (kUseTextureModality) {
      texture_modality_ptr = std::make_shared<m3t::TextureModality>(
          body_name + "_texture_modality", body_ptr, color_camera_ptr,
          color_silhouette_renderer_ptr);
    }

    // Link + optimizer
    auto link_ptr{std::make_shared<m3t::Link>(body_name + "_link", body_ptr)};
    link_ptr->AddModality(region_modality_ptr);
    if (texture_modality_ptr) link_ptr->AddModality(texture_modality_ptr);

    auto optimizer_ptr{
        std::make_shared<m3t::Optimizer>(body_name + "_optimizer", link_ptr)};
    tracker_ptr->AddOptimizer(optimizer_ptr);

    // Detector
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
