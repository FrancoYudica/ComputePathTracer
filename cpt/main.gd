extends Control

@export var pt_renderer: PTRenderer
@export var scene: PathTraceScene
@export var render_control: Control
@export var camera_controller: CameraController

func _ready() -> void:
	RenderingServer.connect("frame_pre_draw", _render_frame)
	camera_controller.camera = scene.camera

func _on_camera_controller_moved() -> void:
	pt_renderer.queue_clear()

func _render_frame():
	pt_renderer.draw(
		scene.camera, 
		int(render_control.size.x),
		int(render_control.size.y)
	)


func _on_pt_renderer_texture_changed(texture_rid: RID) -> void:
	print("Texture rid: %s" % texture_rid)
