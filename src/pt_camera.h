#ifndef PT_CAMERA_H
#define PT_CAMERA_H

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/texture2drd.hpp>
#include "pt_renderer.h"

namespace godot {

    class PTCamera : public Camera3D {
        GDCLASS(PTCamera, Camera3D)
    private:
        NodePath target_texture_path;
        Ref<Texture2DRD> texture_2d_rd = nullptr;

    protected:
        static void _bind_methods();

    public:
        PTCamera() {}
        ~PTCamera() {}

        void _ready() override;

        void set_target_texture_path(const NodePath& path) {
            target_texture_path = path;
        }

        NodePath get_target_texture_path() const { return target_texture_path; }

        void set_target_texture(TextureRect* texture);
        TextureRect* get_target_texture() const;

    private:
        void _setup_render();
        void _update_texture_rect_texture(RID texture_rid);
        PTRenderer* _get_renderer() const;
    };

}  // namespace godot

#endif