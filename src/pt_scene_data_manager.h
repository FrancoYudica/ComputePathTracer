#ifndef SCENE_DATA_MANAGER_H
#define SCENE_DATA_MANAGER_H

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <unordered_map>
#include <vector>
#include <stack>

#include "pt_analytical_geometry.h"
#include "pt_types.h"
#include "pt_resource_manager.h"
#include "pt_renderer_stats.h"

namespace godot {

    /**
     * PTSceneDataManager handles the GPU buffers for scene
     * data such as spheres, triangles, vertices, and
     * materials.
     */
    class PTSceneDataManager {
    private:
        RenderingDevice* _rd;
        SceneTree* _tree;
        PTResourceManager* _resource_manager;

        std::unordered_map<size_t, uint32_t> _frame_materials;  // Maps material
                                                                // hash to index
        std::vector<PTMaterial> _frame_materials_list;          // Stores frames
                                                                // materials

        std::vector<Ref<Texture2D>> _frame_textures;

        Ref<Texture2D> _default_texture;

        RID _spheres_storage_buffer;
        RID _triangles_storage_buffer;
        RID _vertices_storage_buffer;
        RID _materials_storage_buffer;
        RID _textures_storage_buffer;
        RID _bvh_storage_buffer;

        Ref<PTRendererStats> _stats;

    public:
        PTSceneDataManager();
        ~PTSceneDataManager();

        void initialize(RenderingDevice* p_rd, SceneTree* p_tree,
                        PTResourceManager* resource_manager);

        void update_buffers(Ref<PTRendererStats> stats);

    private:
        void _update_spheres_buffer(
            const TypedArray<PTAnalyticalGeometry>& spheres);
        void _update_triangles_buffer(const TypedArray<MeshInstance3D>& meshes);

        void _update_materials_buffer();
        void _update_textures_buffer();
        void _load_mesh_surfaces(const Ref<Mesh> mesh,
                                 Transform3D& mesh_transform,
                                 std::vector<PTVertex>& vertices,
                                 PackedFloat32Array& vertices_data,
                                 std::vector<PTTriangle>& triangles);

        uint32_t _push_material(const PTMaterial& material);
        uint32_t _parse_material(const Ref<Material>& material);
        uint32_t _push_texture(const Ref<Texture2D>& texture);
    };

}  // namespace godot

#endif