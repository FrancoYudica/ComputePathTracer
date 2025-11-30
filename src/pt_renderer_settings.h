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
        RENDER_MODE_UV = 5
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

    protected:
        static void _bind_methods();

    public:
        PTRendererSettings() {}
        ~PTRendererSettings() {}

        uint32_t get_samples_per_pixel() const { return samples_per_pixel; }
        void set_samples_per_pixel(uint32_t p_spp) {
            samples_per_pixel = p_spp;
        }
        uint32_t get_max_bounces() const { return max_bounces; }
        void set_max_bounces(uint32_t p_bounces) { max_bounces = p_bounces; }
        float get_environment_energy() const { return environment_energy; }
        void set_environment_energy(float p_energy) {
            environment_energy = p_energy;
        }
        float get_camera_aperture() const { return camera_aperture; }
        void set_camera_aperture(float p_aperture) {
            camera_aperture = p_aperture;
        }
        float get_camera_focus_distance() const {
            return camera_focus_distance;
        }
        void set_camera_focus_distance(float p_distance) {
            camera_focus_distance = p_distance;
        }

        PTRenderMode get_render_mode() const { return render_mode; }
        void set_render_mode(PTRenderMode p_mode) { render_mode = p_mode; }

        float get_render_scale() const { return render_scale; }
        void set_render_scale(float p_scale) { render_scale = p_scale; }

    private:
    };

}  // namespace godot

VARIANT_ENUM_CAST(godot::PTRenderMode);

#endif