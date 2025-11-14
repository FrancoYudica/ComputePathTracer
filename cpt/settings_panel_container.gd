extends PanelContainer

@export var rasterized_panel: Control
@export var renderer: Renderer
@export var rendered_sub_viewport_container: SubViewportContainer
@export var sub_viewport: SubViewport
@export var camera_controller: Node
@export var save_file_dialog: FileDialog
@export var aperture_slider: Slider
@export var focal_distance_slider: Slider

func _ready() -> void:
	save_file_dialog.file_selected.connect(_file_saved)
	aperture_slider.value = renderer.render_settings.camera_aperture
	focal_distance_slider.value = renderer.render_settings.camera_focal_distance

func _on_rasterized_check_box_toggled(toggled_on: bool) -> void:
	rasterized_panel.visible = toggled_on

func _on_samples_spin_box_value_changed(value: float) -> void:
	renderer.render_settings.samples_per_pixel = int(value)

func _on_render_scale_slider_value_changed(value: float) -> void:
	renderer.set_render_scale(value / 100.0)

func _on_fov_h_slider_value_changed(value: float) -> void:
	camera_controller.fov = value

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

func _on_aperture_h_slider_value_changed(value: float) -> void:
	renderer.render_settings.camera_aperture = value

func _on_focal_length_h_slider_value_changed(value: float) -> void:
	renderer.render_settings.camera_focal_distance = value
