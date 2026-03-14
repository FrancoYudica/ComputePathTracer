#include "pt_material_library.h"
#include "pt_utils.h"
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

namespace godot {

    void PTMaterialLibrary::initialize(PTResourceManager* resource_manager) {
        _rm = resource_manager;

        _default_texture = ResourceLoader::get_singleton()->load(
            PTUtils::get_project_relative_path("textures/white.png"));
    }

    void PTMaterialLibrary::clear() {
        _frame_materials.clear();
        _frame_materials_list.clear();
        _frame_textures.clear();

        // Pushes default material and texture
        PTMaterial default_material;
        get_or_push_material(default_material);
        get_or_push_texture(_default_texture);
    }

    uint32_t PTMaterialLibrary::get_or_push_material(
        const Ref<Material>& material) {
        return _parse_material(material);
    }

    uint32_t PTMaterialLibrary::get_or_push_material(
        const PTMaterial& material) {
        size_t hash = material.get_hash();
        if (!_frame_materials.count(hash)) {
            uint32_t index = uint32_t(_frame_materials_list.size());
            _frame_materials[hash] = index;
            _frame_materials_list.push_back(material);
        }
        return _frame_materials[hash];
    }

    const std::vector<PTMaterial>& PTMaterialLibrary::get_materials() {
        return _frame_materials_list;
    }

    const std::vector<Ref<Texture2D>>& PTMaterialLibrary::get_textures() {
        return _frame_textures;
    }

    PackedByteArray PTMaterialLibrary::get_byte_array() const {
        PackedFloat32Array materials_data;

        constexpr uint32_t MATERIAL_FLOATS = 20;

        materials_data.resize(_frame_materials_list.size() * MATERIAL_FLOATS);
        for (uint32_t i = 0; i < _frame_materials_list.size(); ++i) {
            const PTMaterial& material = _frame_materials_list[i];
            uint32_t base_offset = i * MATERIAL_FLOATS;
            materials_data[base_offset + 0] = material.color.r;
            materials_data[base_offset + 1] = material.color.g;
            materials_data[base_offset + 2] = material.color.b;
            materials_data[base_offset + 3] = float(material.material_type);
            materials_data[base_offset + 4] = material.emission.x;
            materials_data[base_offset + 5] = material.emission.y;
            materials_data[base_offset + 6] = material.emission.z;
            materials_data[base_offset + 7] = material.metallic;
            materials_data[base_offset + 8] = material.roughness;
            materials_data[base_offset + 9] = material.refraction_index;
            materials_data[base_offset + 10] = material.albedo_texture_index;
            materials_data[base_offset + 11] = material.metallic_texture_index;
            materials_data[base_offset + 12] = material.roughness_texture_index;
            materials_data[base_offset + 13] = material.emission_texture_index;
            materials_data[base_offset + 14] = material.normal_texture_index;
            materials_data[base_offset + 15] =
                material.metallic_texture_channel;
            materials_data[base_offset + 16] =
                material.roughness_texture_channel;
            materials_data[base_offset + 17] =
                material.emission_energy_multiplier;
        }

        return materials_data.to_byte_array();
    }

    uint32_t PTMaterialLibrary::_parse_material(const Ref<Material>& material) {
        const Color BLACK_COLOR = Color{0.0, 0.0, 0.0, 1.0};
        const float DEFAULT_REFRACTION_SCALE = 0.05f;

        if (material.is_null()) {
            return 0;  // Default material index
        }

        if (material->is_class("StandardMaterial3D")) {
            Ref<StandardMaterial3D> std_material =
                Ref<StandardMaterial3D>(material);

            PTMaterial pt_material;
            pt_material.albedo_texture_index = get_or_push_texture(
                std_material->get_texture(StandardMaterial3D::TEXTURE_ALBEDO));
            pt_material.metallic_texture_index =
                get_or_push_texture(std_material->get_texture(
                    StandardMaterial3D::TEXTURE_METALLIC));
            pt_material.roughness_texture_index =
                get_or_push_texture(std_material->get_texture(
                    StandardMaterial3D::TEXTURE_ROUGHNESS));
            pt_material.emission_texture_index =
                get_or_push_texture(std_material->get_texture(
                    StandardMaterial3D::TEXTURE_EMISSION));
            pt_material.normal_texture_index = get_or_push_texture(
                std_material->get_texture(StandardMaterial3D::TEXTURE_NORMAL));

            pt_material.metallic_texture_channel =
                std_material->get_metallic_texture_channel();
            pt_material.roughness_texture_channel =
                std_material->get_roughness_texture_channel();

            pt_material.emission_energy_multiplier =
                std_material->get_emission_energy_multiplier();

            Color emission = std_material->get_emission();
            pt_material.emission = Vector3(emission.r, emission.g, emission.b);
            // Determine material type
            MaterialType materialType = MATERIAL_TYPE_LAMBERTIAN;

            // Emissive when emission color and intensity is set
            if (std_material->get_emission_energy_multiplier() > 0.0 &&
                std_material->get_emission() != BLACK_COLOR) {
                materialType = MATERIAL_TYPE_EMISSIVE;
            }
            // Dielectric only when a non default refraction is set
            else if (std_material->get_refraction() !=
                     DEFAULT_REFRACTION_SCALE) {
                // Transforms refraction index to refraction scale
                float refractionIndex =
                    1.0 / (1.0 - std_material->get_refraction());
                materialType = MATERIAL_TYPE_DIELECTRIC;

                pt_material.refraction_index = refractionIndex;
            }
            pt_material.material_type = materialType;

            Color albedo_color = std_material->get_albedo();
            pt_material.color = albedo_color;
            pt_material.metallic = std_material->get_metallic();
            pt_material.roughness = std_material->get_roughness();

            return get_or_push_material(pt_material);
        }

        else {
            ERR_PRINT("Unimplemented material parser for type: " +
                      material->get_class());
        }
        return 0;
    }

    uint32_t godot::PTMaterialLibrary::get_or_push_texture(
        const Ref<Texture2D>& texture) {
        if (texture.is_null()) {
            return 0;  // Default texture index
        }

        for (uint32_t i = 0; i < _frame_textures.size(); ++i) {
            if (_frame_textures[i] == texture) {
                return i;
            }
        }

        if (_frame_textures.size() >= _rm->get_texture_array_layers()) {
            ERR_PRINT(
                "PTMaterialLibrary: Exceeded maximum number of scene "
                "textures: " +
                String::num_int64(_rm->get_texture_array_layers()));
            return 0;
        }

        _frame_textures.push_back(texture);
        return uint32_t(_frame_textures.size() - 1);
    }

}  // namespace godot