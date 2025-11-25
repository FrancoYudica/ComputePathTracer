#ifndef SCENE_DATA_MANAGER_H
#define SCENE_DATA_MANAGER_H

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <unordered_map>
#include <vector>
#include <stack>

#include "pt_analytical_geometry.h"
#include "pt_types.h"

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
        std::vector<PTMaterial> _frame_materials_list;          // Stores frames
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
            const TypedArray<PTAnalyticalGeometry>& spheres);
        void _update_triangles_buffer(const TypedArray<MeshInstance3D>& meshes);

        void _update_materials_buffer();
        void _load_mesh_surfaces(const Ref<Mesh> mesh,
                                 Transform3D& mesh_transform,
                                 std::vector<PTVertex>& vertices,
                                 PackedFloat32Array& vertices_data,
                                 std::vector<PTTriangle>& triangles);

        uint32_t _push_material(const PTMaterial& material);
        uint32_t _parse_material(const Ref<Material>& material);
    };

}  // namespace godot

#endif