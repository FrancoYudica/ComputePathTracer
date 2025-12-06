extends Control

func _enter_tree() -> void:
	PTRenderer.init()
	PTRenderer.renderer_settings.render_mode = 1

func _exit_tree() -> void:
	PTRenderer.destroy()

func _on_camera_controller_moved() -> void:
	PTRenderer.queue_clear()
