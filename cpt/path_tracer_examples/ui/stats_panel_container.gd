extends PanelContainer

@export var fps_label: Label
@export var samples_label: Label
@export var sphere_count_label: Label
@export var triangle_count_label: Label
@export var vertex_count_label: Label
@export var material_count_label: Label
@export var texture_count_label: Label
@export var bvh_node_count_label: Label

var _current_stats: PTRendererStats

func _process(delta: float) -> void:
	fps_label.text = "FPS: %d" % roundi(1.0 / delta)

## Call this to set or swap the stats object
func set_stats(stats: PTRendererStats) -> void:
	# Cleanup previous connection
	if _current_stats != null and _current_stats.changed.is_connected(_update_ui):
		_current_stats.changed.disconnect(_update_ui)
	
	_current_stats = stats
	
	if _current_stats != null:
		# Connect to the "changed" signal (emitted by C++ side)
		_current_stats.changed.connect(_update_ui)
		# Initial update
		_update_ui()

## Named method for updating all labels at once
func _update_ui() -> void:
	if _current_stats == null:
		return
		
	samples_label.text = "Samples: %d" % _current_stats.get_samples()
	sphere_count_label.text = "Spheres: %d" % _current_stats.get_sphere_count()
	triangle_count_label.text = "Triangles: %d" % _current_stats.get_triangle_count()
	vertex_count_label.text = "Vertices: %d" % _current_stats.get_vertex_count()
	material_count_label.text = "Materials: %d" % _current_stats.get_material_count()
	texture_count_label.text = "Textures: %d" % _current_stats.get_texture_count()
	bvh_node_count_label.text = "BVH Nodes: %d" % _current_stats.get_bvh_node_count()
