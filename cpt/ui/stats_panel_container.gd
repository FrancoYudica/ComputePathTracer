extends PanelContainer

@export var fps_label: Label
@export var samples_label: Label
@export var sphere_count_label: Label
@export var triangle_count_label: Label
@export var vertex_count_label: Label
@export var material_count_label: Label
@export var texture_count_label: Label
@export var bvh_node_count_label: Label

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(_delta: float) -> void:
	fps_label.text = "FPS: %s" % roundf(1.0 / _delta)
	var stats = PTRenderer.get_stats()
	samples_label.text = "Samples: %s" % stats.get_samples()
	sphere_count_label.text = "Spheres: %s" % stats.get_sphere_count()
	triangle_count_label.text = "Triangles: %s" % stats.get_triangle_count()
	vertex_count_label.text = "Vertices: %s" % stats.get_vertex_count()
	material_count_label.text = "Materials: %s" % stats.get_material_count()
	texture_count_label.text = "Textures: %s" % stats.get_texture_count()
	bvh_node_count_label.text = "BVH Nodes: %s" % stats.get_bvh_node_count()
