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

namespace godot {

    class PTRenderer : public Node {
        GDCLASS(PTRenderer, Node)
    private:
        Ref<PTRendererSettings> _renderer_settings;
        RenderingDevice* _rd;

        PTResourceManager _resource_manager;
        PTSceneDataManager _scene_data_manager;

        uint32_t _frame_count = 1;
        bool _clear_buffer = false;
        bool _update_scene = false;

        uint32_t _last_render_width = 800;
        uint32_t _last_render_height = 600;
        uint32_t _viewport_width = 800;
        uint32_t _viewport_height = 600;

        Ref<PTRendererStats> _stats;

        bool _initialized = false;

    protected:
        static void _bind_methods();

    public:
        PTRenderer() {}
        ~PTRenderer() {}
        void init();
        void destroy();

        RID get_texture_rid() const;
        uint32_t get_render_width() const;
        uint32_t get_render_height() const;
        Ref<PTRendererSettings> get_renderer_settings() const;

        void set_renderer_settings(const Ref<PTRendererSettings>& settings);
        void queue_clear();
        void update_scene();
        void draw(Camera3D* camera, uint32_t width, uint32_t height);
        Ref<PTRendererStats> get_stats() const { return _stats; }

    private:
        void _initialize_compute();
        void _clear_accumulated_buffer();
        PackedByteArray _get_push_constant_bytes();
        void _update_settings_storage_buffer();
        void _update_camera_storage_buffer(Camera3D* camera);
        void _resize(uint32_t width, uint32_t height);
        void _cleanup();
    };

}  // namespace godot

#endif