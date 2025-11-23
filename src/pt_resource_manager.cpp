#include "pt_resource_manager.hpp"

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
    void PTResourceManager::_bind_methods() {
        ClassDB::bind_method(D_METHOD("initialize", "rd", "camera"),
                             &PTResourceManager::initialize);
        ClassDB::bind_method(D_METHOD("resize", "width", "height"),
                             &PTResourceManager::resize);
        ClassDB::bind_method(D_METHOD("get_output_texture"),
                             &PTResourceManager::get_output_texture);
        ClassDB::bind_method(D_METHOD("get_accumulation_texture"),
                             &PTResourceManager::get_accumulation_texture);
        ClassDB::bind_method(D_METHOD("get_skybox_texture"),
                             &PTResourceManager::get_skybox_texture);
        ClassDB::bind_method(D_METHOD("get_pipeline"),
                             &PTResourceManager::get_pipeline);
        ClassDB::bind_method(D_METHOD("get_shader"),
                             &PTResourceManager::get_shader);
        ClassDB::bind_method(D_METHOD("get_settings_storage_buffer"),
                             &PTResourceManager::get_settings_storage_buffer);
        ClassDB::bind_method(D_METHOD("get_camera_storage_buffer"),
                             &PTResourceManager::get_camera_storage_buffer);
        ClassDB::bind_method(
            D_METHOD("get_scene_spheres_storage_buffer"),
            &PTResourceManager::get_scene_spheres_storage_buffer);
        ClassDB::bind_method(
            D_METHOD("get_scene_triangles_storage_buffer"),
            &PTResourceManager::get_scene_triangles_storage_buffer);
        ClassDB::bind_method(
            D_METHOD("get_scene_vertex_storage_buffer"),
            &PTResourceManager::get_scene_vertex_storage_buffer);
        ClassDB::bind_method(
            D_METHOD("get_scene_materials_storage_buffer"),
            &PTResourceManager::get_scene_materials_storage_buffer);
        ClassDB::bind_method(D_METHOD("get_scene_bvh_storage_buffer"),
                             &PTResourceManager::get_scene_bvh_storage_buffer);
        ClassDB::bind_method(D_METHOD("get_image_uniform_set"),
                             &PTResourceManager::get_image_uniform_set);
        ClassDB::bind_method(D_METHOD("get_settings_uniform_set"),
                             &PTResourceManager::get_settings_uniform_set);
        ClassDB::bind_method(D_METHOD("get_camera_uniform_set"),
                             &PTResourceManager::get_camera_uniform_set);
        ClassDB::bind_method(D_METHOD("get_scene_uniform_set"),
                             &PTResourceManager::get_scene_uniform_set);
    }

    PTResourceManager::PTResourceManager() {}

    PTResourceManager::~PTResourceManager() {
        _rd->free_rid(_output_texture);
        _rd->free_rid(_accumulation_texture);
        // _rd->free_rid(_skybox_texture); // TODO: Only free if it's the one
        // created here
        _rd->free_rid(_skybox_sampler);
        _rd->free_rid(_settings_storage_buffer);
        _rd->free_rid(_camera_storage_buffer);
        _rd->free_rid(_scene_spheres_storage_buffer);
        _rd->free_rid(_scene_triangles_storage_buffer);
        _rd->free_rid(_scene_vertex_storage_buffer);
        _rd->free_rid(_scene_materials_storage_buffer);
        _rd->free_rid(_scene_bvh_storage_buffer);
        _rd->free_rid(_image_uniform_set);
        _rd->free_rid(_settings_uniform_set);
        _rd->free_rid(_camera_uniform_set);
        _rd->free_rid(_scene_uniform_set);
        _rd->free_rid(_pipeline);
        _rd->free_rid(_shader);
    }

    static RID create_texture(RenderingDevice* rd, int width, int height,
                              RenderingDevice::DataFormat format) {
        // Texture format
        Ref<RDTextureFormat> texture_format =
            Ref<RDTextureFormat>(memnew(RDTextureFormat));
        texture_format->set_width(width);
        texture_format->set_height(height);
        texture_format->set_depth(1);
        texture_format->set_format(format);
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

    void PTResourceManager::initialize(RenderingDevice* rd, Camera3D* camera,
                                       String shader_path, uint32_t width,
                                       uint32_t height) {
        _rd = rd;
        _camera = camera;

        _create_shader_and_pipeline(shader_path);
        _create_textures(width, height);
        _load_skybox_texture();
        _create_uniforms();
        _create_storage_buffers();
        _create_uniform_sets();
    }
    void PTResourceManager::resize(uint32_t width, uint32_t height) {
        // Destroys _frees textures
        _output_image_uniform->clear_ids();
        _accumulation_image_uniform->clear_ids();

        if (_output_texture.is_valid()) {
            _rd->free_rid(_output_texture);
        }
        if (_accumulation_texture.is_valid()) {
            _rd->free_rid(_accumulation_texture);
        }

        _create_textures(width, height);

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
    void PTResourceManager::_create_textures(uint32_t width, uint32_t height) {
        // LDR output texture
        _output_texture =
            create_texture(_rd, width, height,
                           RenderingDevice::DATA_FORMAT_R16G16B16A16_SFLOAT);

        // HDR accumulation texture
        _accumulation_texture =
            create_texture(_rd, width, height,
                           RenderingDevice::DATA_FORMAT_R32G32B32A32_SFLOAT);
    }

    void PTResourceManager::_load_skybox_texture() {
        // Gets skybox texture from camera environment
        _skybox_texture = _get_camera_skybox_texture(_camera);

        if (!_skybox_texture.is_valid()) {
            // Fallback to a 1x1 black texture if no skybox is set
            _skybox_texture = create_texture(
                _rd, 1, 1, RenderingDevice::DATA_FORMAT_R8G8B8A8_UNORM);
        }
    }

    RID PTResourceManager::_get_camera_skybox_texture(Camera3D* camera) {
        // Skybox texture. Gotten from the environment later.
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

            // Skybox sampler
            Ref<RDSamplerState> sampler_state =
                Ref<RDSamplerState>(memnew(RDSamplerState));
            sampler_state->set_min_filter(
                RenderingDevice::SAMPLER_FILTER_LINEAR);
            sampler_state->set_mag_filter(
                RenderingDevice::SAMPLER_FILTER_LINEAR);
            _skybox_sampler = _rd->sampler_create(sampler_state);

            // Sampler and then texture
            _skybox_image_uniform->add_id(_skybox_sampler);
            _skybox_image_uniform->add_id(_skybox_texture);
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
            _scene_spheres_uniform = Ref<RDUniform>(memnew(RDUniform));
            _scene_spheres_uniform->set_uniform_type(
                RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
            _scene_spheres_uniform->set_binding(0);

            _scene_triangles_uniform = Ref<RDUniform>(memnew(RDUniform));
            _scene_triangles_uniform->set_uniform_type(
                RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
            _scene_triangles_uniform->set_binding(1);

            _scene_vertex_uniform = Ref<RDUniform>(memnew(RDUniform));
            _scene_vertex_uniform->set_uniform_type(
                RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
            _scene_vertex_uniform->set_binding(2);

            _scene_materials_uniform = Ref<RDUniform>(memnew(RDUniform));
            _scene_materials_uniform->set_uniform_type(
                RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
            _scene_materials_uniform->set_binding(3);

            _scene_bvh_uniform = Ref<RDUniform>(memnew(RDUniform));
            _scene_bvh_uniform->set_uniform_type(
                RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
            _scene_bvh_uniform->set_binding(4);
        }
    }

    void PTResourceManager::_create_storage_buffers() {
        // Settings uniform
        {
            PackedByteArray settings_bytes = PackedByteArray();
            settings_bytes.resize(512);
            _settings_storage_buffer = _rd->storage_buffer_create(
                settings_bytes.size(), settings_bytes);
            _settings_uniform->add_id(_settings_storage_buffer);
        }

        // Camera uniform
        {
            PackedByteArray camera_bytes = PackedByteArray();
            camera_bytes.resize(16 * 2 * 4);  // 2 matrices. View/Projection
            _camera_storage_buffer =
                _rd->storage_buffer_create(camera_bytes.size(), camera_bytes);
            _camera_uniform->add_id(_camera_storage_buffer);
        }

        // Scene storage buffers
        {
            PackedByteArray spheres_bytes = PackedByteArray();
            spheres_bytes.resize(1024 * 1024);  // 1 MB initial size
            _scene_spheres_storage_buffer =
                _rd->storage_buffer_create(spheres_bytes.size(), spheres_bytes);
            _scene_spheres_uniform->add_id(_scene_spheres_storage_buffer);
        }
        {
            PackedByteArray triangles_bytes = PackedByteArray();
            triangles_bytes.resize(32 * 1024 * 1024);  // 32 MB initial size
            _scene_triangles_storage_buffer = _rd->storage_buffer_create(
                triangles_bytes.size(), triangles_bytes);
            _scene_triangles_uniform->add_id(_scene_triangles_storage_buffer);
        }
        {
            PackedByteArray vertex_bytes = PackedByteArray();
            vertex_bytes.resize(32 * 1024 * 1024);  // 32 MB initial size
            _scene_vertex_storage_buffer =
                _rd->storage_buffer_create(vertex_bytes.size(), vertex_bytes);
            _scene_vertex_uniform->add_id(_scene_vertex_storage_buffer);
        }
        {
            PackedByteArray materials_bytes = PackedByteArray();
            materials_bytes.resize(2 * 1024 * 1024);  // 2 MB initial size
            _scene_materials_storage_buffer = _rd->storage_buffer_create(
                materials_bytes.size(), materials_bytes);
            _scene_materials_uniform->add_id(_scene_materials_storage_buffer);
        }
        {
            PackedByteArray bvh_bytes = PackedByteArray();
            bvh_bytes.resize(2 * 1024 * 1024);  // 2 MB initial size
            _scene_bvh_storage_buffer =
                _rd->storage_buffer_create(bvh_bytes.size(), bvh_bytes);
            _scene_bvh_uniform->add_id(_scene_bvh_storage_buffer);
        }
    }
    void PTResourceManager::_create_uniform_sets() {
        _image_uniform_set = _rd->uniform_set_create(
            {_output_image_uniform, _accumulation_image_uniform,
             _skybox_image_uniform},
            _shader, 0);

        _settings_uniform_set =
            _rd->uniform_set_create({_settings_uniform}, _shader, 1);

        _camera_uniform_set =
            _rd->uniform_set_create({_camera_uniform}, _shader, 2);

        _scene_uniform_set = _rd->uniform_set_create(
            {_scene_spheres_uniform, _scene_triangles_uniform,
             _scene_vertex_uniform, _scene_materials_uniform,
             _scene_bvh_uniform},
            _shader, 3);
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

}  // namespace godot