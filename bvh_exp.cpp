#include <iostream>
#include <span>
#include <vector>

#include "contrib/bvh/test/load_obj.h"
#include <bvh/v2/bvh.h>
#include <bvh/v2/default_builder.h>
#include <bvh/v2/executor.h>
#include <bvh/v2/node.h>
#include <bvh/v2/thread_pool.h>
#include <bvh/v2/tri.h>
#include <bvh/v2/vec.h>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>

#ifndef RENDER_OBJECT
#define RENDER_OBJECT 0
#endif

#include <bvh/v2/stack.h>
using Scalar = float;
using Vec3 = bvh::v2::Vec<Scalar, 3>;
using Tri = bvh::v2::Tri<Scalar, 3>;
using Node = bvh::v2::Node<Scalar, 3>;
using Bvh = bvh::v2::Bvh<Node>;

int main(int argc, char **argv) {
  // Build a scene. If a command-line argument is provided, treat it as a path
  // to an OBJ file and load triangles from it. Otherwise, fall back to
  // compile-time selection via RENDER_OBJECT.
  std::vector<Tri> tris;

  std::string path = "./dataset/scenes/bunny.obj";
  if (argc >= 2) {
    path = argv[1];
  }
  auto loaded = load_obj<float>(path);
  if (loaded.empty()) {
    std::cerr << "Failed to load OBJ or OBJ has no triangles: " << path << "\n";
    return 1;
  }
  tris = std::move(loaded);
  std::cout << "Loaded OBJ with triangles: " << tris.size() << " from " << path
            << "\n";

  // Prepare bboxes and centers
  bvh::v2::ThreadPool thread_pool;
  bvh::v2::ParallelExecutor executor(thread_pool);

  std::vector<bvh::v2::BBox<Scalar, 3>> bboxes(tris.size());
  std::vector<Vec3> centers(tris.size());
  executor.for_each(0, tris.size(), [&](size_t begin, size_t end) {
    for (size_t i = begin; i < end; ++i) {
      bboxes[i] = tris[i].get_bbox();
      centers[i] = tris[i].get_center();
    }
  });

  // Build BVH using the parallel MiniTreeBuilder via DefaultBuilder
  using Builder = bvh::v2::DefaultBuilder<Node>;
  typename Builder::Config config;
  config.quality = Builder::Quality::High;
  config.parallel_threshold =
      1; // force parallel mini-tree builder for small scenes

  auto bvh = Builder::build(thread_pool, std::span(bboxes), std::span(centers),
                            config);

  std::cout << "Built BVH with primitives: " << tris.size() << "\n";
  std::cout << "BVH nodes: " << bvh.nodes.size() << "\n";

  // Prepare precomputed triangles for faster intersection tests.
  using PrecomputedTri = bvh::v2::PrecomputedTri<Scalar>;
  static constexpr bool should_permute = true;

  std::vector<PrecomputedTri> precomputed_tris(tris.size());
  executor.for_each(0, tris.size(), [&](size_t begin, size_t end) {
    for (size_t i = begin; i < end; ++i) {
      auto j = should_permute ? bvh.prim_ids[i] : i;
      precomputed_tris[i] = tris[j];
    }
  });

  // Read rays from dataset. Default file is `dataset/rays`, or pass path as
  // second command-line argument.
  std::string rays_path = "dataset/rays";
  std::ifstream rays_in(rays_path);
  if (!rays_in) {
    std::cerr << "Warning: could not open rays file: " << rays_path
              << "; no rays to trace.\n";
    return 0;
  }

  using Ray = bvh::v2::Ray<Scalar, 3>;
  std::vector<Ray> rays;
  std::string line;
  while (std::getline(rays_in, line)) {
    // trim leading whitespace
    auto first_non_ws = line.find_first_not_of(" \t\r\n");
    if (first_non_ws == std::string::npos)
      continue;
    if (line[first_non_ws] == '#')
      continue;
    std::istringstream ss(line);
    Scalar ox, oy, oz, dx, dy, dz, tmin, tmax;
    if (!(ss >> ox >> oy >> oz >> dx >> dy >> dz >> tmin >> tmax))
      continue; // skip malformed lines
    rays.emplace_back(Vec3(ox, oy, oz), Vec3(dx, dy, dz), tmin, tmax);
  }

  std::cout << "Loaded rays: " << rays.size() << " from " << rays_path << "\n";

  // Intersect each ray with the BVH and report simple stats.
  static constexpr size_t invalid_id = std::numeric_limits<size_t>::max();
  static constexpr size_t stack_size = 64;
  static constexpr bool use_robust_traversal = false;

  size_t hit_count = 0;
  double total_distance = 0.0;

  for (auto &ray : rays) {
    size_t prim_id = invalid_id;
    float u = 0.0f, v = 0.0f;
    bvh::v2::SmallStack<Bvh::Index, stack_size> stack;
    bvh.intersect<false, use_robust_traversal>(
        ray, bvh.get_root().index, stack, [&](size_t begin, size_t end) {
          for (size_t i = begin; i < end; ++i) {
            size_t j = should_permute ? i : bvh.prim_ids[i];
            if (auto hit = precomputed_tris[j].intersect(ray)) {
              prim_id = i;
              std::tie(ray.tmax, u, v) = *hit;
            }
          }
          return prim_id != invalid_id;
        });

    if (prim_id != invalid_id) {
      ++hit_count;
      total_distance += static_cast<double>(ray.tmax);
    }
  }

  std::cout << "Rays traced: " << rays.size() << "  hits: " << hit_count;
  if (hit_count)
    std::cout << "  avg distance: " << (total_distance / hit_count);
  std::cout << "\n";

  return 0;
}
