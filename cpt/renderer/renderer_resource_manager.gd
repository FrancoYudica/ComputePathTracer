class_name RendererResourceManager extends RefCounted

var _rd: RenderingDevice
var _scene: PathTraceScene

# Shader and pipeline
var _shader: RID
var _pipeline: RID

# Textures
var _output_texture_rid: RID
var _accumulation_texture_rid: RID
var _skybox_texture_rid: RID

# Uniforms
var _output_image_uniform: RDUniform
var _accumulation_image_uniform: RDUniform
var _skybox_image_uniform: RDUniform
var _settings_uniform: RDUniform
var _camera_uniform: RDUniform
var _scene_spheres_uniform: RDUniform
var _scene_triangles_uniform: RDUniform
var _scene_vertex_uniform: RDUniform
var _scene_materials_uniform: RDUniform

# Storage buffers
var _settings_storage_buffer: RID
var _camera_storage_buffer: RID
var _scene_spheres_storage_buffer: RID
var _scene_triangles_storage_buffer: RID
var _scene_vertex_storage_buffer: RID
var _scene_materials_storage_buffer: RID

# Uniform sets
var _image_set: RID
var _settings_set: RID
var _camera_uniform_set: RID
var _scene_uniform_set: RID

func _init(rd: RenderingDevice, scene: PathTraceScene):
	_rd = rd
	_scene = scene

func initialize(width: int, height: int):
	_load_shader()
	_pipeline = _rd.compute_pipeline_create(_shader)
	_create_textures(width, height)
	_create_uniforms()
	_create_storage_buffers()
	_create_uniform_sets()

func _load_shader():
	var shader_file := load("res://shaders/pathtracer.glsl")
	var shader_spirv: RDShaderSPIRV = shader_file.get_spirv()
	_shader = _rd.shader_create_from_spirv(shader_spirv)

func _create_textures(width: int, height: int):
	_output_texture_rid = _create_attachment_texture(width, height, RenderingDevice.DATA_FORMAT_R8G8B8A8_UNORM)
	_accumulation_texture_rid = _create_attachment_texture(width, height, RenderingDevice.DATA_FORMAT_R32G32B32A32_UINT)
	_load_skybox()

func _create_attachment_texture(width: int, height: int, format):
	var texture_format := RDTextureFormat.new()
	texture_format.width = width
	texture_format.height = height
	texture_format.usage_bits = RenderingDevice.TEXTURE_USAGE_CAN_UPDATE_BIT | RenderingDevice.TEXTURE_USAGE_STORAGE_BIT | RenderingDevice.TEXTURE_USAGE_SAMPLING_BIT
	texture_format.format = format
	var texture_view := RDTextureView.new()
	texture_view.format_override = format
	return _rd.texture_create(texture_format, texture_view)

func _load_skybox():
	var env := _scene.camera.environment
	if env and env.sky.sky_material is PanoramaSkyMaterial:
		var tex: Texture2D = env.sky.sky_material.panorama
		_skybox_texture_rid = RenderingServer.texture_get_rd_texture(tex.get_rid())

func _create_uniforms():
	# Output uniform
	_output_image_uniform = RDUniform.new()
	_output_image_uniform.uniform_type = RenderingDevice.UNIFORM_TYPE_IMAGE
	_output_image_uniform.binding = 0
	_output_image_uniform.add_id(_output_texture_rid)

	# Accumulation uniform
	_accumulation_image_uniform = RDUniform.new()
	_accumulation_image_uniform.uniform_type = RenderingDevice.UNIFORM_TYPE_IMAGE
	_accumulation_image_uniform.binding = 1
	_accumulation_image_uniform.add_id(_accumulation_texture_rid)

	# Skybox uniform
	_skybox_image_uniform = RDUniform.new()
	_skybox_image_uniform.uniform_type = RenderingDevice.UNIFORM_TYPE_SAMPLER_WITH_TEXTURE
	_skybox_image_uniform.binding = 2
	var sampler_state := RDSamplerState.new()
	sampler_state.min_filter = RenderingDevice.SAMPLER_FILTER_LINEAR
	sampler_state.mag_filter = RenderingDevice.SAMPLER_FILTER_LINEAR
	var image_sampler = _rd.sampler_create(sampler_state)
	_skybox_image_uniform.add_id(image_sampler)
	_skybox_image_uniform.add_id(_skybox_texture_rid)

	# Settings uniform
	_settings_uniform = RDUniform.new()
	_settings_uniform.uniform_type = RenderingDevice.UNIFORM_TYPE_STORAGE_BUFFER
	_settings_uniform.binding = 0

	# Camera uniform
	_camera_uniform = RDUniform.new()
	_camera_uniform.uniform_type = RenderingDevice.UNIFORM_TYPE_STORAGE_BUFFER
	_camera_uniform.binding = 0

	# Scene uniforms
	_scene_spheres_uniform = RDUniform.new()
	_scene_spheres_uniform.uniform_type = RenderingDevice.UNIFORM_TYPE_STORAGE_BUFFER
	_scene_spheres_uniform.binding = 0

	_scene_triangles_uniform = RDUniform.new()
	_scene_triangles_uniform.uniform_type = RenderingDevice.UNIFORM_TYPE_STORAGE_BUFFER
	_scene_triangles_uniform.binding = 1

	_scene_vertex_uniform = RDUniform.new()
	_scene_vertex_uniform.uniform_type = RenderingDevice.UNIFORM_TYPE_STORAGE_BUFFER
	_scene_vertex_uniform.binding = 2

	_scene_materials_uniform = RDUniform.new()
	_scene_materials_uniform.uniform_type = RenderingDevice.UNIFORM_TYPE_STORAGE_BUFFER
	_scene_materials_uniform.binding = 3

func _create_storage_buffers():
	# Settings uniform
	var settings_bytes = PackedByteArray()
	settings_bytes.resize(512)
	settings_bytes.fill(0)
	_settings_storage_buffer = _rd.storage_buffer_create(settings_bytes.size(), settings_bytes)
	_settings_uniform.add_id(_settings_storage_buffer)

	# Camera buffer
	var camera_bytes = PackedByteArray()
	camera_bytes.resize(CameraManager.get_buffer_size())
	camera_bytes.fill(0)
	_camera_storage_buffer = _rd.storage_buffer_create(camera_bytes.size(), camera_bytes)
	_camera_uniform.add_id(_camera_storage_buffer)

	# Spheres buffer
	var spheres_bytes = PackedByteArray()
	spheres_bytes.resize(1024 * 1024)
	_scene_spheres_storage_buffer = _rd.storage_buffer_create(spheres_bytes.size(), spheres_bytes)
	_scene_spheres_uniform.add_id(_scene_spheres_storage_buffer)

	# Triangles buffer
	var triangle_bytes = PackedByteArray()
	triangle_bytes.resize(1024 * 1024)
	_scene_triangles_storage_buffer = _rd.storage_buffer_create(triangle_bytes.size(), triangle_bytes)
	_scene_triangles_uniform.add_id(_scene_triangles_storage_buffer)

	# Vertex buffer
	var vertex_bytes = PackedByteArray()
	vertex_bytes.resize(1024 * 1024)
	_scene_vertex_storage_buffer = _rd.storage_buffer_create(vertex_bytes.size(), vertex_bytes)
	_scene_vertex_uniform.add_id(_scene_vertex_storage_buffer)

	# Material buffer
	var materials_bytes = PackedByteArray()
	materials_bytes.resize(1024 * 1024)
	_scene_materials_storage_buffer = _rd.storage_buffer_create(materials_bytes.size(), materials_bytes)
	_scene_materials_uniform.add_id(_scene_materials_storage_buffer)

func _create_uniform_sets():
	_image_set = _rd.uniform_set_create([_output_image_uniform, _accumulation_image_uniform, _skybox_image_uniform], _shader, 0)
	_settings_set = _rd.uniform_set_create([_settings_uniform], _shader, 1)
	_camera_uniform_set = _rd.uniform_set_create([_camera_uniform], _shader, 2)
	_scene_uniform_set = _rd.uniform_set_create([_scene_spheres_uniform, _scene_triangles_uniform, _scene_vertex_uniform, _scene_materials_uniform], _shader, 3)

func resize(width: int, height: int):
	_output_image_uniform.clear_ids()
	_accumulation_image_uniform.clear_ids()
	_rd.free_rid(_output_texture_rid)
	_rd.free_rid(_accumulation_texture_rid)
	
	_output_texture_rid = _create_attachment_texture(width, height, RenderingDevice.DATA_FORMAT_R8G8B8A8_UNORM)
	_accumulation_texture_rid = _create_attachment_texture(width, height, RenderingDevice.DATA_FORMAT_R32G32B32A32_UINT)
	
	_output_image_uniform.add_id(_output_texture_rid)
	_accumulation_image_uniform.add_id(_accumulation_texture_rid)
	
	if _rd.uniform_set_is_valid(_image_set):
		_rd.free_rid(_image_set)
	_image_set = _rd.uniform_set_create([_output_image_uniform, _accumulation_image_uniform, _skybox_image_uniform], _shader, 0)

func get_output_texture() -> RID:
	return _output_texture_rid

func get_shader() -> RID:
	return _shader

func get_pipeline() -> RID:
	return _pipeline

func get_uniform_sets() -> Dictionary:
	return {
		"image": _image_set,
		"settings": _settings_set,
		"camera": _camera_uniform_set,
		"scene": _scene_uniform_set
	}

func get_settings_buffer() -> RID:
	return _settings_storage_buffer

func get_camera_buffer() -> RID:
	return _camera_storage_buffer

func get_scene_buffers() -> Dictionary:
	return {
		"spheres": _scene_spheres_storage_buffer,
		"triangles": _scene_triangles_storage_buffer,
		"vertices": _scene_vertex_storage_buffer,
		"materials": _scene_materials_storage_buffer
	}

func cleanup():
	_rd.free_rid(_output_texture_rid)
	_rd.free_rid(_accumulation_texture_rid)
	_rd.free_rid(_settings_storage_buffer)
	_rd.free_rid(_camera_storage_buffer)
	_rd.free_rid(_scene_spheres_storage_buffer)
	_rd.free_rid(_scene_triangles_storage_buffer)
	_rd.free_rid(_scene_vertex_storage_buffer)
	_rd.free_rid(_scene_materials_storage_buffer)
	_rd.free_rid(_image_set)
	_rd.free_rid(_settings_set)
	_rd.free_rid(_camera_uniform_set)
	_rd.free_rid(_scene_uniform_set)
	_rd.free_rid(_pipeline)
	_rd.free_rid(_shader)
