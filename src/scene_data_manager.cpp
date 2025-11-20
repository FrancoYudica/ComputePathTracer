#include "scene_data_manager.h"
#include <godot_cpp/classes/node.hpp>
#include "ptnode.h"


void godot::SceneDataManager::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("initialize", "rd", "tree", "spheres_storage_buffer", "triangles_storage_buffer", "vertices_storage_buffer", "materials_storage_buffer"), &SceneDataManager::initialize);
    ClassDB::bind_method(D_METHOD("update_buffers"), &SceneDataManager::update_buffers);

    ClassDB::bind_method(D_METHOD("get_sphere_count"), &SceneDataManager::get_sphere_count);
    ClassDB::bind_method(D_METHOD("get_triangle_count"), &SceneDataManager::get_triangle_count);
    ClassDB::bind_method(D_METHOD("get_vertex_count"), &SceneDataManager::get_vertex_count);
    ClassDB::bind_method(D_METHOD("get_material_count"), &SceneDataManager::get_material_count);
}

godot::SceneDataManager::SceneDataManager() 
    : _rd(nullptr), 
      _tree(nullptr), 
      _spheres_storage_buffer(RID()), 
      _triangles_storage_buffer(RID()), 
      _vertices_storage_buffer(RID()), 
      _materials_storage_buffer(RID()), 
      _frame_materials({}),
      _frame_materials_list({})
{
}

godot::SceneDataManager::~SceneDataManager() 
{
}

void godot::SceneDataManager::initialize(
    RenderingDevice *p_rd, 
    SceneTree *p_tree,
    RID spheres_storage_buffer, 
    RID triangles_storage_buffer, 
    RID vertices_storage_buffer, 
    RID materials_storage_buffer)
{
    _rd = p_rd;
    _tree = p_tree;
    _spheres_storage_buffer = spheres_storage_buffer;
    _triangles_storage_buffer = triangles_storage_buffer;
    _vertices_storage_buffer = vertices_storage_buffer;
    _materials_storage_buffer = materials_storage_buffer;
}

void godot::SceneDataManager::update_buffers()
{
    _frame_materials.clear();
    _frame_materials_list.clear();

    TypedArray<Node> all_nodes = _tree->get_nodes_in_group("path_tracer_objects");

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
}

void godot::SceneDataManager::_update_spheres_buffer(
    const TypedArray<Node>& all_nodes, 
    const std::vector<uint32_t>& sphere_indices)
{   

    Ref<PTMaterial> defaultMaterial = Ref<PTMaterial>(memnew(PTMaterial()));
    _push_material(defaultMaterial); // Ensure default material is at index 0

    constexpr uint32_t SPHERE_FLOATS = 8; // vec3 center + float radius + uint32_t material_index + 3 padding
    PackedFloat32Array spheres_data;
    spheres_data.resize(sphere_indices.size() * SPHERE_FLOATS + 4); // +4 for spheres count at start
    spheres_data[0] = float(sphere_indices.size());
    for (uint32_t i = 0; i < sphere_indices.size(); ++i) {
        PTNode* sphere = Object::cast_to<PTNode>(all_nodes[sphere_indices[i]]);
        uint32_t base_offset = i * SPHERE_FLOATS + 4;
        uint32_t material_offset = 4;
        spheres_data[base_offset + 0] = sphere->get_global_position().x;
        spheres_data[base_offset + 1] = sphere->get_global_position().y;
        spheres_data[base_offset + 2] = sphere->get_global_position().z;    
        spheres_data[base_offset + 3] = sphere->get_scale().x * 0.5; // radius
        Ref<PTMaterial> material = sphere->get_material();
        uint32_t material_index = _push_material(material);
        spheres_data[base_offset + material_offset] = float(material_index);
    }

    PackedByteArray spheres_bytes = spheres_data.to_byte_array();
    _rd->buffer_update(_spheres_storage_buffer, 0, spheres_bytes.size(), spheres_bytes);
    _frame_stats.sphere_count = uint32_t(sphere_indices.size());
}

void godot::SceneDataManager::_update_triangles_buffer(
    const TypedArray<Node>& all_nodes, 
    const std::vector<uint32_t>& triangle_mesh_indices)
{
    PackedFloat32Array triangles_data;
    PackedVector4Array vertices_data;

    triangles_data.push_back(0.0f); // Placeholder for triangle count
    triangles_data.push_back(0.0f); // Placeholder for triangle count
    triangles_data.push_back(0.0f); // Placeholder for triangle count
    triangles_data.push_back(0.0f); // Placeholder for triangle count

    uint32_t triangle_count = 0;
    for (uint32_t i = 0; i < triangle_mesh_indices.size(); ++i) {

        // Access PTNode mesh
        PTNode* triangle_mesh_node = Object::cast_to<PTNode>(all_nodes[triangle_mesh_indices[i]]);
        Ref<Mesh> mesh = triangle_mesh_node->get_mesh();

        // Ignore when null
        if (mesh.is_null()) {
            continue;
        }
        // Currently, a single material for the entire mesh
        uint32_t mesh_material_index = _push_material(triangle_mesh_node->get_material());

        for (uint32_t surface_idx = 0; surface_idx < mesh->get_surface_count(); ++surface_idx) {
            Array arr = mesh->surface_get_arrays(surface_idx);
            PackedVector3Array surface_vertices = arr[ArrayMesh::ARRAY_VERTEX];
            PackedInt32Array surface_indices = arr[ArrayMesh::ARRAY_INDEX];
            
            // Base offset relative to current vertices size
            uint32_t base_index_offset = uint32_t(vertices_data.size());

            // Add vertices
            for (uint32_t v = 0; v < surface_vertices.size(); ++v) {
                Vector3 vertex = surface_vertices[v];
                // Apply global transform
                vertex = triangle_mesh_node->get_global_transform().xform(vertex);
                vertices_data.push_back(Vector4(vertex.x, vertex.y, vertex.z, 1.0f));
            }

            // Add triangles
            for (uint32_t idx = 0; idx < surface_indices.size(); idx += 3) {
                uint32_t i0 = base_index_offset + uint32_t(surface_indices[idx + 0]);
                uint32_t i1 = base_index_offset + uint32_t(surface_indices[idx + 1]);
                uint32_t i2 = base_index_offset + uint32_t(surface_indices[idx + 2]);

                triangles_data.push_back(float(i0));
                triangles_data.push_back(float(i1));
                triangles_data.push_back(float(i2));
                triangles_data.push_back(float(mesh_material_index));
            }

            triangle_count += surface_indices.size() / 3;
        }
    }
    triangles_data[0] = float(triangle_count); // Sets triangle counts

    PackedByteArray triangles_bytes = triangles_data.to_byte_array();
    _rd->buffer_update(_triangles_storage_buffer, 0, triangles_bytes.size(), triangles_bytes);

    PackedByteArray vertices_bytes = vertices_data.to_byte_array();
    _rd->buffer_update(_vertices_storage_buffer, 0, vertices_bytes.size(), vertices_bytes);

    _frame_stats.triangle_count = triangle_count;
    _frame_stats.vertex_count = uint32_t(vertices_data.size());
}


void godot::SceneDataManager::_update_materials_buffer()
{
    PackedFloat32Array materials_data;
    constexpr uint32_t MATERIAL_FLOATS = 8; // vec3 color + float metallic + float roughness + float refraction_index + float emission + uint32_t material_type + 3 padding
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
    _rd->buffer_update(_materials_storage_buffer, 0, materials_bytes.size(), materials_bytes);
    _frame_stats.material_count = uint32_t(_frame_materials_list.size());
}

uint32_t godot::SceneDataManager::_push_material(const Ref<PTMaterial> &material)
{
    if (material.is_null()) {
        return 0; // Default material index
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
