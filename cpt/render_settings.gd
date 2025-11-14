class_name RenderSettings extends Resource

@export_range(1, 512) var samples_per_pixel: int = 1:
	set(value):
		samples_per_pixel = value
		emit_changed()

@export_range(0.0, 1.0) var camera_aperture: float = 0.0:
	set(value):
		camera_aperture = value
		emit_changed()
		
@export_range(0.0, 100.0) var camera_focal_distance: float = 1.0:
	set(value):
		camera_focal_distance = value
		emit_changed()
