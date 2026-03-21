#include "pt_scene.h"
#include "pt_utils.h"
#include "pt_renderer_stats.h"

namespace godot {
    void PTScene::initialize(RenderingDevice* rd) {
        _rm.initialize(
            rd, PTUtils::get_project_relative_path("shaders/pathtracer.glsl"),
            512, 512);

        _scene_data_manager.initialize(rd, &_rm);
    }

    void PTScene::update(Node* root, const Ref<PTRendererSettings>& settings,
                         Ref<PTRendererStats> stats) {
        _scene_data_manager.update_buffers(stats, root, settings);
    }

    void PTScene::cleanup() { _rm.cleanup(); }

}  // namespace godot