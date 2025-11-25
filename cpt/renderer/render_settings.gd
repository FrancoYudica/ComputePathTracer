class_name RenderSettings extends Resource

@export_range(1, 512) var samples_per_pixel: int = 1:
	set(value):
		samples_per_pixel = value
		emit_changed()

@export_range(1, 100) var max_bounces: int = 16:
	set(value):
		max_bounces = value
		emit_changed()

@export_range(0.0, 1.0) var environment_energy: float = 0.25:
	set(value):
		environment_energy = value
		emit_changed()

@export_range(0.0, 1.0) var camera_aperture: float = 0.0:
	set(value):
		camera_aperture = value
		emit_changed()
		
@export_range(0.0, 100.0) var camera_focal_distance: float = 1.0:
	set(value):
		camera_focal_distance = value
		emit_changed()

@export_enum("PathTrace:0", "BVH:1", "Normals:2", "Depth:3", "UV:4") var mode: int = 0:
	set(value):
		mode = value
		emit_changed()
