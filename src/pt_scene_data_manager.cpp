#include "pt_scene_data_manager.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <unordered_set>

#include "pt_bounding_volume_hierarchy.h"
#include "pt_types.h"
#include "pt_utils.h"

godot::PTSceneDataManager::PTSceneDataManager()
    : _rd(nullptr),
      _tree(nullptr),
      _frame_materials({}),
      _frame_materials_list({}) {}

godot::PTSceneDataManager::~PTSceneDataManager() {}

void godot::PTSceneDataManager::initialize(
    RenderingDevice* p_rd, SceneTree* p_tree,
    PTResourceManager* resource_manager) {
    _rd = p_rd;
    _tree = p_tree;
    _resource_manager = resource_manager;
    // Create default texture
    _default_texture =
        ResourceLoader::get_singleton()->load("res://textures/white.png");
}

void godot::PTSceneDataManager::update_buffers(Ref<PTRendererStats> stats) {
    uint64_t start_t = Time::get_singleton()->get_ticks_msec();

    _stats = stats;

    _frame_materials.clear();
    _frame_materials_list.clear();
    _frame_textures.clear();

    // Default material at index 0
    PTMaterial default_material;
    _push_material(default_material);

    // Default texture at index 0
    _push_texture(_default_texture);

    TypedArray<PTAnalyticalGeometry> all_analytical =
        PTUtils::gather_nodes_of_type<PTAnalyticalGeometry>(_tree->get_root());

    TypedArray<PTAnalyticalGeometry> spheres;
    for (const Variant& analytical_variant : all_analytical) {
        PTAnalyticalGeometry* analytical =
            Object::cast_to<PTAnalyticalGeometry>(analytical_variant);

        if (!analytical->is_visible_in_tree()) {
            continue;
        }

        if (analytical->get_node_type() == NODE_TYPE_SPHERE) {
            spheres.append(analytical);
        }
    }

    _update_spheres_buffer(spheres);

    TypedArray<MeshInstance3D> mesh_instances =
        PTUtils::gather_nodes_of_type<MeshInstance3D>(_tree->get_root());
    _update_triangles_buffer(mesh_instances);

    _update_materials_buffer();

    _update_textures_buffer();
    _stats->set_texture_count(_frame_textures.size());

    uint64_t end_t = Time::get_singleton()->get_ticks_msec();
    print_line("Elapsed on update: " + String::num_int64(end_t - start_t));
}

void godot::PTSceneDataManager::_update_spheres_buffer(
    const TypedArray<PTAnalyticalGeometry>& spheres) {
    // 3 center + 1 radius + 1 material_index + 3 padding
    constexpr uint32_t SPHERE_FLOATS = 8;
    PackedFloat32Array spheres_data;

    // +4 for spheres count at start
    spheres_data.resize(spheres.size() * SPHERE_FLOATS + 4);
    spheres_data[0] = float(spheres.size());
    for (uint32_t i = 0; i < spheres.size(); ++i) {
        PTAnalyticalGeometry* sphere =
            Object::cast_to<PTAnalyticalGeometry>(spheres[i]);
        uint32_t base_offset = i * SPHERE_FLOATS + 4;
        uint32_t material_offset = 4;
        spheres_data[base_offset + 0] = sphere->get_global_position().x;
        spheres_data[base_offset + 1] = sphere->get_global_position().y;
        spheres_data[base_offset + 2] = sphere->get_global_position().z;
        spheres_data[base_offset + 3] =
            sphere->get_global_transform().basis[0].x * 0.5;  // radius
        uint32_t material_index = _parse_material(sphere->get_material());
        print_line("Sphere material index: " +
                   String::num_int64(material_index));
        spheres_data[base_offset + material_offset] = float(material_index);
    }

    PackedByteArray spheres_bytes = spheres_data.to_byte_array();
    _rd->buffer_update(_resource_manager->get_scene_spheres_storage_buffer(), 0,
                       spheres_bytes.size(), spheres_bytes);

    _stats->set_sphere_count(spheres.size());
}

void godot::PTSceneDataManager::_update_triangles_buffer(
    const TypedArray<MeshInstance3D>& meshes) {
    std::vector<PTTriangle> triangles;
    std::vector<PTVertex> vertices;

    PackedFloat32Array vertices_data;

    for (const Variant mesh_instance : meshes) {
        MeshInstance3D* mesh_instance_ptr =
            Object::cast_to<MeshInstance3D>(mesh_instance);

        if (mesh_instance_ptr == nullptr) {
            continue;
        }

        if (!mesh_instance_ptr->is_visible_in_tree()) {
            continue;
        }

        const Ref<Mesh> mesh = mesh_instance_ptr->get_mesh();

        // Ignore when null
        if (mesh.is_null()) {
            continue;
        }

        Transform3D mesh_transform = mesh_instance_ptr->get_global_transform();
        _load_mesh_surfaces(mesh, mesh_transform, vertices, vertices_data,
                            triangles);
    }

    // Write triangle count
    PackedFloat32Array triangles_count_data;
    triangles_count_data.push_back(float(triangles.size()));
    triangles_count_data.push_back(0.0);
    triangles_count_data.push_back(0.0);
    triangles_count_data.push_back(0.0);
    PackedByteArray triangle_count_bytes = triangles_count_data.to_byte_array();
    _rd->buffer_update(_resource_manager->get_scene_triangles_storage_buffer(),
                       0, triangle_count_bytes.size(), triangle_count_bytes);

    if (triangles.size() == 0) {
        print_line("No triangles to process.");
        return;
    }

    // Write vertices
    PackedByteArray vertices_bytes = vertices_data.to_byte_array();
    _rd->buffer_update(_resource_manager->get_scene_vertex_storage_buffer(), 0,
                       vertices_bytes.size(), vertices_bytes);

    _stats->set_triangle_count(triangles.size());
    _stats->set_vertex_count(vertices_data.size());

    // Build BVH
    uint64_t start_t = Time::get_singleton()->get_ticks_msec();
    PTBoundingVolumeHierarchy bvh;
    bvh.build(vertices, triangles);
    uint64_t end_t = Time::get_singleton()->get_ticks_msec();
    print_line("BVH creation time: " + String::num_int64(end_t - start_t));
    print_line("BVH with " + String::num_int64(bvh.get_nodes().size()) +
               " nodes");

    // std::vector<uint32_t> nodeIndices;
    // nodeIndices.push_back(0);

    // while (nodeIndices.size() > 0) {
    //     uint32_t nodeIndex = nodeIndices.at(0);
    //     nodeIndices.erase(nodeIndices.begin());
    //     const PTBoundingVolumeNode& node = bvh.get_nodes()[nodeIndex];

    //     if (!node.is_leaf) {
    //         nodeIndices.push_back(node.left_child_index);
    //         nodeIndices.push_back(node.right_child_index);
    //     }

    //     print_line(
    //         "Node " + String::num_int64(nodeIndex) + ". Min(" +
    //         String::num_real(node.aabb.min.x) + ", " +
    //         String::num_real(node.aabb.min.y) + ", " +
    //         String::num_real(node.aabb.min.z) + ") " + ", Max(" +
    //         String::num_real(node.aabb.max.x) + ", " +
    //         String::num_real(node.aabb.max.y) + ", " +
    //         String::num_real(node.aabb.max.z) + ") " + ", Left child index: "
    //         + String::num_int64(node.left_child_index) + ", Right child
    //         index: " + String::num_int64(node.right_child_index) + ",
    //         Primitive start: " +
    //         String::num_int64(node.primitive_start_index) +
    //         ", Primitive count: " + String::num_int64(node.primitive_count));
    // }

    PackedFloat32Array sorted_triangles;
    sorted_triangles.resize(triangles.size() * 4);

    for (uint32_t i = 0; i < triangles.size(); i++) {
        PTTriangle& triangle = triangles[i];
        sorted_triangles[i * 4 + 0] = triangle.i0;
        sorted_triangles[i * 4 + 1] = triangle.i1;
        sorted_triangles[i * 4 + 2] = triangle.i2;
        sorted_triangles[i * 4 + 3] = triangle.materialIndex;
    }

    PackedByteArray sorted_triangles_bytes = sorted_triangles.to_byte_array();
    _rd->buffer_update(_resource_manager->get_scene_triangles_storage_buffer(),
                       4 * 4, sorted_triangles_bytes.size(),
                       sorted_triangles_bytes);

    PackedFloat32Array bvh_nodes_data;
    constexpr uint32_t floats_per_node = 12;
    bvh_nodes_data.resize(bvh.get_nodes().size() * floats_per_node);
    for (uint32_t i = 0; i < bvh.get_nodes().size(); i++) {
        const PTBoundingVolumeNode& node = bvh.get_nodes()[i];
        uint32_t base = i * floats_per_node;

        Vector3 min = node.aabb.min;
        Vector3 max = node.aabb.max;

        // AABB
        bvh_nodes_data[base + 0] = min.x;
        bvh_nodes_data[base + 1] = min.y;
        bvh_nodes_data[base + 2] = min.z;
        bvh_nodes_data[base + 3] = 0.0;  // Padding

        bvh_nodes_data[base + 4] = max.x;
        bvh_nodes_data[base + 5] = max.y;
        bvh_nodes_data[base + 6] = max.z;
        bvh_nodes_data[base + 7] = 0.0;  // Padding

        bvh_nodes_data[base + 8] = node.primitive_start_index;
        bvh_nodes_data[base + 9] = node.primitive_count;
        bvh_nodes_data[base + 10] = node.left_child_index;
        bvh_nodes_data[base + 11] = node.right_child_index;
    }
    PackedByteArray bvh_nodes_byte_array = bvh_nodes_data.to_byte_array();
    _rd->buffer_update(_resource_manager->get_scene_bvh_storage_buffer(), 0,
                       bvh_nodes_byte_array.size(), bvh_nodes_byte_array);

    _stats->set_bvh_node_count(bvh.get_nodes().size());
}

void godot::PTSceneDataManager::_update_materials_buffer() {
    PackedFloat32Array materials_data;

    // vec3 color + float metallic + float roughness + float
    // refraction_index + float emission + uint32_t material_type + 3
    // padding
    constexpr uint32_t MATERIAL_FLOATS = 16;

    materials_data.resize(_frame_materials_list.size() * MATERIAL_FLOATS);
    for (uint32_t i = 0; i < _frame_materials_list.size(); ++i) {
        PTMaterial& material = _frame_materials_list[i];
        uint32_t base_offset = i * MATERIAL_FLOATS;
        materials_data[base_offset + 0] = float(material.material_type);
        materials_data[base_offset + 1] = material.metallic;
        materials_data[base_offset + 2] = material.roughness;
        materials_data[base_offset + 3] = material.refraction_index;
        materials_data[base_offset + 4] = material.color.r;
        materials_data[base_offset + 5] = material.color.g;
        materials_data[base_offset + 6] = material.color.b;
        materials_data[base_offset + 7] = material.emission;
        materials_data[base_offset + 8] = material.albedo_texture_index;
        materials_data[base_offset + 9] = material.metallic_texture_index;
        materials_data[base_offset + 10] = material.roughness_texture_index;
        materials_data[base_offset + 11] = material.metallic_texture_channel;
        materials_data[base_offset + 12] = material.roughness_texture_channel;
    }

    PackedByteArray materials_bytes = materials_data.to_byte_array();
    _rd->buffer_update(_resource_manager->get_scene_materials_storage_buffer(),
                       0, materials_bytes.size(), materials_bytes);

    _stats->set_material_count(_frame_materials_list.size());
}

void godot::PTSceneDataManager::_update_textures_buffer() {
    print_line("Updating textures buffer. Total textures: " +
               String::num_int64(_frame_textures.size()));

    // //combine textures into a single large texture
    for (size_t i = 0; i < _frame_textures.size(); i++) {
        auto image = _frame_textures[i]->get_image();
        image->clear_mipmaps();
        image->decompress();
        image->resize(_resource_manager->get_texture_array_resolution(),
                      _resource_manager->get_texture_array_resolution());
        if (_rd->texture_update(_resource_manager->get_scene_texture_array(), i,
                                image->get_data()) != Error::OK) {
            ERR_PRINT("Failed to update texture array layer index: " +
                      String::num_int64(i));
        }
    }
}

void godot::PTSceneDataManager::_load_mesh_surfaces(
    const Ref<Mesh> mesh, Transform3D& mesh_transform,
    std::vector<PTVertex>& vertices, PackedFloat32Array& vertices_data,
    std::vector<PTTriangle>& triangles) {
    // Iterates through all surfaces of the mesh
    for (uint32_t i = 0; i < mesh->get_surface_count(); ++i) {
        // Access surface material and parses
        Ref<Material> surface_material = mesh->surface_get_material(i);
        uint32_t mesh_material_index = _parse_material(surface_material);

        Array arr = mesh->surface_get_arrays(i);
        PackedVector3Array surface_vertices = arr[ArrayMesh::ARRAY_VERTEX];
        PackedInt32Array surface_indices = arr[ArrayMesh::ARRAY_INDEX];
        PackedColorArray surface_colors = arr[ArrayMesh::ARRAY_COLOR];
        PackedVector3Array surface_normals = arr[ArrayMesh::ARRAY_NORMAL];
        PackedVector2Array surface_uvs = arr[ArrayMesh::ARRAY_TEX_UV];
        print_line(
            "Color count: " + String::num_int64(surface_colors.size()),
            " Normal count: " + String::num_int64(surface_normals.size()));

        // Base offset relative to current vertices size
        uint32_t base_index_offset = uint32_t(vertices.size());

        // Add vertices
        for (uint32_t v = 0; v < surface_vertices.size(); ++v) {
            Vector3 position = surface_vertices[v];
            Color color = surface_colors.size() > 0 ? surface_colors[v]
                                                    : Color(1.0, 1.0, 1.0, 1.0);

            Vector3 normal = surface_normals.size() > 0
                                 ? surface_normals[v]
                                 : Vector3(0.0, 1.0, 0.0);

            position = mesh_transform.xform(position);

            // TODO: Handle translation/scale properly
            normal = mesh_transform.basis.xform(normal);

            Vector2 uv =
                surface_uvs.size() > 0 ? surface_uvs[v] : Vector2(0.0, 0.0);

            PTVertex vertex = {position, Vector3(color.r, color.g, color.b),
                               normal, uv};

            vertices_data.push_back(vertex.position.x);
            vertices_data.push_back(vertex.position.y);
            vertices_data.push_back(vertex.position.z);
            vertices_data.push_back(0.0);  // Padding

            vertices_data.push_back(vertex.color.x);
            vertices_data.push_back(vertex.color.y);
            vertices_data.push_back(vertex.color.z);
            vertices_data.push_back(0.0);  // Padding

            vertices_data.push_back(vertex.normal.x);
            vertices_data.push_back(vertex.normal.y);
            vertices_data.push_back(vertex.normal.z);
            vertices_data.push_back(0.0);  // Padding

            vertices_data.push_back(vertex.uv.x);
            vertices_data.push_back(vertex.uv.y);
            vertices_data.push_back(0.0);  // Padding
            vertices_data.push_back(0.0);  // Padding

            vertices.push_back(vertex);
        }

        // Add triangles
        for (uint32_t idx = 0; idx < surface_indices.size(); idx += 3) {
            uint32_t i0 =
                base_index_offset + uint32_t(surface_indices[idx + 0]);
            uint32_t i1 =
                base_index_offset + uint32_t(surface_indices[idx + 1]);
            uint32_t i2 =
                base_index_offset + uint32_t(surface_indices[idx + 2]);
            triangles.push_back({i0, i1, i2, mesh_material_index});
        }
    }
}

uint32_t godot::PTSceneDataManager::_push_material(const PTMaterial& material) {
    size_t hash = material.get_hash();
    if (!_frame_materials.count(hash)) {
        uint32_t index = uint32_t(_frame_materials_list.size());
        _frame_materials[hash] = index;
        _frame_materials_list.push_back(material);
        return index;
    } else {
        return uint32_t(_frame_materials[hash]);
    }
}

uint32_t godot::PTSceneDataManager::_parse_material(
    const Ref<Material>& material) {
    const Color BLACK_COLOR = Color{0.0, 0.0, 0.0, 1.0};
    const float DEFAULT_REFRACTION_SCALE = 0.05f;

    if (material.is_null()) {
        return 0;  // Default material index
    }

    if (material->is_class("StandardMaterial3D")) {
        Ref<StandardMaterial3D> std_material =
            Ref<StandardMaterial3D>(material);

        PTMaterial pt_material;
        pt_material.albedo_texture_index = _push_texture(
            std_material->get_texture(StandardMaterial3D::TEXTURE_ALBEDO));
        pt_material.metallic_texture_index = _push_texture(
            std_material->get_texture(StandardMaterial3D::TEXTURE_METALLIC));
        pt_material.roughness_texture_index = _push_texture(
            std_material->get_texture(StandardMaterial3D::TEXTURE_ROUGHNESS));

        pt_material.metallic_texture_channel =
            std_material->get_metallic_texture_channel();
        pt_material.roughness_texture_channel =
            std_material->get_roughness_texture_channel();

        // Determine material type
        MaterialType materialType = MATERIAL_TYPE_LAMBERTIAN;

        // Emissive when emission color and intensity is set
        if (std_material->get_emission_energy_multiplier() > 0.0 &&
            std_material->get_emission() != BLACK_COLOR) {
            materialType = MATERIAL_TYPE_EMISSIVE;
        }
        // Dielectric only when a non default refraction is set
        else if (std_material->get_refraction() != DEFAULT_REFRACTION_SCALE) {
            // Transforms refraction index to refraction scale
            float refractionIndex =
                1.0 / (1.0 - std_material->get_refraction());
            materialType = MATERIAL_TYPE_DIELECTRIC;
            print_line("Found dielectric material. Refraction index: " +
                       String::num_real(std_material->get_refraction()));

            pt_material.refraction_index = refractionIndex;
        }
        pt_material.material_type = materialType;

        Color albedo_color = std_material->get_albedo();
        pt_material.color = albedo_color;
        pt_material.metallic = std_material->get_metallic();
        pt_material.roughness = std_material->get_roughness();
        pt_material.emission = std_material->get_emission_energy_multiplier();

        return _push_material(pt_material);
    }

    else {
        ERR_PRINT("Unimplemented material parser for type: " +
                  material->get_class());
    }

    return 0;
}

uint32_t godot::PTSceneDataManager::_push_texture(
    const Ref<Texture2D>& texture) {
    if (texture.is_null()) {
        return 0;  // Default texture index
    }

    for (uint32_t i = 0; i < _frame_textures.size(); ++i) {
        if (_frame_textures[i] == texture) {
            return i;
        }
    }

    if (_frame_textures.size() >=
        _resource_manager->get_texture_array_layers()) {
        ERR_PRINT(
            "Exceeded maximum number of scene textures: " +
            String::num_int64(_resource_manager->get_texture_array_layers()));
        return 0;
    }

    _frame_textures.push_back(texture);
    return uint32_t(_frame_textures.size() - 1);
}
