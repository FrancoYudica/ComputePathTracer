#include "ptnode.h"

void godot::PTNode::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_material", "material"),
                         &PTNode::set_material);
    ClassDB::bind_method(D_METHOD("get_material"), &PTNode::get_material);

    ClassDB::bind_method(D_METHOD("set_node_type", "node_type"),
                         &PTNode::set_node_type);
    ClassDB::bind_method(D_METHOD("get_node_type"), &PTNode::get_node_type);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "node_type", PROPERTY_HINT_ENUM,
                              "TriangleMesh,Sphere"),
                 "set_node_type", "get_node_type");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "material",
                              PROPERTY_HINT_RESOURCE_TYPE, "PTMaterial"),
                 "set_material", "get_material");

    BIND_ENUM_CONSTANT(NODE_TYPE_TRIANGLE_MESH);
    BIND_ENUM_CONSTANT(NODE_TYPE_SPHERE);
}

godot::PTNode::PTNode() {}

godot::PTNode::~PTNode() {}

void godot::PTNode::_ready() { add_to_group("path_tracer_objects"); }

void godot::PTNode::set_material(Ref<PTMaterial> p_material) {
    material = p_material;
}

godot::Ref<godot::PTMaterial> godot::PTNode::get_material() const {
    return material;
}

void godot::PTNode::set_node_type(PTNodeType p_node_type) {
    node_type = p_node_type;
}

godot::PTNodeType godot::PTNode::get_node_type() const { return node_type; }
