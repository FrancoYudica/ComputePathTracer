extends PanelContainer

@export var camera_controller: Node
@export var samples_spin: SpinBox
@export var bounces_spin: SpinBox
@export var render_scale: Slider
@export var fov: Slider
@export var aperture_spin: SpinBox
@export var focal_distance_spin: SpinBox
@export var vsync_check_box: CheckBox
@export var mode_option: OptionButton

func _ready() -> void:
	samples_spin.value = PTRenderer.renderer_settings.samples_per_pixel
	bounces_spin.value = PTRenderer.renderer_settings.max_bounces
	render_scale.value = PTRenderer.renderer_settings.render_scale * 100.0
	fov.value = 60.0
	aperture_spin.value = PTRenderer.renderer_settings.camera_aperture
	focal_distance_spin.value = PTRenderer.renderer_settings.camera_focus_distance
	vsync_check_box.button_pressed = DisplayServer.window_get_vsync_mode() == DisplayServer.VSyncMode.VSYNC_ENABLED
	_set_vsync_mode(vsync_check_box.button_pressed)
	
	samples_spin.value_changed.connect(
		func(value):
			PTRenderer.renderer_settings.samples_per_pixel = value
	)
	bounces_spin.value_changed.connect(
		func(value):
			PTRenderer.renderer_settings.max_bounces = value
	)
	render_scale.value_changed.connect(
		func(value):
			PTRenderer.renderer_settings.render_scale = value / 100.0
	)
	fov.value_changed.connect(
		func(value):
			camera_controller.fov = value
	)
	aperture_spin.value_changed.connect(
		func(value):
			PTRenderer.renderer_settings.camera_aperture = value
	)
	focal_distance_spin.value_changed.connect(
		func(value):
			PTRenderer.renderer_settings.camera_focus_distance = value
	)
	vsync_check_box.toggled.connect(_set_vsync_mode)
	mode_option.item_selected.connect(
		func(index):
			PTRenderer.renderer_settings.render_mode = index
	)
	
func _set_vsync_mode(toggled_on: bool):
	if toggled_on:
		vsync_check_box.text = "Enabled"
		DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_ENABLED)
	else:
		vsync_check_box.text = "Disabled"
		DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)

func _on_update_scene_button_pressed() -> void:
	PTRenderer.update_scene()
