extends Control

@export var renderer: Renderer

func _on_camera_controller_moved() -> void:
	renderer.clear_accumulated_buffer()
