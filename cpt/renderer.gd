class_name Renderer extends Node

@onready var _rd: RenderingDevice = RenderingServer.get_rendering_device()
@export var camera: Camera3D
@export var render_control: Control

var samples: int = 1

var _shader: RID
var _pipeline: RID

var _output_texture_set: RID
var _output_texture_rid: RID # Low precision texture

var _accumulation_texture_set: RID
var _accumulation_texture_rid: RID # High precision texture

var _camera_uniform_set: RID
var _camera_storage_buffer: RID

var _scene_uniform_set: RID
var _scene_spheres_storage_buffer: RID
var _scene_materials_storage_buffer: RID


var _image_uniform: RDUniform
var _accumulation_uniform: RDUniform
var _camera_uniform: RDUniform
var _scene_spheres_uniform: RDUniform
var _scene_materials_uniform: RDUniform

var _still_frames_count: int = 1
var _render_scale: float = 1.0

func get_texture_rid():
	return _output_texture_rid

func resize(width: int, height: int):
	_image_uniform.clear_ids()
	_accumulation_uniform.clear_ids()
	_rd.free_rid(_output_texture_rid)
	_rd.free_rid(_accumulation_texture_rid)
	_output_texture_rid = _create_attachment_texture(get_render_width(), get_render_height(), RenderingDevice.DATA_FORMAT_R8G8B8A8_UNORM)
	_accumulation_texture_rid = _create_attachment_texture(get_render_width(), get_render_height(), RenderingDevice.DATA_FORMAT_R32G32B32A32_UINT)
	_image_uniform.add_id(_output_texture_rid)
	_accumulation_uniform.add_id(_accumulation_texture_rid)
	if _rd.uniform_set_is_valid(_output_texture_set):
		_rd.free_rid(_output_texture_set)
	_output_texture_set = _rd.uniform_set_create([_image_uniform], _shader, 0)
	_accumulation_texture_set = _rd.uniform_set_create([_accumulation_uniform], _shader, 0)
	print("Viewport resized: [%s, %s]" % [width, height])
	clear_accumulated_buffer()

func set_render_scale(scale: float):
	_render_scale = scale
	resize(get_render_width(), get_render_height())
	

func get_render_width():
	
	if render_control.size.x == 0.0:
		return 1
	
	return int(render_control.size.x * _render_scale) 
	
func get_render_height():

	if render_control.size.y == 0.0:
		return 1

	return int(render_control.size.y * _render_scale)

func clear_accumulated_buffer():
	_still_frames_count = 1

func _ready() -> void:
	_initialize_compute()
	RenderingServer.connect("frame_pre_draw", _draw)
	render_control.resized.connect(
		func():
			resize(get_render_width(), get_render_height())
	)

func _load_shader():
	var shader_file := load("res://shaders/pathtracer.glsl")
	var shader_spirv: RDShaderSPIRV = shader_file.get_spirv()
	_shader = _rd.shader_create_from_spirv(shader_spirv)

func _create_attachment_texture(width, height, format):
	var texture_format := RDTextureFormat.new()
	texture_format.width = width
	texture_format.height = height
	texture_format.usage_bits = RenderingDevice.TEXTURE_USAGE_CAN_UPDATE_BIT | RenderingDevice.TEXTURE_USAGE_STORAGE_BIT  | RenderingDevice.TEXTURE_USAGE_SAMPLING_BIT
	texture_format.format = format
	var texture_view := RDTextureView.new()
	texture_view.format_override = format
	return _rd.texture_create(texture_format, texture_view)

func _initialize_compute():
	_load_shader()
	_pipeline = _rd.compute_pipeline_create(_shader)
	_output_texture_rid = _create_attachment_texture(get_render_width(), get_render_height(), RenderingDevice.DATA_FORMAT_R8G8B8A8_UNORM)
	_accumulation_texture_rid = _create_attachment_texture(get_render_width(), get_render_height(), RenderingDevice.DATA_FORMAT_R32G32B32A32_UINT)
	
	# Image uniform set
	_image_uniform = RDUniform.new()
	_image_uniform.uniform_type = RenderingDevice.UNIFORM_TYPE_IMAGE
	_image_uniform.binding = 0
	_image_uniform.add_id(_output_texture_rid)
	_output_texture_set = _rd.uniform_set_create([_image_uniform], _shader, 0)

	# Accumulation uniform set
	_accumulation_uniform = RDUniform.new()
	_accumulation_uniform.uniform_type = RenderingDevice.UNIFORM_TYPE_IMAGE
	_accumulation_uniform.binding = 0
	_accumulation_uniform.add_id(_accumulation_texture_rid)
	_accumulation_texture_set = _rd.uniform_set_create([_accumulation_uniform], _shader, 1)


	# Camera uniform set
	_camera_uniform = RDUniform.new()
	_camera_uniform.uniform_type = RenderingDevice.UNIFORM_TYPE_STORAGE_BUFFER
	_camera_uniform.binding = 0
	
	var camera_bytes = PackedByteArray()
	camera_bytes.resize(16 * 2 * 4) # 16f per matrix, 2 matrices, and 4 bytes per float
	camera_bytes.fill(0)
	_camera_storage_buffer = _rd.storage_buffer_create(
		camera_bytes.size(),
		camera_bytes)
	_camera_uniform.add_id(_camera_storage_buffer)
	_camera_uniform_set = _rd.uniform_set_create([_camera_uniform], _shader, 2)
	
	# Sphere uniform set binding
	_scene_spheres_uniform = RDUniform.new()
	_scene_spheres_uniform.uniform_type = RenderingDevice.UNIFORM_TYPE_STORAGE_BUFFER
	_scene_spheres_uniform.binding = 0
	var spheres_bytes = PackedByteArray()
	spheres_bytes.resize(1024)
	_scene_spheres_storage_buffer = _rd.storage_buffer_create(spheres_bytes.size(), spheres_bytes)
	_scene_spheres_uniform.add_id(_scene_spheres_storage_buffer)
	
	# Material uniform set binding
	_scene_materials_uniform = RDUniform.new()
	_scene_materials_uniform.uniform_type = RenderingDevice.UNIFORM_TYPE_STORAGE_BUFFER
	_scene_materials_uniform.binding = 1
	var materials_bytes = PackedByteArray()
	materials_bytes.resize(1024)
	_scene_materials_storage_buffer = _rd.storage_buffer_create(materials_bytes.size(), materials_bytes)
	_scene_materials_uniform.add_id(_scene_materials_storage_buffer)
	
	_scene_uniform_set = _rd.uniform_set_create([_scene_spheres_uniform, _scene_materials_uniform], _shader, 3)

func _notification(what: int) -> void:
	if what == NOTIFICATION_PREDELETE:
		_rd.free_rid(_output_texture_rid)
		_rd.free_rid(_camera_storage_buffer)
		_rd.free_rid(_scene_spheres_storage_buffer)
		_rd.free_rid(_scene_materials_storage_buffer)
		_rd.free_rid(_output_texture_set)
		_rd.free_rid(_camera_uniform_set)
		_rd.free_rid(_scene_uniform_set)
		_rd.free_rid(_pipeline)
		_rd.free_rid(_shader)

# Helper: converts a Transform3D to a 4x4 float array (column-major)
func transform3d_to_mat4_floats(t: Transform3D) -> PackedFloat32Array:
	var col0: Vector3 = t.basis.x
	var col1: Vector3 = t.basis.y
	var col2: Vector3 = t.basis.z
	var col3: Vector3 = t.origin   # translation (origin)
	# Column-major 4x4: columns 0..3, each has 4 components (x,y,z,w).
	# For the 4th element in each column, use 0 for rotational columns, 1 for translation column.
	var arr := PackedFloat32Array()
	# Column 0
	arr.append(col0.x); arr.append(col0.y); arr.append(col0.z); arr.append(0.0)
	# Column 1
	arr.append(col1.x); arr.append(col1.y); arr.append(col1.z); arr.append(0.0)
	# Column 2
	arr.append(col2.x); arr.append(col2.y); arr.append(col2.z); arr.append(0.0)
	# Column 3 (translation)
	arr.append(col3.x); arr.append(col3.y); arr.append(col3.z); arr.append(1.0)
	return arr

func _draw():
	
	var texture_width = get_render_width()
	var texture_height = get_render_height()
	var x_groups = ceili(float(texture_width) / 8)
	var y_groups = ceili(float(texture_height) / 8)
	
	var push_constant = PackedFloat32Array([
		texture_width,
		texture_height,
		_still_frames_count, # Frame number
		_still_frames_count * 1664525 + 1013904223, # Frame-based random seed
		samples,
		1.0 / float(_still_frames_count), # Frame accumulation weight
		0.0, 0.0
	])
	
	_still_frames_count += 1
	
	var push_constant_byte_array = push_constant.to_byte_array()
	
	_update_camera_storage_buffer()
	_update_scene_storage_buffer()

	var compute_list := _rd.compute_list_begin()
	_rd.compute_list_bind_compute_pipeline(compute_list, _pipeline)
	_rd.compute_list_bind_uniform_set(compute_list, _output_texture_set, 0)
	_rd.compute_list_bind_uniform_set(compute_list, _accumulation_texture_set, 1)
	_rd.compute_list_bind_uniform_set(compute_list, _camera_uniform_set, 2)
	_rd.compute_list_bind_uniform_set(compute_list, _scene_uniform_set, 3)
	_rd.compute_list_set_push_constant(compute_list, push_constant_byte_array, push_constant_byte_array.size())
	_rd.compute_list_dispatch(compute_list, x_groups, y_groups, 1)
	_rd.compute_list_end()


func _update_camera_storage_buffer():
	# Build camera matrices
	var view = camera.get_camera_transform().affine_inverse()
	var projection = camera.get_camera_projection() # Scene camera projection. Viewport dependent fov
	
	# Re-calculates projection with scene aspect ratio
	var corrected_projection = Projection.create_perspective(
		camera.fov,
		float(get_render_width()) / float(get_render_height()),
		projection.get_z_near(),
		projection.get_z_far()
	)

	var view_floats = transform3d_to_mat4_floats(view)
	var proj_floats = PackedFloat32Array()
	for c in range(4):
		for r in range(4):
			proj_floats.append(corrected_projection[c][r])
			
	# Combine view + projection
	var camera_data = PackedFloat32Array()
	camera_data.append_array(view_floats)
	camera_data.append_array(proj_floats)

	# Update buffer data
	var camera_bytes = camera_data.to_byte_array()
	_rd.buffer_update(_camera_storage_buffer, 0, camera_bytes.size(), camera_bytes)

func _update_scene_storage_buffer():
	var SPHERE_FLOATS = 8
	var spheres = get_tree().get_nodes_in_group("procedural_sphere")
	
	var spheres_data = PackedFloat32Array() # Sphere count + 3 bytes pad + Spheres data
	var materials_data = PackedFloat32Array()
	var materials_count = 0
	spheres_data.resize(spheres.size() * SPHERE_FLOATS + 4)
	spheres_data[0] = spheres.size()
	for i in range(spheres.size()):
		var sphere: ProceduralSphere = spheres[i] as ProceduralSphere
		var base_offset = i * SPHERE_FLOATS + 4
		var material_offset = 4 # x, y, z, r, mtl
		sphere.load_bytes(spheres_data, base_offset)
		spheres_data[base_offset + material_offset] = materials_count # Material index
		materials_data.push_back(sphere.material_type)
		materials_data.push_back(sphere.emission)
		materials_data.push_back(sphere.fuzz)
		materials_data.push_back(sphere.refraction_index)
		materials_data.push_back(sphere.color.r)
		materials_data.push_back(sphere.color.g)
		materials_data.push_back(sphere.color.b)
		materials_data.push_back(0.0)
		materials_count += 1
		
	var sphere_bytes = spheres_data.to_byte_array()
	_rd.buffer_update(_scene_spheres_storage_buffer, 0, sphere_bytes.size(), sphere_bytes)

	var materials_bytes = materials_data.to_byte_array()
	_rd.buffer_update(_scene_materials_storage_buffer, 0, materials_bytes.size(), materials_bytes)
