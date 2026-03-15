extends PanelContainer

@export var camera: Camera3D
@export var samples_spin: SpinBox
@export var bounces_spin: SpinBox
@export var render_scale: Slider
@export var fov: Slider
@export var aperture_spin: SpinBox
@export var focal_distance_spin: SpinBox
@export var vsync_check_box: CheckBox
@export var mode_option: OptionButton

var _settings: PTRendererSettings = null

func set_renderer_settings(settings: PTRendererSettings):
	# Disconnect previous signals if they exist
	if _settings != null:
		samples_spin.value_changed.disconnect(_on_samples_changed)
		bounces_spin.value_changed.disconnect(_on_bounces_changed)
		render_scale.value_changed.disconnect(_on_scale_changed)
		fov.value_changed.disconnect(_on_fov_changed)
		aperture_spin.value_changed.disconnect(_on_aperture_changed)
		focal_distance_spin.value_changed.disconnect(_on_focal_dist_changed)
		vsync_check_box.toggled.disconnect(_set_vsync_mode)
		mode_option.item_selected.disconnect(_on_mode_selected)

	_settings = settings
	
	# Update UI values from new settings
	samples_spin.value = _settings.samples_per_pixel
	bounces_spin.value = _settings.max_bounces
	render_scale.value = _settings.render_scale * 100.0
	fov.value = camera.fov
	aperture_spin.value = _settings.camera_aperture
	focal_distance_spin.value = _settings.camera_focus_distance
	vsync_check_box.button_pressed = DisplayServer.window_get_vsync_mode() == DisplayServer.VSyncMode.VSYNC_ENABLED
	_set_vsync_mode(vsync_check_box.button_pressed)
	
	# Connect to new named methods
	samples_spin.value_changed.connect(_on_samples_changed)
	bounces_spin.value_changed.connect(_on_bounces_changed)
	render_scale.value_changed.connect(_on_scale_changed)
	fov.value_changed.connect(_on_fov_changed)
	aperture_spin.value_changed.connect(_on_aperture_changed)
	focal_distance_spin.value_changed.connect(_on_focal_dist_changed)
	vsync_check_box.toggled.connect(_set_vsync_mode)
	mode_option.item_selected.connect(_on_mode_selected)

func _on_samples_changed(value: float):
	_settings.samples_per_pixel = int(value)

func _on_bounces_changed(value: float):
	_settings.max_bounces = int(value)

func _on_scale_changed(value: float):
	_settings.render_scale = value / 100.0

func _on_fov_changed(value: float):
	camera.fov = value
	_settings.emit_changed()

func _on_aperture_changed(value: float):
	_settings.camera_aperture = value

func _on_focal_dist_changed(value: float):
	_settings.camera_focus_distance = value

func _on_mode_selected(index: int):
	_settings.render_mode = index

func _set_vsync_mode(toggled_on: bool):
	if toggled_on:
		vsync_check_box.text = "Enabled"
		DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_ENABLED)
	else:
		vsync_check_box.text = "Disabled"
		DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)

func _on_update_scene_button_pressed() -> void:
	PTRenderer.update_scene()
