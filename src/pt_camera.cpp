#include "pt_camera.h"
#include "pt_renderer.h"
#include "pt_utils.h"
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/engine.hpp>

namespace godot {

    // void PTCamera::_bind_methods() {
    // ClassDB::bind_method(D_METHOD("set_target_texture", "texture"),
    //                      &PTCamera::set_target_texture);
    // ClassDB::bind_method(D_METHOD("get_target_texture"),
    //                      &PTCamera::get_target_texture);

    // ClassDB::bind_method(D_METHOD("_setup_render"),
    //                      &PTCamera::_setup_render);

    // ClassDB::bind_method(D_METHOD("set_target_texture_path", "path"),
    //                      &PTCamera::set_target_texture_path);
    // ClassDB::bind_method(D_METHOD("get_target_texture_path"),
    //                      &PTCamera::get_target_texture_path);

    // ClassDB::bind_method(D_METHOD("_update_texture_rect_texture"),
    //                      &PTCamera::_update_texture_rect_texture);

    // // Export texture node path
    // ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "target_texture_path"),
    //              "set_target_texture_path", "get_target_texture_path");
    // }

    // void PTCamera::_ready() {
    //     texture_2d_rd = Ref<Texture2DRD>(memnew(Texture2DRD));

    //     RenderingServer::get_singleton()->connect(
    //         "frame_pre_draw", Callable(this, "_setup_render"));

    //     PTRenderer* renderer = _get_renderer();
    //     if (renderer) {
    //         renderer->connect("texture_changed",
    //                           Callable(this,
    //                           "_update_texture_rect_texture"));
    //     }
    // }

    // void PTCamera::set_target_texture(TextureRect* texture) {
    //     target_texture_path = texture->get_path();
    //     get_target_texture()->set_texture(texture_2d_rd);
    // }

    // TextureRect* PTCamera::get_target_texture() const {
    //     return get_node<TextureRect>(target_texture_path);
    // }
    // void PTCamera::_setup_render() {
    //     if (!is_visible_in_tree()) {
    //         return;
    //     }

    //     PTRenderer* renderer = _get_renderer();
    //     if (!renderer) {
    //         return;
    //     }

    //     TextureRect* target_texture = get_target_texture();

    //     if (target_texture) {
    //         uint32_t width = target_texture->get_size().x;
    //         uint32_t height = target_texture->get_size().y;

    //     } else {
    //         ERR_PRINT("PTCamera: No target texture set");
    //     }
    // }
    // void PTCamera::_update_texture_rect_texture(RID texture_rid) {
    //     TextureRect* target_texture = get_target_texture();
    //     if (target_texture) {
    //         target_texture->set_texture(texture_2d_rd);
    //     } else {
    //         ERR_PRINT("PTCamera: No target texture set");
    //     }

    //     PTRenderer* renderer = _get_renderer();
    //     if (!renderer) {
    //         return;
    //     }

    //     // Update the TextureRect with the renderer's output texture
    //     texture_2d_rd->set_texture_rd_rid(RID());
    //     texture_2d_rd->set_texture_rd_rid(texture_rid);
    // }

    // PTRenderer* PTCamera::_get_renderer() const {
    //     Object* object =
    //     Engine::get_singleton()->get_singleton("PTRenderer");

    //     if (!object) {
    //         ERR_PRINT("PTCamera: PTRenderer singleton not found");
    //         return nullptr;
    //     }

    //     PTRenderer* renderer = Object::cast_to<PTRenderer>(object);

    //     if (!renderer) {
    //         ERR_PRINT("PTCamera: Failed to cast to PTRenderer");
    //         return nullptr;
    //     }
    //     return renderer;
    // }
}  // namespace godot
