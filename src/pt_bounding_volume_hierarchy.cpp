#include "pt_bounding_volume_hierarchy.h"

#include <algorithm>
#define MAX_DEPTH 32

namespace godot {

    void PTBoundingVolumeHierarchy::_bind_methods() {}

    PTBoundingVolumeHierarchy::PTBoundingVolumeHierarchy() {}

    PTBoundingVolumeHierarchy::~PTBoundingVolumeHierarchy() {}

    void PTBoundingVolumeHierarchy::build(const std::vector<PTVertex>& vertices,
                                          std::vector<PTTriangle>& triangles) {
        // Pre-allocate space to avoid reallocations during build
        // A binary tree with N leaves has at most 2N-1 nodes
        size_t estimated_nodes = triangles.size() / 10 * 2;  // Rough estimate
        nodes.reserve(estimated_nodes);

        nodes.push_back(PTBoundingVolumeNode{});
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

    // Choose the best axis and sort in one pass
    static uint32_t choose_split_axis_and_sort(
        const std::vector<PTVertex>& vertices,
        std::vector<PTTriangle>& triangles, uint32_t begin, uint32_t end) {
        // Compute bounding box of triangle centroids
        Vector3 min_centroid(1e30f, 1e30f, 1e30f);
        Vector3 max_centroid(-1e30f, -1e30f, -1e30f);

        for (uint32_t i = begin; i < end; i++) {
            Vector3 centroid = compute_centroid(vertices, triangles[i]);

            min_centroid.x = std::min(min_centroid.x, centroid.x);
            min_centroid.y = std::min(min_centroid.y, centroid.y);
            min_centroid.z = std::min(min_centroid.z, centroid.z);

            max_centroid.x = std::max(max_centroid.x, centroid.x);
            max_centroid.y = std::max(max_centroid.y, centroid.y);
            max_centroid.z = std::max(max_centroid.z, centroid.z);
        }

        // Calculate extent along each axis
        Vector3 extent = max_centroid - min_centroid;

        // Determine axis with largest extent
        uint32_t split_axis;
        if (extent.x > extent.y && extent.x > extent.z) {
            split_axis = 0;
        } else if (extent.y > extent.z) {
            split_axis = 1;
        } else {
            split_axis = 2;
        }

        // Sort by the chosen axis (single sort instead of separate function
        // call)
        std::sort(triangles.begin() + begin, triangles.begin() + end,
                  [&](const PTTriangle& a, const PTTriangle& b) {
                      Vector3 centroid_a = compute_centroid(vertices, a);
                      Vector3 centroid_b = compute_centroid(vertices, b);
                      return centroid_a[split_axis] < centroid_b[split_axis];
                  });

        return split_axis;
    }

    void PTBoundingVolumeHierarchy::split(uint32_t node_index,
                                          const std::vector<PTVertex>& vertices,
                                          std::vector<PTTriangle>& triangles,
                                          uint32_t begin, uint32_t end,
                                          uint32_t depth) {
        uint32_t count = end - begin;

        // Base case, no splitting. Marks node as leaf
        if (count <= 12 || depth >= MAX_DEPTH) {
            // Make leaf node
            nodes[node_index].is_leaf = true;
            nodes[node_index].left_child_index = 0;
            nodes[node_index].right_child_index = 0;
            nodes[node_index].primitive_start_index = begin;
            nodes[node_index].primitive_count = count;
            nodes[node_index].aabb =
                compute_aabb(vertices, triangles, begin, end);
            return;
        }

        // Choose axis and sort in one operation
        choose_split_axis_and_sort(vertices, triangles, begin, end);

        // Split in the middle
        uint32_t split_index = begin + count / 2;

        // Allocate child nodes together
        uint32_t left_node_index = nodes.size();
        nodes.push_back(PTBoundingVolumeNode{});
        nodes.push_back(PTBoundingVolumeNode{});
        uint32_t right_node_index = left_node_index + 1;

        // Recursively split children
        split(left_node_index, vertices, triangles, begin, split_index,
              depth + 1);
        split(right_node_index, vertices, triangles, split_index, end,
              depth + 1);

        // Set up current node as internal node
        // Access node data after recursive calls to avoid reference
        // invalidation
        nodes[node_index].left_child_index = left_node_index;
        nodes[node_index].right_child_index = right_node_index;
        nodes[node_index].primitive_count = 0;
        nodes[node_index].primitive_start_index = 0;
        nodes[node_index].aabb =
            nodes[left_node_index].aabb.merge(nodes[right_node_index].aabb);
        nodes[node_index].is_leaf = false;
    }

}  // namespace godot