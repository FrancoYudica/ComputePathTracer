extends PanelContainer

@export var rasterized_panel: Control
@export var rendered_sub_viewport_container: SubViewportContainer
@export var sub_viewport: SubViewport
@export var save_file_dialog: FileDialog
@export var settings_panel: Node
@export var bvh_settings_panel: Node
@export var stats_panel: Node

func _ready() -> void:
	save_file_dialog.file_selected.connect(_file_saved)
	settings_panel.set_renderer_settings(PTRenderer.renderer_settings)
	bvh_settings_panel.set_renderer_settings(PTRenderer.renderer_settings)
	stats_panel.set_stats(PTRenderer.get_stats())
	
func _on_rasterized_check_box_toggled(toggled_on: bool) -> void:
	rasterized_panel.visible = toggled_on

func _on_screenshot_button_pressed() -> void:
	save_file_dialog.show()

func _file_saved(file):
	var texture = sub_viewport.get_texture()
	var image = texture.get_image()
	var status = image.save_png(file)
	if status != OK:
		push_error("Unable to save image at: %s" % file)
	else:
		print("Saved screenshot at: %s" % file)

func _on_update_scene_button_pressed() -> void:
	PTRenderer.update_scene()
