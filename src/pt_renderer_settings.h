#ifndef PT_RENDERER_SETTINGS_H
#define PT_RENDERER_SETTINGS_H

#include <godot_cpp/classes/resource.hpp>

namespace godot {

    enum PTRenderMode {
        RENDER_MODE_PATH_TRACE = 0,
        RENDER_MODE_RAY_CAST = 1,
        RENDER_MODE_BVH = 2,
        RENDER_MODE_NORMALS = 3,
        RENDER_MODE_DEPTH = 4,
        RENDER_MODE_UV = 5,
        RENDER_MODE_METAL = 6,
        RENDER_MODE_ROUGHNESS = 7,
        RENDER_MODE_EMISSION = 8,
    };

    class PTRendererSettings : public Resource {
        GDCLASS(PTRendererSettings, Resource)
    private:
        uint32_t samples_per_pixel = 1;
        uint32_t max_bounces = 6;
        float environment_energy = 0.25;
        float camera_aperture = 0.0f;
        float camera_focus_distance = 10.0f;
        PTRenderMode render_mode = RENDER_MODE_PATH_TRACE;
        float render_scale = 1.0f;

        uint32_t bvh_max_depth = 32;
        uint32_t bvh_max_triangles_per_leaf = 4;
        uint32_t bvh_sah_bins = 32;

        float debug_bvh_box_intersections_threshold = 100.0f;
        float debug_bvh_triangle_intersections_threshold = 100.0f;

    protected:
        static void _bind_methods();

    public:
        PTRendererSettings() {}
        ~PTRendererSettings() {}

        uint32_t get_samples_per_pixel() const { return samples_per_pixel; }
        uint32_t get_max_bounces() const { return max_bounces; }
        float get_environment_energy() const { return environment_energy; }
        float get_camera_aperture() const { return camera_aperture; }
        float get_camera_focus_distance() const {
            return camera_focus_distance;
        }
        PTRenderMode get_render_mode() const { return render_mode; }
        float get_render_scale() const { return render_scale; }
        uint32_t get_bvh_max_depth() const { return bvh_max_depth; }
        uint32_t get_bvh_max_triangles_per_leaf() const {
            return bvh_max_triangles_per_leaf;
        }
        uint32_t get_bvh_sah_bins() const { return bvh_sah_bins; }
        float get_debug_bvh_box_intersections_threshold() const {
            return debug_bvh_box_intersections_threshold;
        }
        float get_debug_bvh_triangle_intersections_threshold() const {
            return debug_bvh_triangle_intersections_threshold;
        }

        void set_samples_per_pixel(uint32_t p_spp);
        void set_max_bounces(uint32_t p_bounces);
        void set_environment_energy(float p_energy);
        void set_camera_aperture(float p_aperture);
        void set_camera_focus_distance(float p_distance);
        void set_render_mode(PTRenderMode p_mode);
        void set_render_scale(float p_scale);
        void set_bvh_max_depth(uint32_t p_depth);
        void set_bvh_max_triangles_per_leaf(uint32_t p_triangles);
        void set_bvh_sah_bins(uint32_t p_bins);
        void set_debug_bvh_box_intersections_threshold(float p_threshold);
        void set_debug_bvh_triangle_intersections_threshold(float p_threshold);

        PackedByteArray get_byte_array() const;

    private:
    };

}  // namespace godot

VARIANT_ENUM_CAST(godot::PTRenderMode);

#endif