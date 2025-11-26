#ifndef PT_RESOURCE_MANAGER_H
#define PT_RESOURCE_MANAGER_H

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/rd_uniform.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/rendering_device.hpp>

namespace godot {

    class PTResourceManager : public RefCounted {
        GDCLASS(PTResourceManager, RefCounted)
    private:
        RenderingDevice* _rd;
        Camera3D* _camera;

        RID _shader;
        RID _pipeline;

        /* Textures */
        RID _output_texture;
        RID _accumulation_texture;
        RID _skybox_texture;
        RID _texture_array;

        /* Sampler */
        RID _default_sampler;

        /* Storage buffers */
        RID _settings_storage_buffer;
        RID _camera_storage_buffer;
        RID _scene_spheres_storage_buffer;
        RID _scene_triangles_storage_buffer;
        RID _scene_vertex_storage_buffer;
        RID _scene_materials_storage_buffer;
        RID _scene_bvh_storage_buffer;

        /* Uniform sets */
        RID _image_uniform_set;
        RID _settings_uniform_set;
        RID _camera_uniform_set;
        RID _scene_uniform_set;

        /* Uniform bindings */
        Ref<RDUniform> _output_image_uniform;
        Ref<RDUniform> _accumulation_image_uniform;
        Ref<RDUniform> _skybox_image_uniform;
        Ref<RDUniform> _settings_uniform;
        Ref<RDUniform> _camera_uniform;
        Ref<RDUniform> _scene_spheres_uniform;
        Ref<RDUniform> _scene_triangles_uniform;
        Ref<RDUniform> _scene_vertex_uniform;
        Ref<RDUniform> _scene_materials_uniform;
        Ref<RDUniform> _scene_bvh_uniform;
        Ref<RDUniform> _scene_textures_array_uniform;

        uint32_t _texture_array_resolution = 1024;
        uint32_t _texture_array_layers = 256;

    protected:
        static void _bind_methods();

    public:
        PTResourceManager();
        ~PTResourceManager();

        void initialize(RenderingDevice* rd, Camera3D* camera,
                        String shader_path, uint32_t width, uint32_t height);

        void cleanup();

        void resize(uint32_t width, uint32_t height);

        RID get_output_texture() const { return _output_texture; };
        RID get_accumulation_texture() const { return _accumulation_texture; };
        RID get_skybox_texture() const { return _skybox_texture; };
        RID get_pipeline() const { return _pipeline; };
        RID get_shader() const { return _shader; };
        RID get_settings_storage_buffer() const {
            return _settings_storage_buffer;
        }
        RID get_camera_storage_buffer() const { return _camera_storage_buffer; }
        RID get_scene_spheres_storage_buffer() const {
            return _scene_spheres_storage_buffer;
        }
        RID get_scene_triangles_storage_buffer() const {
            return _scene_triangles_storage_buffer;
        }
        RID get_scene_vertex_storage_buffer() const {
            return _scene_vertex_storage_buffer;
        }
        RID get_scene_materials_storage_buffer() const {
            return _scene_materials_storage_buffer;
        }
        RID get_scene_bvh_storage_buffer() const {
            return _scene_bvh_storage_buffer;
        }

        RID get_scene_texture_array() const { return _texture_array; }

        RID get_image_uniform_set() const { return _image_uniform_set; };
        RID get_settings_uniform_set() const { return _settings_uniform_set; };
        RID get_camera_uniform_set() const { return _camera_uniform_set; };
        RID get_scene_uniform_set() const { return _scene_uniform_set; };

        Ref<RDUniform> get_scene_texture_array_uniform() const {
            return _scene_textures_array_uniform;
        };

        uint32_t get_texture_array_resolution() const {
            return _texture_array_resolution;
        };

        void set_texture_array_resolution(uint32_t resolution) {
            _texture_array_resolution = resolution;
        };

        uint32_t get_texture_array_layers() const {
            return _texture_array_layers;
        };

        uint32_t get_texture_array_resolution() {
            return _texture_array_resolution;
        };

    private:
        // Creates size dependent textures
        void _create_viewport_textures(uint32_t width, uint32_t height);

        // Creates other resources, suchs as independent viewport size textures
        // and samplers
        void _create_resources();

        void _load_skybox_texture();

        RID _get_camera_skybox_texture(Camera3D* camera);

        void _create_uniforms();

        void _create_storage_buffers();

        void _create_uniform_sets();

        void _create_shader_and_pipeline(String shader_path);
    };
}  // namespace godot

#endif