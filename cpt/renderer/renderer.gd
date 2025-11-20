class_name Renderer extends Node

@onready var _rd: RenderingDevice = RenderingServer.get_rendering_device()
@export var scene: PathTraceScene
@export var render_control: Control

@export var render_settings: RenderSettings:
	set(new_settings):
		render_settings = new_settings
		queue_clear()
		render_settings.changed.connect(queue_clear)

var _resource_manager: RendererResourceManager
var _scene_data_manager: SceneDataManager

var _still_frames_count: int = 1
var _render_scale: float = 1.0

## Updated every frame, holds all the scene materials
var _frame_materials: Dictionary[PTMaterial, int]

var _clear_buffer: bool = false

func queue_clear():
	_clear_buffer = true

func get_texture_rid():
	return _resource_manager.get_output_texture()

func resize(width: int, height: int):
	_resource_manager.resize(width, height)
	print("Viewport resized: [%s, %s]" % [width, height])
	queue_clear()

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

func _clear_accumulated_buffer():
	_still_frames_count = 1

func _ready() -> void:
	_initialize_compute()
	RenderingServer.connect("frame_pre_draw", _draw)
	render_control.resized.connect(
		func():
			resize(get_render_width(), get_render_height())
	)

func _initialize_compute():
	_resource_manager = RendererResourceManager.new(_rd, scene)
	_resource_manager.initialize(get_render_width(), get_render_height())
	
	_scene_data_manager = SceneDataManager.new()
	_scene_data_manager.initialize(
		_rd,
		get_tree(),
		_resource_manager.get_scene_buffers()["spheres"],
		_resource_manager.get_scene_buffers()["triangles"],
		_resource_manager.get_scene_buffers()["vertices"],
		_resource_manager.get_scene_buffers()["materials"]
	)
	
func _notification(what: int) -> void:
	if what == NOTIFICATION_PREDELETE:
		_resource_manager.cleanup()

func _get_push_constant_bytes() -> PackedByteArray:
	var texture_width = get_render_width()
	var texture_height = get_render_height()

	var push_constant = PackedFloat32Array([
		texture_width,
		texture_height,
		_still_frames_count, # Frame number
		_still_frames_count * 1664525 + 1013904223 # Frame-based random seed
	])
	
	return push_constant.to_byte_array()

func _draw():
	
	if _clear_buffer:
		_clear_accumulated_buffer()
		_clear_buffer = false
	
	var push_constant_byte_array = _get_push_constant_bytes()
	var x_groups = ceili(float(get_render_width()) / 8)
	var y_groups = ceili(float(get_render_height()) / 8)
	
	_update_settings_storage_buffer()
	_update_camera_storage_buffer()
	_scene_data_manager.update_buffers()
	
	print("Spheres: %s. Triangles: %s. Vertices %s. Materials %s" % [
		_scene_data_manager.get_sphere_count(),
		_scene_data_manager.get_triangle_count(),
		_scene_data_manager.get_vertex_count(),
		_scene_data_manager.get_material_count()
	])

	var compute_list := _rd.compute_list_begin()
	_rd.compute_list_bind_compute_pipeline(compute_list, _resource_manager.get_pipeline())
	var sets = _resource_manager.get_uniform_sets()
	_rd.compute_list_bind_uniform_set(compute_list, sets["image"], 0)
	_rd.compute_list_bind_uniform_set(compute_list, sets["settings"], 1)
	_rd.compute_list_bind_uniform_set(compute_list, sets["camera"], 2)
	_rd.compute_list_bind_uniform_set(compute_list, sets["scene"], 3)
	_rd.compute_list_set_push_constant(compute_list, push_constant_byte_array, push_constant_byte_array.size())
	_rd.compute_list_dispatch(compute_list, x_groups, y_groups, 1)
	_rd.compute_list_end()
	
	_still_frames_count += 1


func _update_settings_storage_buffer():
	var data = PackedFloat32Array(
		[
			render_settings.samples_per_pixel,
			render_settings.max_bounces,
			render_settings.environment_energy,
			render_settings.camera_aperture,
			render_settings.camera_focal_distance,
			0.0, 0.0, 0.0
		]
	).to_byte_array()
	_rd.buffer_update(_resource_manager.get_settings_buffer(), 0, data.size(), data)

func _update_camera_storage_buffer():
	# Update buffer data
	var camera_bytes = CameraManager.get_camera_bytes(scene.camera, get_render_width(), get_render_height())
	_rd.buffer_update(_resource_manager.get_camera_buffer(), 0, camera_bytes.size(), camera_bytes)


func _push_material(material: PTMaterial) -> int:
	if not _frame_materials.has(material):
		_frame_materials[material] = _frame_materials.size()
		
	return _frame_materials[material]
