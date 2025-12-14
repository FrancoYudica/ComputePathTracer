#ifndef PT_VBH_H
#define PT_VBH_H

#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <vector>
#include "pt_types.h"

namespace godot {

    struct BVHSettings {
        uint32_t max_depth = 32;
        uint32_t max_triangles_per_leaf = 4;
        uint32_t sah_bins = 32;
    };

    struct PTBoundingVolumeNode {
        PTAABB aabb;
        bool is_leaf = false;
        uint32_t left_child_index = 0;
        uint32_t right_child_index = 0;
        uint32_t primitive_start_index = 0;
        uint32_t primitive_count = 0;
        PTBoundingVolumeNode() {};
    };

    class PTBoundingVolumeHierarchy {
    private:
        std::vector<PTBoundingVolumeNode> _nodes;
        BVHSettings _settings;

    protected:
        static void _bind_methods();

    public:
        PTBoundingVolumeHierarchy();
        ~PTBoundingVolumeHierarchy();

        const std::vector<PTBoundingVolumeNode>& get_nodes() const {
            return _nodes;
        }

        void build(const std::vector<PTVertex>& vertices,
                   std::vector<PTTriangle>& triangles,
                   const BVHSettings& settings);
        void split(uint32_t node_index, const std::vector<PTVertex>& vertices,
                   std::vector<PTTriangle>& triangles, uint32_t begin,
                   uint32_t end, uint32_t depth);
    };
}  // namespace godot

#endif