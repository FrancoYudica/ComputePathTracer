#include "pt_geometry_processor.h"
#include <godot_cpp/classes/surface_tool.hpp>
#include <godot_cpp/classes/mesh_data_tool.hpp>
#include <godot_cpp/classes/skin.hpp>

namespace godot {

    PTGeometryResult PTGeometryProcessor::process_scene_geometry(
        const TypedArray<MeshInstance3D>& meshes,
        PTMaterialLibrary* material_lib) {
        this->_material_lib = material_lib;

        PTGeometryResult result;

        for (int i = 0; i < meshes.size(); i++) {
            MeshInstance3D* instance =
                Object::cast_to<MeshInstance3D>(meshes[i]);

            // Skips loading non visible
            if (!instance->is_visible_in_tree()) {
                continue;
            }

            if (instance->get_mesh().is_valid()) {
                _process_single_instance(instance, result);
            }
        }

        return result;
    }

    void PTGeometryProcessor::_process_single_instance(
        MeshInstance3D* instance, PTGeometryResult& result) {
        // Prepare the Mesh
        Ref<ArrayMesh> processed_mesh = _get_processed_array_mesh(instance);
        if (processed_mesh.is_null()) return;

        // Bake Deformations (Skeletal & Blend Shapes)
        _apply_deformations(instance, processed_mesh);

        // Make sure that the mesh has tangents
        processed_mesh = _ensure_tangents(processed_mesh);

        // Loads the materials. Gets it's indices
        std::vector<uint32_t> material_indices;
        _load_mesh_materials(instance, material_indices);

        _pack_mesh_data(instance, processed_mesh, material_indices, result);
    }

    Ref<ArrayMesh> PTGeometryProcessor::_get_processed_array_mesh(
        MeshInstance3D* instance) {
        Ref<Mesh> base_mesh = instance->get_mesh();
        Ref<ArrayMesh> am = base_mesh;

        if (am.is_null()) {
            Ref<SurfaceTool> st;
            st.instantiate();
            st->create_from(base_mesh, 0);
            return st->commit();
        }
        return base_mesh->duplicate();
    }

    void PTGeometryProcessor::_apply_deformations(MeshInstance3D* instance,
                                                  Ref<ArrayMesh>& mesh) {
        if (instance->get_blend_shape_count() > 0) {
            instance->bake_mesh_from_current_blend_shape_mix(mesh);
        }
        if (!instance->get_skeleton_path().is_empty() &&
            instance->get_skin().is_valid()) {
            instance->bake_mesh_from_current_skeleton_pose(mesh);
        }
    }

    Ref<ArrayMesh> PTGeometryProcessor::_ensure_tangents(Ref<ArrayMesh> mesh) {
        Ref<ArrayMesh> final_mesh;
        final_mesh.instantiate();

        for (int i = 0; i < mesh->get_surface_count(); i++) {
            Ref<SurfaceTool> st;
            st.instantiate();
            st->begin(Mesh::PRIMITIVE_TRIANGLES);
            st->append_from(mesh, i, Transform3D());
            st->generate_tangents();
            final_mesh = st->commit(final_mesh);
        }
        return final_mesh;
    }

    void PTGeometryProcessor::_load_mesh_materials(
        MeshInstance3D* instance, std::vector<uint32_t>& indices) {
        // Loads the materials of the surfaces
        Ref<Mesh> mesh = instance->get_mesh();

        for (uint32_t i = 0; i < mesh->get_surface_count(); ++i) {
            Ref<Material> surface_material;

            // If there is a material override for the surface, use that instead
            if (instance->get_surface_override_material(i).is_valid()) {
                surface_material = instance->get_surface_override_material(i);

            }
            // Otherwise, just use the default surface material
            else {
                surface_material = mesh->surface_get_material(i);
            }

            indices.push_back(
                _material_lib->get_or_push_material(surface_material));
        }
    }

    void PTGeometryProcessor::_pack_mesh_data(
        MeshInstance3D* instance, Ref<ArrayMesh> mesh,
        const std::vector<uint32_t>& material_indices,
        PTGeometryResult& result) {
        // Loads per surface vertices
        Transform3D mesh_transform = instance->get_global_transform();

        for (uint32_t i = 0; i < mesh->get_surface_count(); ++i) {
            Array arr = mesh->surface_get_arrays(i);

            // Use MeshDataTool to access tangents
            Ref<MeshDataTool> mdt;
            mdt.instantiate();
            mdt->create_from_surface(mesh, i);

            // Access surface material and parses
            uint32_t mesh_material_index = material_indices[i];

            PackedVector3Array surface_vertices = arr[ArrayMesh::ARRAY_VERTEX];
            PackedInt32Array surface_indices = arr[ArrayMesh::ARRAY_INDEX];
            PackedColorArray surface_colors = arr[ArrayMesh::ARRAY_COLOR];
            PackedVector3Array surface_normals = arr[ArrayMesh::ARRAY_NORMAL];
            PackedVector2Array surface_uvs = arr[ArrayMesh::ARRAY_TEX_UV];

            // Base offset relative to current vertices size
            uint32_t base_index_offset = uint32_t(result.vertices.size());

            // Add vertices
            for (uint32_t v = 0; v < surface_vertices.size(); ++v) {
                Vector3 local_position = surface_vertices[v];

                Vector2 uv =
                    surface_uvs.size() > 0 ? surface_uvs[v] : Vector2(0.0, 0.0);

                Vector3 local_normal = surface_normals.size() > 0
                                           ? surface_normals[v]
                                           : Vector3(0.0, 1.0, 0.0);

                Vector4 local_tangent =
                    Vector4(mdt->get_vertex_tangent(v).normal.x,
                            mdt->get_vertex_tangent(v).normal.y,
                            mdt->get_vertex_tangent(v).normal.z,
                            mdt->get_vertex_tangent(v).d);

                Color color = surface_colors.size() > 0
                                  ? surface_colors[v]
                                  : Color(1.0, 1.0, 1.0, 1.0);

                // Transform Position
                Vector3 position = mesh_transform.xform(local_position);

                // Transform Normal (Inverse Transpose + Normalize)
                Basis inv_transpose = mesh_transform.basis.inverse();
                inv_transpose.transpose();
                Vector3 normal = inv_transpose.xform(local_normal).normalized();

                // Transform Tangent (Basis + Normalize)
                Vector3 tangent_raw =
                    Vector3(local_tangent.x, local_tangent.y, local_tangent.z);
                Vector3 tangent =
                    mesh_transform.basis.xform(tangent_raw).normalized();

                // Preserve the handedness for the bitangent calculation
                float tangent_w = local_tangent.w;

                PTVertex vertex = {
                    position, Vector3(color.r, color.g, color.b), normal, uv,
                    Vector4(tangent.x, tangent.y, tangent.z, tangent_w)};

                result.vertices.push_back(vertex);
            }

            // Add triangles
            for (uint32_t idx = 0; idx < surface_indices.size(); idx += 3) {
                uint32_t i0 =
                    base_index_offset + uint32_t(surface_indices[idx + 0]);
                uint32_t i1 =
                    base_index_offset + uint32_t(surface_indices[idx + 1]);
                uint32_t i2 =
                    base_index_offset + uint32_t(surface_indices[idx + 2]);

                result.triangles.push_back({i0, i1, i2, mesh_material_index});
            }
        }
    }

}  // namespace godot