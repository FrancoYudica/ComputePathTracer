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
        Vector2 uv;
        Vector4 tangent;
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

    enum MaterialType {
        MATERIAL_TYPE_LAMBERTIAN = 0,
        MATERIAL_TYPE_METAL = 1,
        MATERIAL_TYPE_DIELECTRIC = 2,
        MATERIAL_TYPE_EMISSIVE = 3,
    };

    struct PTMaterial {
        MaterialType material_type = MaterialType::MATERIAL_TYPE_LAMBERTIAN;
        Color color = Color(1.0, 1.0, 1.0, 1.0);
        float metallic = 0.0;
        float roughness = 1.0;
        float refraction_index = 0.0;
        Vector3 emission = Vector3(0.0, 0.0, 0.0);
        uint32_t albedo_texture_index = 0;
        uint32_t metallic_texture_index = 0;
        uint32_t roughness_texture_index = 0;
        uint32_t emission_texture_index = 0;
        uint32_t normal_texture_index = 0;
        uint32_t metallic_texture_channel = 0;
        uint32_t roughness_texture_channel = 0;
        float emission_energy_multiplier = 1.0;

        size_t get_hash() const;
    };

}  // namespace godot

#endif