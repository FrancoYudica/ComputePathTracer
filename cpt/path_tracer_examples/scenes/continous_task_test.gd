extends Control

@export var render_camera: Camera3D
@export var output_texture_rect: TextureRect

@export var settings_panel: Node
@export var bvh_settings_panel: Node
@export var stats_panel: Node


var task_handle = null
var renderer_settings = PTRendererSettings.new()

func _init() -> void:
	PTRenderer.init()

func _ready():
	PTRenderer.texture_changed.connect(_on_texture_changed)
	
	settings_panel.set_renderer_settings(renderer_settings)
	bvh_settings_panel.set_renderer_settings(renderer_settings)
	stats_panel.set_stats(PTRenderer.get_stats())
	
	_submit_render_task()
	
func _submit_render_task():
	
	if task_handle != null:
		push_warning("Already rendering, please wait render task to finish")
		return
	
	task_handle = PTRenderer.submit_continuous_task(render_camera, renderer_settings)


func _on_texture_changed(task):
	if task != task_handle:
		return
		
	var texture_rd_rid = PTRenderer.get_task_output(task_handle)
	
	# Gets texture 2d rd
	var texture: Texture2DRD = output_texture_rect.texture as Texture2D
	if texture == null:
		texture = Texture2DRD.new()
		output_texture_rect.texture = texture
		
	# Sets new texture rid
	texture.texture_rd_rid = texture_rd_rid


func _on_camera_controller_moved() -> void:
	if task_handle != null:
		PTRenderer.task_clear_progress(task_handle)
