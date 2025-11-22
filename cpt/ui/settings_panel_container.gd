extends PanelContainer

@export var rasterized_panel: Control
@export var renderer: Renderer
@export var rendered_sub_viewport_container: SubViewportContainer
@export var sub_viewport: SubViewport
@export var camera_controller: Node
@export var save_file_dialog: FileDialog
@export var samples_spin: SpinBox
@export var bounces_spin: SpinBox
@export var aperture_slider: Slider
@export var focal_distance_slider: Slider
@export var vsync_check_box: CheckBox


func _ready() -> void:
	save_file_dialog.file_selected.connect(_file_saved)
	samples_spin.value = renderer.render_settings.samples_per_pixel
	bounces_spin.value = renderer.render_settings.max_bounces
	aperture_slider.value = renderer.render_settings.camera_aperture
	focal_distance_slider.value = renderer.render_settings.camera_focal_distance
	vsync_check_box.button_pressed = DisplayServer.window_get_vsync_mode() == DisplayServer.VSyncMode.VSYNC_ENABLED
	_set_vsync_mode(vsync_check_box.button_pressed)

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

func _on_bounces_spin_box_value_changed(value: float) -> void:
	renderer.render_settings.max_bounces = int(value)

func _on_v_sync_check_box_toggled(toggled_on: bool) -> void:
	_set_vsync_mode(toggled_on)

func _set_vsync_mode(toggled_on: bool):
	if toggled_on:
		vsync_check_box.text = "Enabled"
		DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_ENABLED)
	else:
		vsync_check_box.text = "Disabled"
		DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)

func _on_mode_option_button_item_selected(index: int) -> void:
	renderer.render_settings.mode = index
