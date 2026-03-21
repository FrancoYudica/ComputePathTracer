#include "pt_renderer.h"
#include "pt_utils.h"
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/viewport.hpp>

namespace godot {

    void PTRenderer::_bind_methods() {
        ClassDB::bind_method(D_METHOD("init"), &PTRenderer::init);
        ClassDB::bind_method(D_METHOD("destroy"), &PTRenderer::destroy);

        ClassDB::bind_method(D_METHOD("queue_clear"), &PTRenderer::queue_clear);
        ClassDB::bind_method(D_METHOD("update_scene"),
                             &PTRenderer::update_scene);

        ClassDB::bind_method(
            D_METHOD("set_renderer_settings", "renderer_settings"),
            &PTRenderer::set_renderer_settings);
        ClassDB::bind_method(D_METHOD("get_renderer_settings"),
                             &PTRenderer::get_renderer_settings);
        ClassDB::bind_method(D_METHOD("get_stats"), &PTRenderer::get_stats);

        ClassDB::bind_method(D_METHOD("_resize", "width", "height"),
                             &PTRenderer::_resize);

        ClassDB::bind_method(
            D_METHOD("_resize_task", "task", "width", "height"),
            &PTRenderer::_resize_task);

        ClassDB::bind_method(D_METHOD("pause_task", "task"),
                             &PTRenderer::pause_task);
        ClassDB::bind_method(D_METHOD("resume_task", "task"),
                             &PTRenderer::resume_task);
        ClassDB::bind_method(D_METHOD("kill_task", "task"),
                             &PTRenderer::kill_task);
        ClassDB::bind_method(D_METHOD("task_clear_progress", "task"),
                             &PTRenderer::task_clear_progress);
        ClassDB::bind_method(D_METHOD("task_get_stats", "task"),
                             &PTRenderer::task_get_stats);
        ClassDB::bind_method(D_METHOD("get_task_output", "task"),
                             &PTRenderer::get_task_output);

        ClassDB::bind_method(
            D_METHOD("submit_one_shot_task", "camera", "settings"),
            &PTRenderer::submit_one_shot_task);

        ClassDB::bind_method(
            D_METHOD("submit_continuous_task", "camera", "settings"),
            &PTRenderer::submit_continuous_task);

        ClassDB::bind_method(D_METHOD("_process_tasks"),
                             &PTRenderer::_process_tasks);

        ClassDB::bind_method(D_METHOD("queue_clear_task", "task"),
                             &PTRenderer::queue_clear_task);

        ADD_SIGNAL(MethodInfo("task_completed",
                              PropertyInfo(Variant::OBJECT, "task")));

        // Export in inspector
        ADD_PROPERTY(
            PropertyInfo(Variant::OBJECT, "renderer_settings",
                         PROPERTY_HINT_RESOURCE_TYPE, "PTRendererSettings"),
            "set_renderer_settings", "get_renderer_settings");

        ADD_SIGNAL(MethodInfo("texture_changed",
                              PropertyInfo(Variant::OBJECT, "task")));
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

        _scene.instantiate();

        _rd = RenderingServer::get_singleton()->get_rendering_device();
        _scene->initialize(_rd);

        update_scene();

        _renderer_settings->connect("changed", Callable(this, "queue_clear"));

        // The task processing is done on pre draw
        RenderingServer::get_singleton()->connect(
            "frame_pre_draw", Callable(this, "_process_tasks"));

        _initialized = true;
    }

    void PTRenderer::destroy() { _cleanup(); }

    void PTRenderer::pause_task(Ref<PTRenderTask> task) {
        if (task.is_valid()) task->status = STATUS_PAUSED;
    }

    void PTRenderer::resume_task(Ref<PTRenderTask> task) {
        if (task.is_valid()) task->status = STATUS_RENDERING;
    }

    void PTRenderer::kill_task(Ref<PTRenderTask> task) {
        if (task.is_null()) return;

        for (int i = 0; i < _tasks.size(); ++i) {
            if (_tasks[i] == task) {
                _tasks.erase(_tasks.begin() + i);
                break;
            }
        }
    }

    void PTRenderer::task_clear_progress(Ref<PTRenderTask> task) {
        if (task.is_valid()) {
            task->should_clear_textures = true;
        }
    }

    Ref<PTRendererStats> PTRenderer::task_get_stats(Ref<PTRenderTask> task) {
        if (task.is_null()) return Ref<PTRendererStats>();
        return task->stats;
    }

    RID PTRenderer::get_task_output(Ref<PTRenderTask> task) {
        if (task.is_null()) return RID();

        if (task->type == RenderTaskType::ONE_SHOT) {
            // Gets the output texture if the task is completed and kills the
            // task
            if (task->status == STATUS_COMPLETED) {
                RID output_rid = task->scene->get_resource_manager()
                                     ->extract_output_texture();

                kill_task(task);
                return output_rid;
            }

            return RID();
        }

        else if (task->type == RenderTaskType::CONTINUOUS) {
            return task->scene->get_resource_manager()->get_output_texture();
        }

        return RID();
    }

    void clear_task(Ref<PTRenderTask> task) {
        if (task.is_valid()) {
            task->should_clear_textures = true;
        }
    }

    Ref<PTRenderTask> PTRenderer::_create_render_task(
        Camera3D* camera, Ref<PTRendererSettings> settings,
        RenderTaskType type) {
        Ref<PTRenderTask> task;
        task.instantiate();
        task->status = RenderTaskStatus::STATUS_CREATED;
        task->type = type;
        task->camera = camera;
        task->settings = settings;
        task->stats.instantiate();
        task->scene.instantiate();
        task->scene->initialize(_rd);
        task->frame_count = 1;
        task->should_update_scene = true;
        task->should_clear_textures = true;
        emit_signal("texture_changed", task);
        task->settings->connect("changed",
                                Callable(this, "queue_clear_task").bind(task));
        return task;
    }

    Ref<PTRenderTask> PTRenderer::submit_one_shot_task(
        Camera3D* camera, Ref<PTRendererSettings> settings) {
        Ref<PTRenderTask> task =
            _create_render_task(camera, settings, RenderTaskType::ONE_SHOT);
        _tasks.push_back(task);
        return task;
    }

    Ref<PTRenderTask> PTRenderer::submit_continuous_task(
        Camera3D* camera, Ref<PTRendererSettings> settings) {
        Ref<PTRenderTask> task =
            _create_render_task(camera, settings, RenderTaskType::CONTINUOUS);
        _tasks.push_back(task);
        return task;
    }

    RID PTRenderer::get_texture_rid() const {
        PTResourceManager* rm = _scene->get_resource_manager();
        return rm->get_output_texture();
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
    void PTRenderer::queue_clear_task(Ref<PTRenderTask> task) {
        task->should_clear_textures = true;
    }

    void PTRenderer::update_scene() {
        _update_scene = true;
        queue_clear();
    }

    void PTRenderer::_process_tasks() {
        for (int i = 0; i < _tasks.size(); ++i) {
            Ref<PTRenderTask> task = _tasks[i];

            if (task.is_null() || task->status == STATUS_PAUSED ||
                task->status == STATUS_COMPLETED)
                continue;

            task->scene->get_resource_manager()->begin_frame();

            // Start rendering if it's new
            if (task->status == STATUS_CREATED) task->status = STATUS_RENDERING;

            _render_task(task);

            if (task->type == RenderTaskType::ONE_SHOT) {
                task->status = STATUS_COMPLETED;
                // Notify the user
                emit_signal("task_completed", task);
            }
        }
    }

    void PTRenderer::_cleanup() { _scene->cleanup(); }

    void PTRenderer::_resize(uint32_t width, uint32_t height) {
        PTResourceManager* rm = _scene->get_resource_manager();
        rm->resize(get_render_width(), get_render_height());
        emit_signal("texture_changed", get_texture_rid());
        queue_clear();
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
            _frame_count = 1;
            _stats->set_samples(0);
            _clear_buffer = false;
        }

        if (_update_scene) {
            _stats->reset();
            Node* root = Node::cast_to<Node>(camera->get_viewport());
            _scene->update(root, _renderer_settings, _stats);
            _update_scene = false;
        }

        PTResourceManager* rm = _scene->get_resource_manager();
        rm->load_skybox_from_camera(camera);

        PackedByteArray push_constant_bytes = _get_push_constant_bytes(
            get_render_width(), get_render_height(), _frame_count);

        // Assuming workgroup size of 8x8. Do ceiling division to cover the
        // entire texture.

        uint32_t x_groups =
            static_cast<uint32_t>(ceil(get_render_width() / 8.0));
        uint32_t y_groups =
            static_cast<uint32_t>(ceil(get_render_height() / 8.0));
        {
            // Settings
            PackedByteArray settings_bytes =
                _renderer_settings->get_byte_array();
            rm->update_storage_buffer("settings", settings_bytes);

            // Camera
            PackedByteArray camera_bytes = PTUtils::get_camera_byte_array(
                camera, get_render_width(), get_render_height());
            rm->update_storage_buffer("camera", camera_bytes);
        }

        // Updates resource manager
        rm->flush_pending_updates();

        int64_t compute_list = _rd->compute_list_begin();
        _rd->compute_list_bind_compute_pipeline(compute_list,
                                                rm->get_pipeline());
        _rd->compute_list_bind_uniform_set(compute_list,
                                           rm->get_image_uniform_set(), 0);
        _rd->compute_list_bind_uniform_set(compute_list,
                                           rm->get_settings_uniform_set(), 1);
        _rd->compute_list_bind_uniform_set(compute_list,
                                           rm->get_camera_uniform_set(), 2);
        _rd->compute_list_bind_uniform_set(compute_list,
                                           rm->get_scene_uniform_set(), 3);
        _rd->compute_list_set_push_constant(compute_list, push_constant_bytes,
                                            push_constant_bytes.size());
        _rd->compute_list_dispatch(compute_list, x_groups, y_groups, 1);
        _rd->compute_list_end();
        _frame_count++;

        _stats->set_samples(_stats->get_samples() +
                            _renderer_settings->get_samples_per_pixel());
    }

    PackedByteArray PTRenderer::_get_push_constant_bytes(uint32_t w, uint32_t h,
                                                         uint32_t frames) {
        auto push_constant = PackedFloat32Array();
        push_constant.push_back(float(w));
        push_constant.push_back(float(h));

        // Frame number
        push_constant.push_back(float(frames));

        // Frame-based random seed
        push_constant.push_back(float(frames * 1664525 + 1013904223));
        return push_constant.to_byte_array();
    }

    void PTRenderer::_render_task(Ref<PTRenderTask> task) {
        Camera3D* camera = task->camera;
        Ref<PTScene> scene = task->scene;
        Ref<PTRendererSettings> settings = task->settings;
        Ref<PTRendererStats> stats = task->stats;
        PTResourceManager* rm = scene->get_resource_manager();

        Rect2 rect = camera->get_viewport()->get_visible_rect();

        int viewport_width = rect.size.x;
        int viewport_height = rect.size.y;

        uint32_t current_width = rm->get_textures_width();
        uint32_t current_height = rm->get_textures_height();

        // Delays resize to avoid issues when called during rendering
        if (current_width != viewport_width ||
            current_height != viewport_height) {
            _resize_task(task, viewport_width, viewport_height);
            current_width = viewport_width;
            current_height = viewport_height;
            task->should_clear_textures = true;
        }

        if (task->should_clear_textures) {
            // This clears the buffer
            task->frame_count = 1;
            stats->set_samples(0);

            task->should_clear_textures = false;
        }

        if (task->should_update_scene) {
            stats->reset();
            Node* root = Node::cast_to<Node>(camera->get_viewport());
            scene->update(root, settings, stats);
            task->should_update_scene = false;
        }

        rm->load_skybox_from_camera(camera);

        PackedByteArray push_constant_bytes = _get_push_constant_bytes(
            current_width, current_height, task->frame_count);

        // Do ceiling division to cover the entire texture.
        uint32_t x_groups = static_cast<uint32_t>(ceil(current_width / 8.0));
        uint32_t y_groups = static_cast<uint32_t>(ceil(current_height / 8.0));

        // Buffer updates
        {
            // Settings
            PackedByteArray settings_bytes = settings->get_byte_array();
            rm->update_storage_buffer("settings", settings_bytes);

            // Camera
            PackedByteArray camera_bytes = PTUtils::get_camera_byte_array(
                camera, current_width, current_height);
            rm->update_storage_buffer("camera", camera_bytes);
        }

        // Updates resource manager
        rm->flush_pending_updates();

        int64_t compute_list = _rd->compute_list_begin();
        _rd->compute_list_bind_compute_pipeline(compute_list,
                                                rm->get_pipeline());
        _rd->compute_list_bind_uniform_set(compute_list,
                                           rm->get_image_uniform_set(), 0);
        _rd->compute_list_bind_uniform_set(compute_list,
                                           rm->get_settings_uniform_set(), 1);
        _rd->compute_list_bind_uniform_set(compute_list,
                                           rm->get_camera_uniform_set(), 2);
        _rd->compute_list_bind_uniform_set(compute_list,
                                           rm->get_scene_uniform_set(), 3);
        _rd->compute_list_set_push_constant(compute_list, push_constant_bytes,
                                            push_constant_bytes.size());
        _rd->compute_list_dispatch(compute_list, x_groups, y_groups, 1);
        _rd->compute_list_end();

        task->frame_count++;

        stats->set_samples(stats->get_samples() +
                           settings->get_samples_per_pixel());
    }

    void PTRenderer::_resize_task(Ref<PTRenderTask> task, uint32_t width,
                                  uint32_t height) {
        PTResourceManager* rm = task->scene->get_resource_manager();
        rm->resize(width, height);
        emit_signal("texture_changed", task);
    }

}  // namespace godot