class_name ProceduralSphere extends GeometricObject

enum MaterialType {
	Diffuse,
	Metal,
	Dielectric,
	Emissive
}

@export var color: Color = Color.WHITE
@export var material_type: MaterialType
@export_range(0.0, 1.0) var fuzz: float = 0.0
@export_range(0.0, 3.0) var refraction_index: float = 1.3
@export_range(1.0, 100.0) var emission: float = 0.0

func load_bytes(_byte_array: PackedFloat32Array, offset: int) -> void:
	_byte_array[offset + 0] = position.x
	_byte_array[offset + 1] = position.y
	_byte_array[offset + 2] = position.z
	_byte_array[offset + 3] = scale.x * 0.5
