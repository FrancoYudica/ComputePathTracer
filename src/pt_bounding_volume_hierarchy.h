#ifndef PT_VBH_H
#define PT_VBH_H

#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <vector>
#include "pt_types.h"

namespace godot {

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
        std::vector<PTBoundingVolumeNode> nodes;

    protected:
        static void _bind_methods();

    public:
        PTBoundingVolumeHierarchy();
        ~PTBoundingVolumeHierarchy();

        const std::vector<PTBoundingVolumeNode>& get_nodes() const {
            return nodes;
        }

        void build(const std::vector<PTVertex>& vertices,
                   std::vector<PTTriangle>& triangles);
        void split(uint32_t node_index, const std::vector<PTVertex>& vertices,
                   std::vector<PTTriangle>& triangles, uint32_t begin,
                   uint32_t end, uint32_t depth);
    };
}  // namespace godot

#endif