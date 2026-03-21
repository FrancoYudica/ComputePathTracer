#include "pt_types.h"
#include "pt_resource_manager.h"
#include "pt_scene_data_manager.h"

namespace godot {
    class PTScene : public RefCounted {
    private:
        PTResourceManager _rm;
        PTSceneDataManager _scene_data_manager;

    public:
        void initialize(RenderingDevice* rd);
        void update(Node* root, const Ref<PTRendererSettings>& settings,
                    Ref<PTRendererStats> stats);
        PTResourceManager* get_resource_manager() { return &_rm; }

        void cleanup();

    private:
        void write_buffers();
    };
}  // namespace godot