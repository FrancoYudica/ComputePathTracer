#ifndef PT_RENDERER_H
#define PT_RENDERER_H

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/core/math.hpp>
#include <unordered_map>
#include <vector>
#include <stack>
#include "pt_analytical_geometry.h"
#include "pt_types.h"
#include "pt_resource_manager.h"
#include "pt_renderer_settings.h"
#include "pt_scene_data_manager.h"
#include "pt_renderer_stats.h"
#include "pt_scene.h"
namespace godot {
    enum RenderTaskType { ONE_SHOT, CONTINUOUS };

    enum RenderTaskStatus {
        STATUS_CREATED,
        STATUS_RENDERING,
        STATUS_PAUSED,
        STATUS_COMPLETED
    };

    class PTRenderTask : public RefCounted {
    public:
        RenderTaskType type;
        RenderTaskStatus status;
        Ref<PTScene> scene;
        Camera3D* camera;
        Ref<PTRendererSettings> settings;
        Ref<PTRendererStats> stats;
        uint32_t frame_count;
        bool should_update_scene;
        bool should_clear_textures;

        /**
         * The scene and it's resources will be cleaned up when the task is
         * destroyed. Note that it also includes the output texture, so the
         * user must keep this reference alive if wants to keep the output
         */
        ~PTRenderTask() { scene->cleanup(); }
    };

    class PTRenderer : public Node {
        GDCLASS(PTRenderer, Node)
    private:
        RenderingDevice* _rd;
        std::vector<Ref<PTRenderTask>> _tasks;
        bool _initialized = false;

    protected:
        static void _bind_methods();

    public:
        PTRenderer() {}
        ~PTRenderer() {}
        void init();
        void destroy();

        Ref<PTRenderTask> submit_one_shot_task(
            Camera3D* camera, Ref<PTRendererSettings> settings);

        Ref<PTRenderTask> submit_continuous_task(
            Camera3D* camera, Ref<PTRendererSettings> settings);

        void task_pause(Ref<PTRenderTask> task);
        void task_resume(Ref<PTRenderTask> task);
        void task_kill(Ref<PTRenderTask> task);
        void task_clear_progress(Ref<PTRenderTask> task);
        void task_reload_scene(Ref<PTRenderTask> task);
        Ref<PTRendererStats> task_get_stats(Ref<PTRenderTask> task);
        RID task_get_output(Ref<PTRenderTask> task);

    private:
        Ref<PTRenderTask> _create_render_task(Camera3D* camera,
                                              Ref<PTRendererSettings> settings,
                                              RenderTaskType type);
        PackedByteArray _get_push_constant_bytes(uint32_t w, uint32_t h,
                                                 uint32_t frames);

        void _queue_clear_task(Ref<PTRenderTask> task);

        void _render_task(Ref<PTRenderTask> task);

        void _resize_task(Ref<PTRenderTask> task, uint32_t width,
                          uint32_t height);

        void _process_tasks();
    };

}  // namespace godot

#endif