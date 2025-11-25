extends PanelContainer

@export var renderer: Renderer
@export var fps_label: Label
@export var sphere_count_label: Label
@export var triangle_count_label: Label
@export var vertex_count_label: Label
@export var material_count_label: Label
@export var texture_count_label: Label


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(_delta: float) -> void:
	fps_label.text = "FPS: %s" % roundf(1.0 / _delta)
	sphere_count_label.text = "Spheres: %s" % renderer.scene_data_manager.get_sphere_count()
	triangle_count_label.text = "Triangles: %s" % renderer.scene_data_manager.get_triangle_count()
	vertex_count_label.text = "Vertices: %s" % renderer.scene_data_manager.get_vertex_count()
	material_count_label.text = "Materials: %s" % renderer.scene_data_manager.get_material_count()
	texture_count_label.text = "Textures: %s" % renderer.scene_data_manager.get_texture_count()
