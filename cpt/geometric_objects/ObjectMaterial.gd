class_name ObjectMaterial extends Resource

enum MaterialType {
	Diffuse,
	Metal,
	Dielectric,
	Emissive
}

@export var color: Color = Color.WHITE
@export var material_type: MaterialType = MaterialType.Diffuse
@export_range(0.0, 1.0) var metal: float = 0.0
@export_range(0.0, 1.0) var roughness: float = 0.0
@export_range(0.0, 3.0) var refraction_index: float = 1.3
@export_range(0.0, 1000.0) var emission: float = 0.0
