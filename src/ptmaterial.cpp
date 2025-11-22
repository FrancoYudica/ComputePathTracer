#include "ptmaterial.h"

#include <functional>

void godot::PTMaterial::_bind_methods() {
    // Binds methods
    ClassDB::bind_method(D_METHOD("set_color", "color"),
                         &PTMaterial::set_color);
    ClassDB::bind_method(D_METHOD("get_color"), &PTMaterial::get_color);

    ClassDB::bind_method(D_METHOD("set_metallic", "metallic"),
                         &PTMaterial::set_metallic);
    ClassDB::bind_method(D_METHOD("get_metallic"), &PTMaterial::get_metallic);

    ClassDB::bind_method(D_METHOD("set_roughness", "roughness"),
                         &PTMaterial::set_roughness);
    ClassDB::bind_method(D_METHOD("get_roughness"), &PTMaterial::get_roughness);

    ClassDB::bind_method(D_METHOD("set_refraction_index", "refraction_index"),
                         &PTMaterial::set_refraction_index);
    ClassDB::bind_method(D_METHOD("get_refraction_index"),
                         &PTMaterial::get_refraction_index);

    ClassDB::bind_method(D_METHOD("set_emission", "emission"),
                         &PTMaterial::set_emission);
    ClassDB::bind_method(D_METHOD("get_emission"), &PTMaterial::get_emission);

    ClassDB::bind_method(D_METHOD("get_material_type"),
                         &PTMaterial::get_material_type);
    ClassDB::bind_method(D_METHOD("set_material_type", "material_type"),
                         &PTMaterial::set_material_type);

    // Add properties
    ADD_PROPERTY(PropertyInfo(Variant::INT, "material_type", PROPERTY_HINT_ENUM,
                              "Lambertian,Metal,Dielectric,Emissive"),
                 "set_material_type", "get_material_type");
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color"), "set_color",
                 "get_color");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "metallic", PROPERTY_HINT_RANGE,
                              "0,1,0.01"),
                 "set_metallic", "get_metallic");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "roughness", PROPERTY_HINT_RANGE,
                              "0,1,0.01"),
                 "set_roughness", "get_roughness");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "refraction_index",
                              PROPERTY_HINT_RANGE, "0,3,0.01"),
                 "set_refraction_index", "get_refraction_index");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "emission", PROPERTY_HINT_RANGE,
                              "0,100,0.01"),
                 "set_emission", "get_emission");

    BIND_ENUM_CONSTANT(MATERIAL_TYPE_LAMBERTIAN);
    BIND_ENUM_CONSTANT(MATERIAL_TYPE_METAL);
    BIND_ENUM_CONSTANT(MATERIAL_TYPE_DIELECTRIC);
    BIND_ENUM_CONSTANT(MATERIAL_TYPE_EMISSIVE);
}

godot::PTMaterial::PTMaterial()
    : material_type(MATERIAL_TYPE_LAMBERTIAN),
      color(Color(1.0, 1.0, 1.0)),
      metallic(0.0),
      roughness(0.5),
      refraction_index(1.5),
      emission(0.0) {}

void godot::PTMaterial::set_color(const Color& p_color) { color = p_color; }

godot::Color godot::PTMaterial::get_color() const { return color; }

void godot::PTMaterial::set_metallic(float p_metallic) {
    metallic = p_metallic;
}

float godot::PTMaterial::get_metallic() const { return metallic; }

void godot::PTMaterial::set_roughness(float p_roughness) {
    roughness = p_roughness;
}

float godot::PTMaterial::get_roughness() const { return roughness; }

void godot::PTMaterial::set_refraction_index(float p_refraction_index) {
    refraction_index = p_refraction_index;
}

float godot::PTMaterial::get_refraction_index() const {
    return refraction_index;
}

void godot::PTMaterial::set_emission(float p_emission) {
    emission = p_emission;
}

float godot::PTMaterial::get_emission() const { return emission; }

godot::MaterialType godot::PTMaterial::get_material_type() const {
    return material_type;
}

void godot::PTMaterial::set_material_type(MaterialType p_material_type) {
    material_type = p_material_type;
}

static void hash_combine(size_t& seed, size_t value) {
    seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

size_t godot::PTMaterial::get_hash() const {
    size_t hash = 0;
    // Use std::hash for floats/ints to avoid truncation and collisions
    hash_combine(hash, std::hash<int>()(static_cast<int>(material_type)));
    hash_combine(hash, std::hash<float>()(color.r));
    hash_combine(hash, std::hash<float>()(color.g));
    hash_combine(hash, std::hash<float>()(color.b));
    hash_combine(hash, std::hash<float>()(color.a));
    hash_combine(hash, std::hash<float>()(metallic));
    hash_combine(hash, std::hash<float>()(roughness));
    hash_combine(hash, std::hash<float>()(refraction_index));
    hash_combine(hash, std::hash<float>()(emission));
    return hash;
}

godot::String godot::PTMaterial::to_string() const {
    return "PTMaterial(type=" +
           String::num_int64(static_cast<int64_t>(material_type)) +
           ", color=" + color.to_html(true) +
           ", metallic=" + String::num_real(metallic) +
           ", roughness=" + String::num_real(roughness) +
           ", refraction_index=" + String::num_real(refraction_index) +
           ", emission=" + String::num_real(emission) + ")";
}
