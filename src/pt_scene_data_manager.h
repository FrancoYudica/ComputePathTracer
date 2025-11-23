#ifndef SCENE_DATA_MANAGER_H
#define SCENE_DATA_MANAGER_H

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <unordered_map>
#include <vector>

#include "pt_material.h"

namespace godot {

    /**
     * PTSceneDataManager handles the GPU buffers for scene
     * data such as spheres, triangles, vertices, and
     * materials.
     */

    class PTSceneDataManager : public RefCounted {
        GDCLASS(PTSceneDataManager, RefCounted)
    private:
        RenderingDevice* _rd;
        SceneTree* _tree;
        RID _spheres_storage_buffer;
        RID _triangles_storage_buffer;
        RID _vertices_storage_buffer;
        RID _materials_storage_buffer;
        RID _bvh_storage_buffer;

        std::unordered_map<size_t, uint32_t> _frame_materials;  // Maps material
                                                                // hash to index
        std::vector<Ref<PTMaterial>> _frame_materials_list;     // Stores frames
                                                                // materials

        struct FrameStats {
            uint32_t sphere_count = 0;
            uint32_t triangle_count = 0;
            uint32_t vertex_count = 0;
            uint32_t material_count = 0;
        } _stats;

    protected:
        static void _bind_methods();

    public:
        PTSceneDataManager();
        ~PTSceneDataManager();

        void initialize(RenderingDevice* p_rd, SceneTree* p_tree,
                        RID spheres_storage_buffer,
                        RID triangles_storage_buffer,
                        RID vertices_storage_buffer,
                        RID materials_storage_buffer, RID bvh_storage_buffer);

        void update_buffers();

        uint32_t get_sphere_count() const { return _stats.sphere_count; }
        uint32_t get_triangle_count() const { return _stats.triangle_count; }
        uint32_t get_vertex_count() const { return _stats.vertex_count; }
        uint32_t get_material_count() const { return _stats.material_count; }

    private:
        void _update_spheres_buffer(
            const TypedArray<Node>& all_nodes,
            const std::vector<uint32_t>& sphere_indices);

        void _update_triangles_buffer(
            const TypedArray<Node>& all_nodes,
            const std::vector<uint32_t>& triangle_mesh_indices);

        void _update_materials_buffer();

        uint32_t _push_material(const Ref<PTMaterial>& material);
    };
}  // namespace godot

#endif