#ifndef PT_RESOURCE_MANAGER_H
#define PT_RESOURCE_MANAGER_H

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/rd_uniform.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <string>
#include <unordered_map>
namespace godot {

    class PTResourceManager {
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

        uint32_t _texture_array_resolution = 1024;
        uint32_t _texture_array_layers = 256;

        std::unordered_map<std::string, uint64_t> _storage_buffer_sizes;
        std::unordered_map<std::string, RID> _storage_buffers;

        bool _owns_skybox_texture = false;
        bool _should_update_scene_uniform_set = true;

    public:
        PTResourceManager();
        ~PTResourceManager();

        void initialize(RenderingDevice* rd, String shader_path, uint32_t width,
                        uint32_t height);

        void cleanup();

        void resize(uint32_t width, uint32_t height);
        void load_skybox_from_camera(Camera3D* camera);

        void flush_pending_updates();

        RID get_output_texture() const { return _output_texture; };
        RID get_accumulation_texture() const { return _accumulation_texture; };
        RID get_skybox_texture() const { return _skybox_texture; };
        RID get_pipeline() const { return _pipeline; };
        RID get_shader() const { return _shader; };

        RID get_storage_buffer(const std::string& name) {
            return _storage_buffers[name];
        }

        RID get_scene_texture_array() const { return _texture_array; }

        RID get_image_uniform_set() const { return _image_uniform_set; };
        RID get_settings_uniform_set() const { return _settings_uniform_set; };
        RID get_camera_uniform_set() const { return _camera_uniform_set; };
        RID get_scene_uniform_set() const { return _scene_uniform_set; };

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

        void update_storage_buffer(const std::string& name,
                                   const PackedByteArray& data,
                                   uint64_t offset = 0);

    private:
        // Creates size dependent textures
        void _create_viewport_textures(uint32_t width, uint32_t height);

        // Creates other resources, suchs as independent viewport size textures
        // and samplers
        void _create_resources();

        RID _get_camera_skybox_texture(Camera3D* camera);

        void _create_uniforms();

        void _create_storage_buffers();

        void _create_uniform_sets();

        void _create_shader_and_pipeline(String shader_path);

        void _create_storage_buffer(const std::string& name, uint64_t size);

        void _build_scene_uniform_set();
    };
}  // namespace godot

#endif