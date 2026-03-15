extends PanelContainer

@export var depth_spin: SpinBox
@export var max_leaf_triangles_spin: SpinBox
@export var sah_bins_spin: SpinBox
@export var box_threshold_spin: SpinBox
@export var triangle_threshold_spin: SpinBox

var _settings: PTRendererSettings = null

func set_renderer_settings(settings: PTRendererSettings) -> void:
	
	# Safety: Disconnect existing connections if any
	if _settings != null:
		depth_spin.value_changed.disconnect(_on_depth_changed)
		max_leaf_triangles_spin.value_changed.disconnect(_on_max_leaf_triangles_changed)
		sah_bins_spin.value_changed.disconnect(_on_sah_bins_changed)
		box_threshold_spin.value_changed.disconnect(_on_box_threshold_changed)
		triangle_threshold_spin.value_changed.disconnect(_on_triangle_threshold_changed)
	
	_settings = settings
	
	# Initialize UI values
	depth_spin.value = settings.bvh_max_depth
	max_leaf_triangles_spin.value = settings.bvh_max_triangles_per_leaf
	sah_bins_spin.value = settings.bvh_sah_bins
	box_threshold_spin.value = settings.get_debug_bvh_box_intersections_threshold()
	triangle_threshold_spin.value = settings.get_debug_bvh_triangle_intersections_threshold()
	
	# Connect to named methods
	depth_spin.value_changed.connect(_on_depth_changed)
	max_leaf_triangles_spin.value_changed.connect(_on_max_leaf_triangles_changed)
	sah_bins_spin.value_changed.connect(_on_sah_bins_changed)
	box_threshold_spin.value_changed.connect(_on_box_threshold_changed)
	triangle_threshold_spin.value_changed.connect(_on_triangle_threshold_changed)

func _on_depth_changed(value: float) -> void:
	_settings.bvh_max_depth = int(value)

func _on_max_leaf_triangles_changed(value: float) -> void:
	_settings.bvh_max_triangles_per_leaf = int(value)

func _on_sah_bins_changed(value: float) -> void:
	_settings.bvh_sah_bins = int(value)

func _on_box_threshold_changed(value: float) -> void:
	_settings.set_debug_bvh_box_intersections_threshold(value)

func _on_triangle_threshold_changed(value: float) -> void:
	_settings.set_debug_bvh_triangle_intersections_threshold(value)
