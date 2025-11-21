#ifndef PT_VBH_H
#define PT_VBH_H

#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <vector>
#include <godot_cpp/classes/ref_counted.hpp>

namespace godot {

    struct PTTriangle {
        uint32_t i0;
        uint32_t i1;
        uint32_t i2;
        uint32_t materialIndex;
    };

    struct PTVertex {
        Vector3 position;
    };

    struct PTAABB {
        Vector3 min;
        Vector3 max;

        PTAABB merge(PTAABB& other) {
            return { min.min(other.min), max.max(other.max) };
        }

        void expand_to(Vector3 point) {
            min = min.min(point);
            max = max.max(point);
        }

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
        std::vector<PTBoundingVolumeNode> nodes;

    protected:
        static void _bind_methods();

    public:
        PTBoundingVolumeHierarchy();
        ~PTBoundingVolumeHierarchy();

        const std::vector<PTBoundingVolumeNode> &get_nodes() const { return nodes; }
        
        void build(const std::vector<PTVertex> &vertices, std::vector<PTTriangle>& triangles);
        void split(
            uint32_t nodeIndex, 
            const std::vector<PTVertex> &vertices, 
            std::vector<PTTriangle>& triangles, 
            uint32_t begin,
            uint32_t end,
            uint32_t depth);

    };
}

#endif