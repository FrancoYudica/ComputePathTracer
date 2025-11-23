#include "pt_scene_data_manager.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/time.hpp>
#include <unordered_set>

#include "pt_bounding_volume_hierarchy.h"
#include "pt_node.h"

void godot::PTSceneDataManager::_bind_methods() {
    ClassDB::bind_method(
        D_METHOD("initialize", "rd", "tree", "spheres_storage_buffer",
                 "triangles_storage_buffer", "vertices_storage_buffer",
                 "materials_storage_buffer"),
        &PTSceneDataManager::initialize);
    ClassDB::bind_method(D_METHOD("update_buffers"),
                         &PTSceneDataManager::update_buffers);

    ClassDB::bind_method(D_METHOD("get_sphere_count"),
                         &PTSceneDataManager::get_sphere_count);
    ClassDB::bind_method(D_METHOD("get_triangle_count"),
                         &PTSceneDataManager::get_triangle_count);
    ClassDB::bind_method(D_METHOD("get_vertex_count"),
                         &PTSceneDataManager::get_vertex_count);
    ClassDB::bind_method(D_METHOD("get_material_count"),
                         &PTSceneDataManager::get_material_count);
}

godot::PTSceneDataManager::PTSceneDataManager()
    : _rd(nullptr),
      _tree(nullptr),
      _spheres_storage_buffer(RID()),
      _triangles_storage_buffer(RID()),
      _vertices_storage_buffer(RID()),
      _materials_storage_buffer(RID()),
      _frame_materials({}),
      _frame_materials_list({}) {}

godot::PTSceneDataManager::~PTSceneDataManager() {}

void godot::PTSceneDataManager::initialize(
    RenderingDevice* p_rd, SceneTree* p_tree, RID spheres_storage_buffer,
    RID triangles_storage_buffer, RID vertices_storage_buffer,
    RID materials_storage_buffer, RID bvh_storage_buffer) {
    _rd = p_rd;
    _tree = p_tree;
    _spheres_storage_buffer = spheres_storage_buffer;
    _triangles_storage_buffer = triangles_storage_buffer;
    _vertices_storage_buffer = vertices_storage_buffer;
    _materials_storage_buffer = materials_storage_buffer;
    _bvh_storage_buffer = bvh_storage_buffer;
}

void godot::PTSceneDataManager::update_buffers() {
    uint64_t start_t = Time::get_singleton()->get_ticks_msec();
    _frame_materials.clear();
    _frame_materials_list.clear();

    TypedArray<Node> all_nodes =
        _tree->get_nodes_in_group("path_tracer_objects");

    std::vector<uint32_t> sphere_indices;
    std::vector<uint32_t> triangle_indices;

    for (uint32_t i = 0; i < all_nodes.size(); ++i) {
        PTNode* n = Object::cast_to<PTNode>(all_nodes[i]);
        if (n && n->is_visible_in_tree()) {
            if (n->get_node_type() == NODE_TYPE_TRIANGLE_MESH) {
                triangle_indices.push_back(i);
            } else if (n->get_node_type() == NODE_TYPE_SPHERE) {
                sphere_indices.push_back(i);
            }
        }
    }

    _update_spheres_buffer(all_nodes, sphere_indices);
    _update_triangles_buffer(all_nodes, triangle_indices);
    _update_materials_buffer();

    uint64_t end_t = Time::get_singleton()->get_ticks_msec();
    print_line("Elapsed on update: " + String::num_int64(end_t - start_t));
}

void godot::PTSceneDataManager::_update_spheres_buffer(
    const TypedArray<Node>& all_nodes,
    const std::vector<uint32_t>& sphere_indices) {
    Ref<PTMaterial> defaultMaterial = Ref<PTMaterial>(memnew(PTMaterial()));
    _push_material(defaultMaterial);  // Ensure default material is at index 0

    constexpr uint32_t SPHERE_FLOATS =
        8;  // vec3 center + float radius + uint32_t material_index + 3 padding
    PackedFloat32Array spheres_data;
    spheres_data.resize(sphere_indices.size() * SPHERE_FLOATS +
                        4);  // +4 for spheres count at start
    spheres_data[0] = float(sphere_indices.size());
    for (uint32_t i = 0; i < sphere_indices.size(); ++i) {
        PTNode* sphere = Object::cast_to<PTNode>(all_nodes[sphere_indices[i]]);
        uint32_t base_offset = i * SPHERE_FLOATS + 4;
        uint32_t material_offset = 4;
        spheres_data[base_offset + 0] = sphere->get_global_position().x;
        spheres_data[base_offset + 1] = sphere->get_global_position().y;
        spheres_data[base_offset + 2] = sphere->get_global_position().z;
        spheres_data[base_offset + 3] = sphere->get_scale().x * 0.5;  // radius
        Ref<PTMaterial> material = sphere->get_material();
        uint32_t material_index = _push_material(material);
        spheres_data[base_offset + material_offset] = float(material_index);
    }

    PackedByteArray spheres_bytes = spheres_data.to_byte_array();
    _rd->buffer_update(_spheres_storage_buffer, 0, spheres_bytes.size(),
                       spheres_bytes);
    _stats.sphere_count = uint32_t(sphere_indices.size());
}

void godot::PTSceneDataManager::_update_triangles_buffer(
    const TypedArray<Node>& all_nodes,
    const std::vector<uint32_t>& triangle_mesh_indices) {
    std::vector<PTTriangle> triangles;
    std::vector<PTVertex> vertices;

    PackedFloat32Array vertices_data;

    for (uint32_t i = 0; i < triangle_mesh_indices.size(); ++i) {
        // Access PTNode mesh
        PTNode* triangle_mesh_node =
            Object::cast_to<PTNode>(all_nodes[triangle_mesh_indices[i]]);
        Ref<Mesh> mesh = triangle_mesh_node->get_mesh();

        // Ignore when null
        if (mesh.is_null()) {
            continue;
        }
        // Currently, a single material for the entire mesh
        uint32_t mesh_material_index =
            _push_material(triangle_mesh_node->get_material());

        for (uint32_t surface_idx = 0; surface_idx < mesh->get_surface_count();
             ++surface_idx) {
            Array arr = mesh->surface_get_arrays(surface_idx);
            PackedVector3Array surface_vertices = arr[ArrayMesh::ARRAY_VERTEX];
            PackedInt32Array surface_indices = arr[ArrayMesh::ARRAY_INDEX];
            PackedColorArray surface_colors = arr[ArrayMesh::ARRAY_COLOR];
            PackedVector3Array surface_normals = arr[ArrayMesh::ARRAY_NORMAL];
            print_line(
                "Color count: " + String::num_int64(surface_colors.size()),
                " Normal count: " + String::num_int64(surface_normals.size()));

            // Base offset relative to current vertices size
            uint32_t base_index_offset = uint32_t(vertices.size());

            // Add vertices
            for (uint32_t v = 0; v < surface_vertices.size(); ++v) {
                Vector3 position = surface_vertices[v];
                Color color = surface_colors.size() > 0
                                  ? surface_colors[v]
                                  : Color(1.0, 1.0, 1.0, 1.0);

                Vector3 normal = surface_normals.size() > 0
                                     ? surface_normals[v]
                                     : Vector3(0.0, 1.0, 0.0);

                position = triangle_mesh_node->get_transform().xform(position);

                // TODO: Handle translation/scale properly
                normal =
                    triangle_mesh_node->get_transform().basis.xform(normal);

                PTVertex vertex = {position, Vector3(color.r, color.g, color.b),
                                   normal};

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

    // Write triangle count
    PackedFloat32Array triangles_count_data;
    triangles_count_data.push_back(float(triangles.size()));
    triangles_count_data.push_back(0.0);
    triangles_count_data.push_back(0.0);
    triangles_count_data.push_back(0.0);
    PackedByteArray triangle_count_bytes = triangles_count_data.to_byte_array();
    _rd->buffer_update(_triangles_storage_buffer, 0,
                       triangle_count_bytes.size(), triangle_count_bytes);

    // Write vertices
    PackedByteArray vertices_bytes = vertices_data.to_byte_array();
    _rd->buffer_update(_vertices_storage_buffer, 0, vertices_bytes.size(),
                       vertices_bytes);

    _stats.triangle_count = triangles.size();
    _stats.vertex_count = uint32_t(vertices_data.size());
    print_line("Total vertices: " + String::num_int64(vertices_data.size()) +
               ", bytes: " + String::num_int64(vertices_bytes.size()));
    // Build BVH
    uint64_t start_t = Time::get_singleton()->get_ticks_msec();
    PTBoundingVolumeHierarchy bvh;
    bvh.build(vertices, triangles);
    uint64_t end_t = Time::get_singleton()->get_ticks_msec();
    print_line("BVH creation time: " + String::num_int64(end_t - start_t));
    print_line("BVH with " + String::num_int64(bvh.get_nodes().size()) +
               " nodes");

    std::vector<uint32_t> nodeIndices;
    nodeIndices.push_back(0);

    while (nodeIndices.size() > 0) {
        uint32_t nodeIndex = nodeIndices.at(0);
        nodeIndices.erase(nodeIndices.begin());
        const PTBoundingVolumeNode& node = bvh.get_nodes()[nodeIndex];

        if (!node.is_leaf) {
            nodeIndices.push_back(node.left_child_index);
            nodeIndices.push_back(node.right_child_index);
        }

        print_line(
            "Node " + String::num_int64(nodeIndex) + ". Min(" +
            String::num_real(node.aabb.min.x) + ", " +
            String::num_real(node.aabb.min.y) + ", " +
            String::num_real(node.aabb.min.z) + ") " + ", Max(" +
            String::num_real(node.aabb.max.x) + ", " +
            String::num_real(node.aabb.max.y) + ", " +
            String::num_real(node.aabb.max.z) + ") " + ", Left child index: " +
            String::num_int64(node.left_child_index) + ", Right child index: " +
            String::num_int64(node.right_child_index) + ", Primitive start: " +
            String::num_int64(node.primitive_start_index) +
            ", Primitive count: " + String::num_int64(node.primitive_count));
    }

    PackedFloat32Array sortedTriangles;
    sortedTriangles.resize(triangles.size() * 4);

    for (uint32_t i = 0; i < triangles.size(); i++) {
        PTTriangle& triangle = triangles[i];
        sortedTriangles[i * 4 + 0] = triangle.i0;
        sortedTriangles[i * 4 + 1] = triangle.i1;
        sortedTriangles[i * 4 + 2] = triangle.i2;
        sortedTriangles[i * 4 + 3] = triangle.materialIndex;
    }

    PackedByteArray sorted_triangles_bytes = sortedTriangles.to_byte_array();
    _rd->buffer_update(_triangles_storage_buffer, 4 * 4,
                       sorted_triangles_bytes.size(), sorted_triangles_bytes);

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
    _rd->buffer_update(_bvh_storage_buffer, 0, bvh_nodes_byte_array.size(),
                       bvh_nodes_byte_array);
}

void godot::PTSceneDataManager::_update_materials_buffer() {
    PackedFloat32Array materials_data;

    // vec3 color + float metallic + float roughness + float
    // refraction_index + float emission + uint32_t material_type + 3
    // padding
    constexpr uint32_t MATERIAL_FLOATS = 8;

    materials_data.resize(_frame_materials_list.size() * MATERIAL_FLOATS);
    for (uint32_t i = 0; i < _frame_materials_list.size(); ++i) {
        Ref<PTMaterial> material = _frame_materials_list[i];
        uint32_t base_offset = i * MATERIAL_FLOATS;
        Color color = material->get_color();
        materials_data[base_offset + 0] = float(material->get_material_type());
        materials_data[base_offset + 1] = material->get_metallic();
        materials_data[base_offset + 2] = material->get_roughness();
        materials_data[base_offset + 3] = material->get_refraction_index();
        materials_data[base_offset + 4] = color.r;
        materials_data[base_offset + 5] = color.g;
        materials_data[base_offset + 6] = color.b;
        materials_data[base_offset + 7] = material->get_emission();
    }

    PackedByteArray materials_bytes = materials_data.to_byte_array();
    _rd->buffer_update(_materials_storage_buffer, 0, materials_bytes.size(),
                       materials_bytes);
    _stats.material_count = uint32_t(_frame_materials_list.size());
}

uint32_t godot::PTSceneDataManager::_push_material(
    const Ref<PTMaterial>& material) {
    if (material.is_null()) {
        return 0;  // Default material index
    }

    size_t hash = material->get_hash();
    if (!_frame_materials.count(hash)) {
        uint32_t index = uint32_t(_frame_materials_list.size());
        _frame_materials[hash] = index;
        _frame_materials_list.push_back(material);
        return index;
    } else {
        return uint32_t(_frame_materials[hash]);
    }
}
