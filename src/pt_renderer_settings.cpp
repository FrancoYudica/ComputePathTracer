#include "pt_renderer_settings.h"

void godot::PTRendererSettings::_bind_methods() {
    // Method bindings
    ClassDB::bind_method(D_METHOD("set_samples_per_pixel", "samples_per_pixel"),
                         &PTRendererSettings::set_samples_per_pixel);
    ClassDB::bind_method(D_METHOD("get_samples_per_pixel"),
                         &PTRendererSettings::get_samples_per_pixel);

    ClassDB::bind_method(D_METHOD("set_max_bounces", "max_bounces"),
                         &PTRendererSettings::set_max_bounces);
    ClassDB::bind_method(D_METHOD("get_max_bounces"),
                         &PTRendererSettings::get_max_bounces);

    ClassDB::bind_method(
        D_METHOD("set_environment_energy", "environment_energy"),
        &PTRendererSettings::set_environment_energy);
    ClassDB::bind_method(D_METHOD("get_environment_energy"),
                         &PTRendererSettings::get_environment_energy);

    ClassDB::bind_method(D_METHOD("set_camera_aperture", "camera_aperture"),
                         &PTRendererSettings::set_camera_aperture);
    ClassDB::bind_method(D_METHOD("get_camera_aperture"),
                         &PTRendererSettings::get_camera_aperture);

    ClassDB::bind_method(
        D_METHOD("set_camera_focus_distance", "camera_focus_distance"),
        &PTRendererSettings::set_camera_focus_distance);
    ClassDB::bind_method(D_METHOD("get_camera_focus_distance"),
                         &PTRendererSettings::get_camera_focus_distance);

    ClassDB::bind_method(D_METHOD("set_render_mode", "render_mode"),
                         &PTRendererSettings::set_render_mode);
    ClassDB::bind_method(D_METHOD("get_render_mode"),
                         &PTRendererSettings::get_render_mode);

    ClassDB::bind_method(D_METHOD("set_render_scale", "render_scale"),
                         &PTRendererSettings::set_render_scale);
    ClassDB::bind_method(D_METHOD("get_render_scale"),
                         &PTRendererSettings::get_render_scale);

    // Export properties for edit on the inspector
    // Render scale in range [0.1, 1.0]
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "render_scale",
                              PROPERTY_HINT_RANGE, "0.1,1.0,0.01"),
                 "set_render_scale", "get_render_scale");

    // Samples per pixel in range [1, 1024]
    ADD_PROPERTY(PropertyInfo(Variant::INT, "samples_per_pixel",
                              PROPERTY_HINT_RANGE, "1,1024,1"),
                 "set_samples_per_pixel", "get_samples_per_pixel");
    // Max bounces in range [1, 64]
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_bounces", PROPERTY_HINT_RANGE,
                              "1,64,1"),
                 "set_max_bounces", "get_max_bounces");
    // Environment energy in range [0.0, 10.0]
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "environment_energy",
                              PROPERTY_HINT_RANGE, "0.0,10.0,0.01"),
                 "set_environment_energy", "get_environment_energy");
    // Camera aperture in range [0.0, 32.0]
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "camera_aperture",
                              PROPERTY_HINT_RANGE, "0.0,32.0,0.01"),
                 "set_camera_aperture", "get_camera_aperture");
    // Camera focus distance in range [0.1, 1000.0]
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "camera_focus_distance",
                              PROPERTY_HINT_RANGE, "0.1,1000.0,0.1"),
                 "set_camera_focus_distance", "get_camera_focus_distance");
    // Render mode as enum
    ADD_PROPERTY(PropertyInfo(Variant::INT, "render_mode", PROPERTY_HINT_ENUM,
                              "Path Trace,Ray Cast,BVH, Normals, Depth, UVs"),
                 "set_render_mode", "get_render_mode");

    BIND_ENUM_CONSTANT(RENDER_MODE_PATH_TRACE);
    BIND_ENUM_CONSTANT(RENDER_MODE_RAY_CAST);
    BIND_ENUM_CONSTANT(RENDER_MODE_BVH);
    BIND_ENUM_CONSTANT(RENDER_MODE_NORMALS);
    BIND_ENUM_CONSTANT(RENDER_MODE_DEPTH);
    BIND_ENUM_CONSTANT(RENDER_MODE_UV);
}

void godot::PTRendererSettings::set_samples_per_pixel(uint32_t p_spp) {
    samples_per_pixel = p_spp;
    emit_changed();
}

void godot::PTRendererSettings::set_max_bounces(uint32_t p_bounces) {
    max_bounces = p_bounces;
    emit_changed();
}

void godot::PTRendererSettings::set_environment_energy(float p_energy) {
    environment_energy = p_energy;
    emit_changed();
}

void godot::PTRendererSettings::set_camera_aperture(float p_aperture) {
    camera_aperture = p_aperture;
    emit_changed();
}

void godot::PTRendererSettings::set_camera_focus_distance(float p_distance) {
    camera_focus_distance = p_distance;
    emit_changed();
}

void godot::PTRendererSettings::set_render_mode(PTRenderMode p_mode) {
    render_mode = p_mode;
    emit_changed();
}

void godot::PTRendererSettings::set_render_scale(float p_scale) {
    render_scale = p_scale;
    emit_changed();
}
