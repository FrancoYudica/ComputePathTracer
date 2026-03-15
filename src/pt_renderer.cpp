#include "pt_renderer.h"
#include "pt_utils.h"
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/viewport.hpp>

namespace godot {

    void PTRenderer::_bind_methods() {
        ClassDB::bind_method(D_METHOD("init"), &PTRenderer::init);
        ClassDB::bind_method(D_METHOD("destroy"), &PTRenderer::destroy);

        ClassDB::bind_method(D_METHOD("queue_clear"), &PTRenderer::queue_clear);
        ClassDB::bind_method(D_METHOD("get_texture_rid"),
                             &PTRenderer::get_texture_rid);
        ClassDB::bind_method(D_METHOD("get_render_width"),
                             &PTRenderer::get_render_width);
        ClassDB::bind_method(D_METHOD("get_render_height"),
                             &PTRenderer::get_render_height);
        ClassDB::bind_method(D_METHOD("update_scene"),
                             &PTRenderer::update_scene);

        ClassDB::bind_method(
            D_METHOD("set_renderer_settings", "renderer_settings"),
            &PTRenderer::set_renderer_settings);
        ClassDB::bind_method(D_METHOD("get_renderer_settings"),
                             &PTRenderer::get_renderer_settings);
        ClassDB::bind_method(D_METHOD("get_stats"), &PTRenderer::get_stats);

        ClassDB::bind_method(D_METHOD("draw", "camera", "width", "height"),
                             &PTRenderer::draw);

        ClassDB::bind_method(D_METHOD("_resize", "width", "height"),
                             &PTRenderer::_resize);
        // Export in inspector
        ADD_PROPERTY(
            PropertyInfo(Variant::OBJECT, "renderer_settings",
                         PROPERTY_HINT_RESOURCE_TYPE, "PTRendererSettings"),
            "set_renderer_settings", "get_renderer_settings");

        ADD_SIGNAL(MethodInfo("texture_changed",
                              PropertyInfo(Variant::RID, "texture_rid")));
    }

    void PTRenderer::init() {
        if (_initialized) {
            ERR_PRINT("PTRenderer is already initialized.");
            return;
        }

        if (!_renderer_settings.is_valid()) {
            _renderer_settings =
                Ref<PTRendererSettings>(memnew(PTRendererSettings));
        }

        _stats = Ref<PTRendererStats>(memnew(PTRendererStats));
        _stats->reset();

        _initialize_compute();
        update_scene();

        _renderer_settings->connect("changed", Callable(this, "queue_clear"));

        _initialized = true;
    }

    void PTRenderer::destroy() { _cleanup(); }

    RID PTRenderer::get_texture_rid() const {
        return _resource_manager.get_output_texture();
    }

    uint32_t PTRenderer::get_render_width() const {
        return Math::max(
            static_cast<uint32_t>(_viewport_width *
                                  _renderer_settings->get_render_scale()),
            1u);
    }

    uint32_t PTRenderer::get_render_height() const {
        return Math::max(
            static_cast<uint32_t>(_viewport_height *
                                  _renderer_settings->get_render_scale()),
            1u);
    };

    Ref<PTRendererSettings> PTRenderer::get_renderer_settings() const {
        return _renderer_settings;
    }

    void PTRenderer::set_renderer_settings(
        const Ref<PTRendererSettings>& settings) {
        _renderer_settings = settings;
        _renderer_settings->connect("changed", Callable(this, "queue_clear"));
    }

    void PTRenderer::queue_clear() { _clear_buffer = true; }

    void PTRenderer::update_scene() {
        _update_scene = true;
        queue_clear();
    }

    void PTRenderer::_cleanup() { _resource_manager.cleanup(); }

    void PTRenderer::_resize(uint32_t width, uint32_t height) {
        _resource_manager.resize(get_render_width(), get_render_height());
        emit_signal("texture_changed", get_texture_rid());
        queue_clear();
    }

    void PTRenderer::_clear_accumulated_buffer() {
        _frame_count = 1;
        _stats->set_samples(0);
    }

    void PTRenderer::draw(Camera3D* camera, uint32_t width, uint32_t height) {
        _viewport_width = width;
        _viewport_height = height;
        uint32_t current_width = get_render_width();
        uint32_t current_height = get_render_height();

        // Delays resize to avoid issues when called during rendering
        if (current_width != _last_render_width ||
            current_height != _last_render_height) {
            call_deferred("_resize", width, height);
            _last_render_width = current_width;
            _last_render_height = current_height;
        }

        if (_clear_buffer) {
            _clear_accumulated_buffer();
            _clear_buffer = false;
        }

        if (_update_scene) {
            _stats->reset();
            Node* root = Node::cast_to<Node>(camera->get_viewport());
            _scene_data_manager.update_buffers(_stats, root,
                                               _renderer_settings);
            _update_scene = false;
        }

        _resource_manager.load_skybox_from_camera(camera);

        PackedByteArray push_constant_bytes = _get_push_constant_bytes();

        // Assuming workgroup size of 8x8. Do ceiling division to cover the
        // entire texture.

        uint32_t x_groups =
            static_cast<uint32_t>(ceil(get_render_width() / 8.0));
        uint32_t y_groups =
            static_cast<uint32_t>(ceil(get_render_height() / 8.0));

        _update_settings_storage_buffer();
        _update_camera_storage_buffer(camera);

        // Updates resource manager
        _resource_manager.flush_pending_updates();

        int64_t compute_list = _rd->compute_list_begin();
        _rd->compute_list_bind_compute_pipeline(
            compute_list, _resource_manager.get_pipeline());
        _rd->compute_list_bind_uniform_set(
            compute_list, _resource_manager.get_image_uniform_set(), 0);
        _rd->compute_list_bind_uniform_set(
            compute_list, _resource_manager.get_settings_uniform_set(), 1);
        _rd->compute_list_bind_uniform_set(
            compute_list, _resource_manager.get_camera_uniform_set(), 2);
        _rd->compute_list_bind_uniform_set(
            compute_list, _resource_manager.get_scene_uniform_set(), 3);
        _rd->compute_list_set_push_constant(compute_list, push_constant_bytes,
                                            push_constant_bytes.size());
        _rd->compute_list_dispatch(compute_list, x_groups, y_groups, 1);
        _rd->compute_list_end();
        _frame_count++;

        _stats->set_samples(_stats->get_samples() +
                            _renderer_settings->get_samples_per_pixel());
    }

    void PTRenderer::_initialize_compute() {
        _rd = RenderingServer::get_singleton()->get_rendering_device();
        _resource_manager.initialize(
            _rd, PTUtils::get_project_relative_path("shaders/pathtracer.glsl"),
            get_render_width(), get_render_height());

        _scene_data_manager.initialize(_rd, &_resource_manager);
    }

    PackedByteArray PTRenderer::_get_push_constant_bytes() {
        auto push_constant = PackedFloat32Array();
        push_constant.push_back(float(get_render_width()));
        push_constant.push_back(float(get_render_height()));

        // Frame number
        push_constant.push_back(float(_frame_count));

        // Frame-based random seed
        push_constant.push_back(float(_frame_count * 1664525 + 1013904223));
        return push_constant.to_byte_array();
    }

    void PTRenderer::_update_settings_storage_buffer() {
        PackedByteArray settings_bytes = _renderer_settings->get_byte_array();
        _resource_manager.update_storage_buffer("settings", settings_bytes);
    }

    void PTRenderer::_update_camera_storage_buffer(Camera3D* camera) {
        PackedByteArray camera_bytes = PTUtils::get_camera_bytes(
            camera, get_render_width(), get_render_height());
        _resource_manager.update_storage_buffer("camera", camera_bytes);
    }

}  // namespace godot