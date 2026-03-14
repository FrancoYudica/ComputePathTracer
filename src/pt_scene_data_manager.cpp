#include "pt_scene_data_manager.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/image.hpp>
#include <unordered_set>

#include "pt_bounding_volume_hierarchy.h"
#include "pt_types.h"
#include "pt_utils.h"

godot::PTSceneDataManager::PTSceneDataManager()
    : _rd(nullptr), _root(nullptr) {}

godot::PTSceneDataManager::~PTSceneDataManager() {}

void godot::PTSceneDataManager::initialize(
    RenderingDevice* p_rd, PTResourceManager* resource_manager) {
    _rd = p_rd;
    _resource_manager = resource_manager;
    _material_lib.initialize(_resource_manager);
}

void godot::PTSceneDataManager::update_buffers(
    Ref<PTRendererStats> stats, Node* root,
    const Ref<PTRendererSettings> render_settings) {
    uint64_t start_t = Time::get_singleton()->get_ticks_msec();

    _stats = stats;
    _root = root;

    _material_lib.clear();

    TypedArray<PTAnalyticalGeometry> all_analytical =
        PTUtils::gather_nodes_of_type<PTAnalyticalGeometry>(_root);

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
        PTUtils::gather_nodes_of_type<MeshInstance3D>(_root);

    _update_triangles_buffer(mesh_instances, render_settings);

    _update_materials_buffer();

    _update_textures_buffer();
    _stats->set_texture_count(_material_lib.get_textures().size());

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
        uint32_t material_index =
            _material_lib.get_or_push_material(sphere->get_material());
        print_line("Sphere material index: " +
                   String::num_int64(material_index));
        spheres_data[base_offset + material_offset] = float(material_index);
    }

    PackedByteArray spheres_bytes = spheres_data.to_byte_array();
    _resource_manager->update_storage_buffer("spheres", spheres_bytes);
    _stats->set_sphere_count(spheres.size());
}

void godot::PTSceneDataManager::_update_triangles_buffer(
    const TypedArray<MeshInstance3D>& meshes,
    const Ref<PTRendererSettings> render_settings) {
    PTGeometryResult result =
        _geometry_processor.process_scene_geometry(meshes, &_material_lib);

    // Write triangle count
    PackedFloat32Array triangles_count_data;
    triangles_count_data.push_back(float(result.triangles.size()));
    triangles_count_data.push_back(0.0);
    triangles_count_data.push_back(0.0);
    triangles_count_data.push_back(0.0);
    PackedByteArray triangle_count_bytes = triangles_count_data.to_byte_array();
    _resource_manager->update_storage_buffer("triangles", triangle_count_bytes);

    if (result.triangles.size() == 0) {
        print_line("No triangles to process.");
        return;
    }

    // Write vertices
    PackedFloat32Array vertex_buffer;
    for (PTVertex& vertex : result.vertices) {
        vertex_buffer.push_back(vertex.position.x);
        vertex_buffer.push_back(vertex.position.y);
        vertex_buffer.push_back(vertex.position.z);
        vertex_buffer.push_back(0.0);  // Padding

        vertex_buffer.push_back(vertex.color.x);
        vertex_buffer.push_back(vertex.color.y);
        vertex_buffer.push_back(vertex.color.z);
        vertex_buffer.push_back(0.0);  // Padding

        vertex_buffer.push_back(vertex.normal.x);
        vertex_buffer.push_back(vertex.normal.y);
        vertex_buffer.push_back(vertex.normal.z);
        vertex_buffer.push_back(0.0);  // Padding

        vertex_buffer.push_back(vertex.uv.x);
        vertex_buffer.push_back(vertex.uv.y);
        vertex_buffer.push_back(0.0);  // Padding
        vertex_buffer.push_back(0.0);  // Padding

        vertex_buffer.push_back(vertex.tangent.x);
        vertex_buffer.push_back(vertex.tangent.y);
        vertex_buffer.push_back(vertex.tangent.z);
        vertex_buffer.push_back(vertex.tangent.w);
    }
    PackedByteArray vertices_bytes = vertex_buffer.to_byte_array();
    _resource_manager->update_storage_buffer("vertex", vertices_bytes);

    _stats->set_triangle_count(result.triangles.size());
    _stats->set_vertex_count(vertex_buffer.size());
    // Build BVH
    uint64_t start_t = Time::get_singleton()->get_ticks_msec();
    PTBoundingVolumeHierarchy bvh;

    BVHSettings bvh_settings{
        .max_depth = render_settings->get_bvh_max_depth(),
        .max_triangles_per_leaf =
            render_settings->get_bvh_max_triangles_per_leaf(),
        .sah_bins = render_settings->get_bvh_sah_bins()};

    bvh.build(result.vertices, result.triangles, bvh_settings);

    uint64_t end_t = Time::get_singleton()->get_ticks_msec();
    print_line("BVH creation time: " + String::num_int64(end_t - start_t));
    print_line("BVH with " + String::num_int64(bvh.get_nodes().size()) +
               " nodes");

    uint32_t leaf_node_count = 0;
    uint32_t total_leaf_primitives = 0;
    uint32_t max_leaf_primitives = 0;
    uint32_t min_leaf_primitives = UINT32_MAX;
    for (auto& node : bvh.get_nodes()) {
        if (node.is_leaf) {
            leaf_node_count++;
            total_leaf_primitives += node.primitive_count;
            if (node.primitive_count > max_leaf_primitives) {
                max_leaf_primitives = node.primitive_count;
            }
            if (node.primitive_count < min_leaf_primitives) {
                min_leaf_primitives = node.primitive_count;
            }
        }
    }

    print_line("BVH leaf nodes: " + String::num_int64(leaf_node_count) +
               " total primitives in leaves: " +
               String::num_int64(total_leaf_primitives) + " average: " +
               String::num_real(float(total_leaf_primitives) /
                                float(leaf_node_count)) +
               " min: " + String::num_int64(min_leaf_primitives) +
               " max: " + String::num_int64(max_leaf_primitives));

    PackedFloat32Array sorted_triangles;
    sorted_triangles.resize(result.triangles.size() * 4);

    for (uint32_t i = 0; i < result.triangles.size(); i++) {
        PTTriangle& triangle = result.triangles[i];
        sorted_triangles[i * 4 + 0] = triangle.i0;
        sorted_triangles[i * 4 + 1] = triangle.i1;
        sorted_triangles[i * 4 + 2] = triangle.i2;
        sorted_triangles[i * 4 + 3] = triangle.materialIndex;
    }

    PackedByteArray sorted_triangles_bytes = sorted_triangles.to_byte_array();
    _resource_manager->update_storage_buffer(
        "triangles", sorted_triangles_bytes, 4 * sizeof(float));

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
    _resource_manager->update_storage_buffer("bvh", bvh_nodes_byte_array);

    _stats->set_bvh_node_count(bvh.get_nodes().size());
}

void godot::PTSceneDataManager::_update_materials_buffer() {
    PackedFloat32Array materials_data;

    constexpr uint32_t MATERIAL_FLOATS = 20;

    const std::vector<PTMaterial>& materials = _material_lib.get_materials();

    materials_data.resize(materials.size() * MATERIAL_FLOATS);
    for (uint32_t i = 0; i < materials.size(); ++i) {
        const PTMaterial& material = materials[i];
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
        materials_data[base_offset + 15] = material.metallic_texture_channel;
        materials_data[base_offset + 16] = material.roughness_texture_channel;
        materials_data[base_offset + 17] = material.emission_energy_multiplier;
    }

    PackedByteArray materials_bytes = materials_data.to_byte_array();
    _resource_manager->update_storage_buffer("materials", materials_bytes);
    _stats->set_material_count(materials.size());
}

void godot::PTSceneDataManager::_update_textures_buffer() {
    const std::vector<Ref<Texture2D>>& textures = _material_lib.get_textures();

    print_line("Updating textures buffer. Total textures: " +
               String::num_int64(textures.size()));

    // //combine textures into a single large texture
    for (size_t i = 0; i < textures.size(); i++) {
        auto image = textures[i]->get_image();
        image->clear_mipmaps();
        image->decompress();
        image->convert(Image::FORMAT_RGBA8);
        image->resize(_resource_manager->get_texture_array_resolution(),
                      _resource_manager->get_texture_array_resolution());
        if (_rd->texture_update(_resource_manager->get_scene_texture_array(), i,
                                image->get_data()) != Error::OK) {
            ERR_PRINT("Failed to update texture array layer index: " +
                      String::num_int64(i));
        }
    }
}