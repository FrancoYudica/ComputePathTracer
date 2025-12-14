#include "pt_bounding_volume_hierarchy.h"

#include <algorithm>

struct SAHSplit {
    uint32_t axis;
    uint32_t index;  // Index in the triangle array where the split occurs
    float cost;
};

namespace godot {

    void PTBoundingVolumeHierarchy::_bind_methods() {}

    PTBoundingVolumeHierarchy::PTBoundingVolumeHierarchy() {}

    PTBoundingVolumeHierarchy::~PTBoundingVolumeHierarchy() {}

    void PTBoundingVolumeHierarchy::build(const std::vector<PTVertex>& vertices,
                                          std::vector<PTTriangle>& triangles,
                                          const BVHSettings& settings) {
        _settings = settings;
        // Pre-allocate space to avoid reallocations during build
        // A binary tree with N leaves has at most 2N-1 _nodes
        size_t estimated_nodes = triangles.size() / 10 * 2;  // Rough estimate
        _nodes.reserve(estimated_nodes);

        _nodes.push_back(PTBoundingVolumeNode{});
        split(0, vertices, triangles, 0, uint32_t(triangles.size()), 0);
    }

    static PTAABB compute_aabb(const std::vector<PTVertex>& vertices,
                               const std::vector<PTTriangle>& triangles,
                               uint32_t begin, uint32_t end) {
        PTAABB aabb;

        // Initialize with first vertex instead of inside loop
        const PTTriangle& first_tri = triangles[begin];
        aabb.min = vertices[first_tri.i0].position;
        aabb.max = vertices[first_tri.i0].position;
        aabb.expand_to(vertices[first_tri.i1].position);
        aabb.expand_to(vertices[first_tri.i2].position);

        // Process remaining triangles
        for (uint32_t i = begin + 1; i < end; i++) {
            const PTTriangle& triangle = triangles[i];
            aabb.expand_to(vertices[triangle.i0].position);
            aabb.expand_to(vertices[triangle.i1].position);
            aabb.expand_to(vertices[triangle.i2].position);
        }

        return aabb;
    }

    // Inline helper to compute centroid
    static inline Vector3 compute_centroid(
        const std::vector<PTVertex>& vertices, const PTTriangle& tri) {
        return (vertices[tri.i0].position + vertices[tri.i1].position +
                vertices[tri.i2].position) /
               3.0f;
    }

    static inline float surface_area(PTAABB& aabb) {
        Vector3 d = aabb.max - aabb.min;
        return 2.0f * (d.x * d.y + d.y * d.z + d.z * d.x);
    }

    static SAHSplit find_best_sah_split(const std::vector<PTVertex>& vertices,
                                        std::vector<PTTriangle>& triangles,
                                        uint32_t begin, uint32_t end,
                                        PTAABB& parent_aabb,
                                        uint32_t num_bins) {
        uint32_t count = end - begin;

        // Cost constants
        const float TRAVERSAL_COST = 0.125f;
        const float INTERSECTION_COST = 1.2f;

        SAHSplit best_split;
        best_split.cost = 1e30f;  // Large initial cost
        best_split.axis = 0;
        best_split.index = begin + count / 2;

        float parent_sa = surface_area(parent_aabb);

        // Try each axis
        for (uint32_t axis = 0; axis < 3; axis++) {
            // Sort triangles by centroid along that axis
            std::sort(triangles.begin() + begin, triangles.begin() + end,
                      [&](const PTTriangle& a, const PTTriangle& b) {
                          Vector3 centroid_a = compute_centroid(vertices, a);
                          Vector3 centroid_b = compute_centroid(vertices, b);
                          return centroid_a[axis] < centroid_b[axis];
                      });

            // Number of bins for SAH evaluation
            const uint32_t bins = num_bins > count ? count : num_bins;

            // Split at regular intervals
            for (uint32_t bin = 1; bin < bins; bin++) {
                uint32_t split_index = begin + (count * bin) / bins;

                // Compute AABBs for left and right subsets
                PTAABB left_aabb =
                    compute_aabb(vertices, triangles, begin, split_index);
                PTAABB right_aabb =
                    compute_aabb(vertices, triangles, split_index, end);

                // Compute SAH cost
                uint32_t left_count = split_index - begin;
                uint32_t right_count = end - split_index;

                float left_sa = surface_area(left_aabb);
                float right_sa = surface_area(right_aabb);

                float sah_cost =
                    TRAVERSAL_COST +
                    (left_sa / parent_sa) * left_count * INTERSECTION_COST +
                    (right_sa / parent_sa) * right_count * INTERSECTION_COST;

                // Update best split if this is better
                if (sah_cost < best_split.cost) {
                    best_split.cost = sah_cost;
                    best_split.axis = axis;
                    best_split.index = split_index;
                }
            }
        }

        if (best_split.index != 2) {
            // Re-sort triangles by best axis
            std::sort(triangles.begin() + begin, triangles.begin() + end,
                      [&](const PTTriangle& a, const PTTriangle& b) {
                          Vector3 centroid_a = compute_centroid(vertices, a);
                          Vector3 centroid_b = compute_centroid(vertices, b);
                          return centroid_a[best_split.axis] <
                                 centroid_b[best_split.axis];
                      });
        }

        return best_split;
    }

    void PTBoundingVolumeHierarchy::split(uint32_t node_index,
                                          const std::vector<PTVertex>& vertices,
                                          std::vector<PTTriangle>& triangles,
                                          uint32_t begin, uint32_t end,
                                          uint32_t depth) {
        uint32_t count = end - begin;

        PTAABB node_aabb = compute_aabb(vertices, triangles, begin, end);
        _nodes[node_index].aabb = node_aabb;

        // Base case, no splitting. Marks node as leaf
        if (count <= _settings.max_triangles_per_leaf ||
            depth >= _settings.max_depth) {
            _nodes[node_index].is_leaf = true;
            _nodes[node_index].left_child_index = 0;
            _nodes[node_index].right_child_index = 0;
            _nodes[node_index].primitive_start_index = begin;
            _nodes[node_index].primitive_count = count;
            return;
        }

        // Find best split using SAH
        SAHSplit sah_split = find_best_sah_split(
            vertices, triangles, begin, end, node_aabb, _settings.sah_bins);

        const float INTERSECTION_COST = 1.2f;
        float no_split_cost = count * INTERSECTION_COST;

        if (sah_split.cost >= no_split_cost) {
            // No split is better, make leaf
            _nodes[node_index].is_leaf = true;
            _nodes[node_index].left_child_index = 0;
            _nodes[node_index].right_child_index = 0;
            _nodes[node_index].primitive_start_index = begin;
            _nodes[node_index].primitive_count = count;
            return;
        }

        // Split in the middle
        uint32_t split_index = sah_split.index;

        // Allocate child _nodes together
        uint32_t left_node_index = _nodes.size();
        _nodes.push_back(PTBoundingVolumeNode{});
        _nodes.push_back(PTBoundingVolumeNode{});
        uint32_t right_node_index = left_node_index + 1;

        // Recursively split children
        split(left_node_index, vertices, triangles, begin, split_index,
              depth + 1);
        split(right_node_index, vertices, triangles, split_index, end,
              depth + 1);

        // Set up current node as internal node
        // Access node data after recursive calls to avoid reference
        // invalidation
        _nodes[node_index].left_child_index = left_node_index;
        _nodes[node_index].right_child_index = right_node_index;
        _nodes[node_index].primitive_count = 0;
        _nodes[node_index].primitive_start_index = 0;
    }

}  // namespace godot