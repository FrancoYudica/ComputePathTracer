#pragma once
#include <vector>
#include <unordered_map>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include "pt_types.h"
#include "pt_material_library.h"

namespace godot {

    struct PTGeometryResult {
        std::vector<PTVertex> vertices;
        std::vector<PTTriangle> triangles;
    };

    class PTGeometryProcessor {
    private:
        PTMaterialLibrary* _material_lib;

    public:
        PTGeometryResult process_scene_geometry(
            const TypedArray<MeshInstance3D>& meshes,
            PTMaterialLibrary* material_lib);

    private:
        void _process_single_instance(MeshInstance3D* instance,
                                      PTGeometryResult& result);

        Ref<ArrayMesh> _get_processed_array_mesh(MeshInstance3D* instance);
        void _apply_deformations(MeshInstance3D* instance,
                                 Ref<ArrayMesh>& mesh);

        Ref<ArrayMesh> _ensure_tangents(Ref<ArrayMesh> mesh);

        void _load_mesh_materials(MeshInstance3D* instance,
                                  std::vector<uint32_t>& indices);

        void _pack_mesh_data(MeshInstance3D* instance, Ref<ArrayMesh> mesh,
                             const std::vector<uint32_t>& material_indices,
                             PTGeometryResult& result);
    };

}  // namespace godot