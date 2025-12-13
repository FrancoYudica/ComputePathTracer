#include "pt_resource_manager.h"

#include <godot_cpp/classes/cubemap.hpp>
#include <godot_cpp/classes/environment.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/panorama_sky_material.hpp>
#include <godot_cpp/classes/rd_sampler_state.hpp>
#include <godot_cpp/classes/rd_shader_file.hpp>
#include <godot_cpp/classes/rd_shader_spirv.hpp>
#include <godot_cpp/classes/rd_texture_format.hpp>
#include <godot_cpp/classes/rd_texture_view.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/sky.hpp>
#include <godot_cpp/classes/texture2d.hpp>

namespace godot {

    PTResourceManager::PTResourceManager() {}

    PTResourceManager::~PTResourceManager() {}

    static RID create_texture(RenderingDevice* rd, int width, int height,
                              RenderingDevice::DataFormat format,
                              uint32_t array_layers = 1,
                              RenderingDevice::TextureType texture_type =
                                  RenderingDevice::TEXTURE_TYPE_2D) {
        // Texture format
        Ref<RDTextureFormat> texture_format =
            Ref<RDTextureFormat>(memnew(RDTextureFormat));
        texture_format->set_texture_type(texture_type);
        texture_format->set_width(width);
        texture_format->set_height(height);
        texture_format->set_depth(1);
        texture_format->set_format(format);
        texture_format->set_array_layers(array_layers);
        texture_format->set_usage_bits(
            RenderingDevice::TextureUsageBits::TEXTURE_USAGE_SAMPLING_BIT |
            RenderingDevice::TextureUsageBits::TEXTURE_USAGE_STORAGE_BIT |
            RenderingDevice::TextureUsageBits::TEXTURE_USAGE_CAN_UPDATE_BIT);

        // Texture view
        Ref<RDTextureView> texture_view =
            Ref<RDTextureView>(memnew(RDTextureView));
        texture_view->set_format_override(format);

        // Creation
        RID texture = rd->texture_create(texture_format, texture_view);
        return texture;
    }

    void PTResourceManager::initialize(RenderingDevice* rd, String shader_path,
                                       uint32_t width, uint32_t height) {
        _rd = rd;
        _camera = nullptr;

        _create_shader_and_pipeline(shader_path);
        _create_viewport_textures(width, height);
        _create_resources();
        _create_uniforms();
        _create_storage_buffers();
        load_skybox_from_camera(nullptr);
        _create_uniform_sets();

        print_line("PTResourceManager initialized.");
    }
    void PTResourceManager::cleanup() {
        _rd->free_rid(_output_texture);
        _rd->free_rid(_accumulation_texture);
        _rd->free_rid(_texture_array);
        // _rd->free_rid(_skybox_texture); // TODO: Only free if it's the one
        // created here
        _rd->free_rid(_default_sampler);

        // Frees all storage buffers
        for (const auto& pair : _storage_buffers) {
            _rd->free_rid(pair.second);
        }

        _rd->free_rid(_image_uniform_set);
        _rd->free_rid(_settings_uniform_set);
        _rd->free_rid(_camera_uniform_set);
        _rd->free_rid(_scene_uniform_set);
        _rd->free_rid(_pipeline);
        _rd->free_rid(_shader);
    }

    void PTResourceManager::resize(uint32_t width, uint32_t height) {
        if (_output_texture.is_valid()) {
            _rd->free_rid(_output_texture);
        }
        if (_accumulation_texture.is_valid()) {
            _rd->free_rid(_accumulation_texture);
        }

        // Destroys _frees textures
        _output_image_uniform->clear_ids();
        _accumulation_image_uniform->clear_ids();

        _create_viewport_textures(width, height);

        _output_image_uniform->add_id(_output_texture);
        _accumulation_image_uniform->add_id(_accumulation_texture);

        if (_image_uniform_set.is_valid()) {
            _rd->free_rid(_image_uniform_set);
        }

        _image_uniform_set = _rd->uniform_set_create(
            {_output_image_uniform, _accumulation_image_uniform,
             _skybox_image_uniform},
            _shader, 0);
    }
    void PTResourceManager::load_skybox_from_camera(Camera3D* camera) {
        // Early return if camera hasn't changed
        if (camera != nullptr && camera == _camera) {
            return;
        }

        // Update camera reference
        _camera = camera;

        // Clean up previous skybox texture if we own it
        if (_skybox_texture.is_valid() && _owns_skybox_texture) {
            _rd->free_rid(_skybox_texture);
            _skybox_texture = RID();
            _owns_skybox_texture = false;
        }

        // Try to get skybox from camera, or create fallback
        if (camera) {
            _skybox_texture = _get_camera_skybox_texture(camera);
            _owns_skybox_texture = false;
        }

        // Create fallback black texture if no valid skybox
        if (!_skybox_texture.is_valid()) {
            _skybox_texture = create_texture(
                _rd, 1, 1, RenderingDevice::DATA_FORMAT_R8G8B8A8_UNORM);
            _owns_skybox_texture = true;
        }

        // Update uniform with new texture
        _skybox_image_uniform->clear_ids();
        _skybox_image_uniform->add_id(_default_sampler);
        _skybox_image_uniform->add_id(_skybox_texture);

        if (_image_uniform_set.is_valid()) {
            _rd->free_rid(_image_uniform_set);
        }

        _image_uniform_set = _rd->uniform_set_create(
            {_output_image_uniform, _accumulation_image_uniform,
             _skybox_image_uniform},
            _shader, 0);
    }

    void PTResourceManager::flush_pending_updates() {
        if (_should_update_scene_uniform_set) {
            _build_scene_uniform_set();
            _should_update_scene_uniform_set = false;
        }
    }

    void PTResourceManager::update_storage_buffer(const std::string& name,
                                                  const PackedByteArray& data,
                                                  uint64_t offset) {
        if (_storage_buffer_sizes.find(name) == _storage_buffer_sizes.end()) {
            print_line("Storage buffer " + String(name.c_str()) +
                       " not found.");
            return;
        }

        // Resize if needed
        if (_storage_buffer_sizes[name] < data.size()) {
            print_line("Resizing storage buffer " + String(name.c_str()) +
                       " from " +
                       String::num_uint64(_storage_buffer_sizes[name]) +
                       " to " + String::num_uint64(data.size()) + " bytes.");
            // Re-create buffer
            RID rid = _rd->storage_buffer_create(data.size(), data);

            // Frees previous buffer
            _rd->free_rid(_storage_buffers[name]);

            // Update maps
            _storage_buffers[name] = rid;
            _storage_buffer_sizes[name] = data.size();
            _should_update_scene_uniform_set = true;
        }

        // Update buffer data
        else {
            _rd->buffer_update(_storage_buffers[name], offset, data.size(),
                               data);
        }
    }

    void PTResourceManager::_create_viewport_textures(uint32_t width,
                                                      uint32_t height) {
        // LDR output texture
        _output_texture =
            create_texture(_rd, width, height,
                           RenderingDevice::DATA_FORMAT_R16G16B16A16_SFLOAT);

        // HDR accumulation texture
        _accumulation_texture =
            create_texture(_rd, width, height,
                           RenderingDevice::DATA_FORMAT_R32G32B32A32_SFLOAT);
    }

    void PTResourceManager::_create_resources() {
        // Texture array for scene textures
        _texture_array = create_texture(
            _rd, get_texture_array_resolution(), get_texture_array_resolution(),
            RenderingDevice::DATA_FORMAT_R8G8B8A8_UNORM,
            get_texture_array_layers(), RenderingDevice::TEXTURE_TYPE_2D_ARRAY);

        // Default sampler
        Ref<RDSamplerState> sampler_state =
            Ref<RDSamplerState>(memnew(RDSamplerState));
        sampler_state->set_min_filter(RenderingDevice::SAMPLER_FILTER_LINEAR);
        sampler_state->set_mag_filter(RenderingDevice::SAMPLER_FILTER_LINEAR);
        _default_sampler = _rd->sampler_create(sampler_state);
    }

    RID PTResourceManager::_get_camera_skybox_texture(Camera3D* camera) {
        // Skybox texture. Gotten from the environment later.
        if (camera == nullptr) {
            return RID();
        }

        Ref<Environment> env = camera->get_environment();
        if (!env.is_valid()) {
            return RID();
        }

        Ref<Sky> sky = env->get_sky();

        if (!sky.is_valid()) {
            return RID();
        }

        Ref<Material> material = sky->get_material();
        if (!material.is_valid()) {
            return RID();
        }

        // Panorama sky material holds skybox
        if (!material->is_class("PanoramaSkyMaterial")) {
            return RID();
        }

        Ref<PanoramaSkyMaterial> psm = material;
        Ref<Texture2D> cubemap = psm->get_panorama();

        if (!cubemap.is_valid()) {
            return RID();
        }

        // Convert engine Texture RID to an RD texture RID understood by the
        // RenderingDevice
        RID engine_tex_rid = cubemap->get_rid();
        RID rd_tex = RenderingServer::get_singleton()->texture_get_rd_texture(
            engine_tex_rid);
        return rd_tex;
    }

    void PTResourceManager::_create_uniforms() {
        // Image uniforms
        {
            // Output image
            _output_image_uniform = Ref<RDUniform>(memnew(RDUniform));
            _output_image_uniform->set_uniform_type(
                RenderingDevice::UNIFORM_TYPE_IMAGE);
            _output_image_uniform->set_binding(0);
            _output_image_uniform->add_id(_output_texture);

            // Accumulation image
            _accumulation_image_uniform = Ref<RDUniform>(memnew(RDUniform));
            _accumulation_image_uniform->set_uniform_type(
                RenderingDevice::UNIFORM_TYPE_IMAGE);
            _accumulation_image_uniform->set_binding(1);
            _accumulation_image_uniform->add_id(_accumulation_texture);

            // Skybox image
            _skybox_image_uniform = Ref<RDUniform>(memnew(RDUniform));
            _skybox_image_uniform->set_uniform_type(
                RenderingDevice::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE);
            _skybox_image_uniform->set_binding(2);
        }

        // Settings uniform
        {
            _settings_uniform = Ref<RDUniform>(memnew(RDUniform));
            _settings_uniform->set_uniform_type(
                RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
            _settings_uniform->set_binding(0);
        }

        // Camera uniform
        {
            _camera_uniform = Ref<RDUniform>(memnew(RDUniform));
            _camera_uniform->set_uniform_type(
                RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
            _camera_uniform->set_binding(0);
        }

        // Scene uniforms
        {
        }
    }

    void PTResourceManager::_create_storage_buffers() {
        // Settings uniform
        {
            _create_storage_buffer("settings", 512);
            _settings_uniform->add_id(_storage_buffers["settings"]);
        }

        // Camera uniform
        {
            // 2 matrices (view/proj)
            _create_storage_buffer("camera", 16 * 2 * 4);
            _camera_uniform->add_id(_storage_buffers["camera"]);
        }

        // Scene storage buffers
        // 16 MB initial size
        _create_storage_buffer("spheres", 16 * 1024 * 1024);
        // 32 MB initial size
        _create_storage_buffer("triangles", 32 * 1024 * 1024);
        // 32 MB initial size
        _create_storage_buffer("vertex", 32 * 1024 * 1024);
        // 16 MB initial size
        _create_storage_buffer("materials", 16 * 1024 * 1024);
        // 32 MB initial size
        _create_storage_buffer("bvh", 32 * 1024 * 1024);
    }
    void PTResourceManager::_create_uniform_sets() {
        if (!_image_uniform_set.is_valid()) {
            _image_uniform_set = _rd->uniform_set_create(
                {_output_image_uniform, _accumulation_image_uniform,
                 _skybox_image_uniform},
                _shader, 0);
        }

        _settings_uniform_set =
            _rd->uniform_set_create({_settings_uniform}, _shader, 1);

        _camera_uniform_set =
            _rd->uniform_set_create({_camera_uniform}, _shader, 2);

        _build_scene_uniform_set();
    }
    void PTResourceManager::_create_shader_and_pipeline(String shader_path) {
        // Shader
        {
            Ref<RDShaderFile> shader_file =
                ResourceLoader::get_singleton()->load(shader_path);

            if (shader_file.is_null()) {
                ERR_PRINT("Failed to load path tracer shader file. " +
                          shader_path);
                return;
            }
            Ref<RDShaderSPIRV> spirv = shader_file->get_spirv();
            if (spirv.is_null()) {
                ERR_PRINT("Shader file does not contain SPIR-V bytecode: " +
                          shader_path);
                return;
            }
            _shader = _rd->shader_create_from_spirv(spirv);
        }

        // Pipeline
        {
            _pipeline = _rd->compute_pipeline_create(_shader);
        }
    }

    void PTResourceManager::_create_storage_buffer(const std::string& name,
                                                   uint64_t size) {
        if (_storage_buffers.find(name) != _storage_buffers.end()) {
            print_line("Storage buffer " + String(name.c_str()) +
                       " already exists.");
            return;
        }

        PackedByteArray data = PackedByteArray();
        data.resize(size);

        RID rid = _rd->storage_buffer_create(size, data);
        _storage_buffers[name] = rid;
        _storage_buffer_sizes[name] = size;
    }
    void PTResourceManager::_build_scene_uniform_set() {
        if (_scene_uniform_set.is_valid()) {
            _rd->free_rid(_scene_uniform_set);
        }

        TypedArray<Ref<RDUniform>> scene_uniforms;
        std::array<std::string, 5> ordered_names = {
            "spheres", "triangles", "vertex", "materials", "bvh"};

        // Creates all the uniforms in order and sets it's storage buffer
        for (uint32_t i = 0; i < ordered_names.size(); ++i) {
            auto uniform = Ref<RDUniform>(memnew(RDUniform));
            uniform->set_uniform_type(
                RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
            uniform->set_binding(scene_uniforms.size());
            uniform->add_id(_storage_buffers[ordered_names[i]]);
            scene_uniforms.append(uniform);
        }

        // Creates texture array uniform
        auto texture_array_uniform = Ref<RDUniform>(memnew(RDUniform));
        texture_array_uniform->set_uniform_type(
            RenderingDevice::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE);
        texture_array_uniform->set_binding(5);
        texture_array_uniform->add_id(_default_sampler);
        texture_array_uniform->add_id(_texture_array);

        scene_uniforms.append(texture_array_uniform);

        // Finally, create uniform set
        _scene_uniform_set =
            _rd->uniform_set_create(scene_uniforms, _shader, 3);
    }
}  // namespace godot