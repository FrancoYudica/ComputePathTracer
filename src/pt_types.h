#ifndef PT_TYPES_H
#define PT_TYPES_H

#include <godot_cpp/classes/node.hpp>

namespace godot {
    struct PTTriangle {
        uint32_t i0;
        uint32_t i1;
        uint32_t i2;
        uint32_t materialIndex;
    };

    struct PTVertex {
        Vector3 position;
        Vector3 color;
        Vector3 normal;
    };

    struct PTAABB {
        Vector3 min;
        Vector3 max;

        PTAABB merge(PTAABB& other) {
            return {min.min(other.min), max.max(other.max)};
        }

        void expand_to(Vector3 point) {
            min = min.min(point);
            max = max.max(point);
        }
    };

}  // namespace godot

#endif