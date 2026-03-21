#include "pt_renderer.h"
#include "pt_utils.h"
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/viewport.hpp>

namespace godot {

    void PTRenderer::_bind_methods() {
        ClassDB::bind_method(D_METHOD("init"), &PTRenderer::init);
        ClassDB::bind_method(D_METHOD("destroy"), &PTRenderer::destroy);

        ClassDB::bind_method(
            D_METHOD("_resize_task", "task", "width", "height"),
            &PTRenderer::_resize_task);

        ClassDB::bind_method(D_METHOD("task_pause", "task"),
                             &PTRenderer::task_pause);
        ClassDB::bind_method(D_METHOD("task_resume", "task"),
                             &PTRenderer::task_resume);
        ClassDB::bind_method(D_METHOD("task_kill", "task"),
                             &PTRenderer::task_kill);
        ClassDB::bind_method(D_METHOD("task_clear_progress", "task"),
                             &PTRenderer::task_clear_progress);
        ClassDB::bind_method(D_METHOD("task_reload_scene", "task"),
                             &PTRenderer::task_reload_scene);
        ClassDB::bind_method(D_METHOD("task_get_stats", "task"),
                             &PTRenderer::task_get_stats);
        ClassDB::bind_method(D_METHOD("task_get_output", "task"),
                             &PTRenderer::task_get_output);

        ClassDB::bind_method(
            D_METHOD("submit_one_shot_task", "camera", "settings"),
            &PTRenderer::submit_one_shot_task);

        ClassDB::bind_method(
            D_METHOD("submit_continuous_task", "camera", "settings"),
            &PTRenderer::submit_continuous_task);

        ClassDB::bind_method(D_METHOD("_process_tasks"),
                             &PTRenderer::_process_tasks);

        ClassDB::bind_method(D_METHOD("_queue_clear_task", "task"),
                             &PTRenderer::_queue_clear_task);

        ADD_SIGNAL(MethodInfo("task_completed",
                              PropertyInfo(Variant::OBJECT, "task")));

        ADD_SIGNAL(MethodInfo("texture_changed",
                              PropertyInfo(Variant::OBJECT, "task")));
    }

    void PTRenderer::init() {
        if (_initialized) {
            ERR_PRINT("PTRenderer is already initialized.");
            return;
        }

        RenderingServer* rs = RenderingServer::get_singleton();
        _rd = rs->get_rendering_device();
        rs->connect("frame_pre_draw", Callable(this, "_process_tasks"));

        _initialized = true;
    }

    void PTRenderer::destroy() {
        for (const auto& task : _tasks) {
            task->scene->cleanup();
        }
        _tasks.clear();
    }

    void PTRenderer::task_pause(Ref<PTRenderTask> task) {
        if (task.is_valid()) task->status = STATUS_PAUSED;
    }

    void PTRenderer::task_resume(Ref<PTRenderTask> task) {
        if (task.is_valid()) task->status = STATUS_RENDERING;
    }

    void PTRenderer::task_kill(Ref<PTRenderTask> task) {
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

    void PTRenderer::task_reload_scene(Ref<PTRenderTask> task) {
        if (task.is_valid()) {
            task->should_clear_textures = true;
            task->should_update_scene = true;
        }
    }

    Ref<PTRendererStats> PTRenderer::task_get_stats(Ref<PTRenderTask> task) {
        if (task.is_null()) return Ref<PTRendererStats>();
        return task->stats;
    }

    RID PTRenderer::task_get_output(Ref<PTRenderTask> task) {
        if (task.is_null()) return RID();

        if (task->type == RenderTaskType::ONE_SHOT) {
            // Gets the output texture if the task is completed and kills the
            // task
            if (task->status == STATUS_COMPLETED) {
                RID output_rid = task->scene->get_resource_manager()
                                     ->extract_output_texture();

                task_kill(task);
                return output_rid;
            }

            return RID();
        }

        else if (task->type == RenderTaskType::CONTINUOUS) {
            return task->scene->get_resource_manager()->get_output_texture();
        }

        return RID();
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

        task->settings->connect("changed",
                                Callable(this, "_queue_clear_task").bind(task));
        _tasks.push_back(task);
        return task;
    }

    void PTRenderer::_queue_clear_task(Ref<PTRenderTask> task) {
        task->should_clear_textures = true;
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

        int viewport_width = rect.size.x * settings->get_render_scale();
        int viewport_height = rect.size.y * settings->get_render_scale();

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