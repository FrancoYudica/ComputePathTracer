extends PanelContainer

@export var depth_spin: SpinBox
@export var max_leaf_triangles_spin: SpinBox
@export var sah_bins_spin: SpinBox
@export var box_threshold_spin: SpinBox
@export var triangle_threshold_spin: SpinBox


func _ready() -> void:
	depth_spin.value = PTRenderer.renderer_settings.bvh_max_depth
	max_leaf_triangles_spin.value = PTRenderer.renderer_settings.bvh_max_triangles_per_leaf
	sah_bins_spin.value = PTRenderer.renderer_settings.bvh_sah_bins
	box_threshold_spin.value = PTRenderer.renderer_settings.get_debug_bvh_box_intersections_threshold()
	triangle_threshold_spin.value = PTRenderer.renderer_settings.get_debug_bvh_triangle_intersections_threshold()
	
	depth_spin.value_changed.connect(
		func(value):
			PTRenderer.renderer_settings.bvh_max_depth = value
	)
	
	max_leaf_triangles_spin.value_changed.connect(
		func(value):
			PTRenderer.renderer_settings.bvh_max_triangles_per_leaf = value
	)
	
	sah_bins_spin.value_changed.connect(
		func(value):
			PTRenderer.renderer_settings.bvh_sah_bins = value
	)
	
	box_threshold_spin.value_changed.connect(
		func(value):
			PTRenderer.renderer_settings.set_debug_bvh_box_intersections_threshold(value)
	)
	
	triangle_threshold_spin.value_changed.connect(
		func(value):
			PTRenderer.renderer_settings.set_debug_bvh_triangle_intersections_threshold(value)
	)
