#include "pt_utils.h"

namespace godot {

    godot::String PTUtils::get_project_relative_path(
        const godot::String& addon_relative_path) {
        static const godot::String base_path = "res://addons/pathtracer/";
        return base_path + addon_relative_path;
    }

    PackedFloat32Array PTUtils::transform3d_to_float32_array(
        const Transform3D& transform) {
        auto origin = transform.origin;

        PackedFloat32Array arr;
        arr.push_back(transform.basis[0][0]);
        arr.push_back(transform.basis[1][0]);
        arr.push_back(transform.basis[2][0]);
        arr.push_back(0.0f);

        arr.push_back(transform.basis[0][1]);
        arr.push_back(transform.basis[1][1]);
        arr.push_back(transform.basis[2][1]);
        arr.push_back(0.0f);

        arr.push_back(transform.basis[0][2]);
        arr.push_back(transform.basis[1][2]);
        arr.push_back(transform.basis[2][2]);
        arr.push_back(0.0f);

        arr.push_back(transform.origin.x);
        arr.push_back(transform.origin.y);
        arr.push_back(transform.origin.z);
        arr.push_back(1.0f);

        return arr;
    }
    PackedByteArray PTUtils::get_camera_byte_array(Camera3D* camera,
                                                   uint32_t width,
                                                   uint32_t height) {
        auto view = camera->get_global_transform().affine_inverse();
        auto projection = camera->get_camera_projection();

        // Re-calculates projection with scene aspect ratio
        auto corrected_projection = Projection::create_perspective(
            camera->get_fov(),
            static_cast<float>(width) / static_cast<float>(height),
            projection.get_z_near(), projection.get_z_far());

        PackedFloat32Array view_floats = transform3d_to_float32_array(view);
        PackedFloat32Array projection_floats = PackedFloat32Array();
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                projection_floats.push_back(corrected_projection[i][j]);
            }
        }

        PackedFloat32Array camera_data;
        camera_data.append_array(view_floats);
        camera_data.append_array(projection_floats);

        return camera_data.to_byte_array();
    }

}  // namespace godot