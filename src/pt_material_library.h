#pragma once
#include <vector>
#include <unordered_map>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include "pt_types.h"
#include "pt_resource_manager.h"

namespace godot {
    class PTMaterialLibrary {
    private:
        std::unordered_map<size_t, uint32_t> _frame_materials;  // Maps material
        std::vector<PTMaterial> _frame_materials_list;          // Stores frames
        std::vector<Ref<Texture2D>> _frame_textures;
        Ref<Texture2D> _default_texture;
        PTResourceManager* _rm;

    public:
        void initialize(PTResourceManager* resource_manager);

        void clear();

        /**
         * Loads a new material into the library. Also checks
         * if the material is already loaded, to avoid duplicates
         */
        uint32_t get_or_push_material(const Ref<Material>& material);
        uint32_t get_or_push_material(const PTMaterial& material);
        uint32_t get_or_push_texture(const Ref<Texture2D>& texture);
        const std::vector<PTMaterial>& get_materials();
        const std::vector<Ref<Texture2D>>& get_textures();

    private:
        /**
         * Parses the material from a Godot Material to PTMaterial.
         * Returns it's index
         */
        uint32_t _parse_material(const Ref<Material>& material);
    };
}  // namespace godot