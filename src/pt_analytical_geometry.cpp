#include "pt_analytical_geometry.h"

void godot::PTAnalyticalGeometry::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_material", "material"),
                         &PTAnalyticalGeometry::set_material);
    ClassDB::bind_method(D_METHOD("get_material"),
                         &PTAnalyticalGeometry::get_material);

    ClassDB::bind_method(D_METHOD("set_node_type", "node_type"),
                         &PTAnalyticalGeometry::set_node_type);
    ClassDB::bind_method(D_METHOD("get_node_type"),
                         &PTAnalyticalGeometry::get_node_type);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "node_type", PROPERTY_HINT_ENUM, "Sphere"),
        "set_node_type", "get_node_type");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "material",
                              PROPERTY_HINT_RESOURCE_TYPE, "PTMaterial"),
                 "set_material", "get_material");

    BIND_ENUM_CONSTANT(NODE_TYPE_SPHERE);
}

godot::PTAnalyticalGeometry::PTAnalyticalGeometry() {}

godot::PTAnalyticalGeometry::~PTAnalyticalGeometry() {}

void godot::PTAnalyticalGeometry::_ready() {
    add_to_group("path_tracer_objects");
}
void godot::PTAnalyticalGeometry::set_material(Ref<Material> p_material) {
    material = p_material;
}

godot::Ref<godot::Material> godot::PTAnalyticalGeometry::get_material() const {
    return material;
}

void godot::PTAnalyticalGeometry::set_node_type(
    PTAnalyticalGeometryType p_node_type) {
    node_type = p_node_type;
}

godot::PTAnalyticalGeometryType godot::PTAnalyticalGeometry::get_node_type()
    const {
    return node_type;
}