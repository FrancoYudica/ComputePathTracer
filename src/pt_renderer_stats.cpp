#include "pt_renderer_stats.h"

void godot::PTRendererStats::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_sphere_count"),
                         &PTRendererStats::get_sphere_count);
    ClassDB::bind_method(D_METHOD("set_sphere_count", "count"),
                         &PTRendererStats::set_sphere_count);

    ClassDB::bind_method(D_METHOD("get_triangle_count"),
                         &PTRendererStats::get_triangle_count);
    ClassDB::bind_method(D_METHOD("set_triangle_count", "count"),
                         &PTRendererStats::set_triangle_count);

    ClassDB::bind_method(D_METHOD("get_vertex_count"),
                         &PTRendererStats::get_vertex_count);
    ClassDB::bind_method(D_METHOD("set_vertex_count", "count"),
                         &PTRendererStats::set_vertex_count);

    ClassDB::bind_method(D_METHOD("get_texture_count"),
                         &PTRendererStats::get_texture_count);
    ClassDB::bind_method(D_METHOD("set_texture_count", "count"),
                         &PTRendererStats::set_texture_count);

    ClassDB::bind_method(D_METHOD("get_material_count"),
                         &PTRendererStats::get_material_count);
    ClassDB::bind_method(D_METHOD("set_material_count", "count"),
                         &PTRendererStats::set_material_count);

    ClassDB::bind_method(D_METHOD("get_bvh_node_count"),
                         &PTRendererStats::get_bvh_node_count);
    ClassDB::bind_method(D_METHOD("set_bvh_node_count", "count"),
                         &PTRendererStats::set_bvh_node_count);
}

void godot::PTRendererStats::reset() {
    sphere_count = 0;
    triangle_count = 0;
    vertex_count = 0;
    texture_count = 0;
    material_count = 0;
    bvh_node_count = 0;
}
