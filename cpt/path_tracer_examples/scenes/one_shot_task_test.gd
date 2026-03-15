extends Control

@export var render_button: Button
@export var render_camera: Camera3D
@export var output_texture_rect: TextureRect
var task_handle = null
var renderer_settings = PTRendererSettings.new()
var _processing_task = false

func _ready():
	render_button.pressed.connect(_submit_render_task)
	PTRenderer.init()
	PTRenderer.task_completed.connect(_on_task_completed)
	
func _submit_render_task():
	
	if _processing_task:
		push_warning("Already rendering, please wait render task to finish")
		return
	
	task_handle = PTRenderer.submit_one_shot_task(render_camera, renderer_settings)
	_processing_task = true


func _on_task_completed(task):
	if task != task_handle:
		return
	
	var texture_rd_rid = PTRenderer.get_task_output(task_handle)
	
	# Gets texture 2d rd
	var texture: Texture2DRD = output_texture_rect.texture as Texture2D
	if texture == null:
		texture = Texture2DRD.new()
		output_texture_rect.texture = texture
		
	# Stores old texture rid
	var old_texture_rid = RID()
	if texture.texture_rd_rid.is_valid():
		old_texture_rid = texture.texture_rd_rid
	
	# Sets new texture rid
	texture.texture_rd_rid = texture_rd_rid
	
	# Erases old texture
	if old_texture_rid.is_valid():
		RenderingServer.get_rendering_device().free_rid(old_texture_rid)
		
	_processing_task = false
	task_handle = null
