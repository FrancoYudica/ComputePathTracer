#include "pt_types.h"
#include <functional>
namespace godot {
    static void hash_combine(size_t& seed, size_t value) {
        seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    size_t PTMaterial::get_hash() const {
        size_t hash = 0;
        hash_combine(hash, std::hash<int>()(static_cast<int>(material_type)));
        hash_combine(hash, std::hash<float>()(color.r));
        hash_combine(hash, std::hash<float>()(color.g));
        hash_combine(hash, std::hash<float>()(color.b));
        hash_combine(hash, std::hash<float>()(metallic));
        hash_combine(hash, std::hash<float>()(roughness));
        hash_combine(hash, std::hash<float>()(refraction_index));
        hash_combine(hash, std::hash<float>()(emission));
        hash_combine(hash, std::hash<uint32_t>()(albedo_texture_index));
        return hash;
    }
}  // namespace godot