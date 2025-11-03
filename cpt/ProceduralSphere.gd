class_name ProceduralSphere extends GeometricObject

func load_bytes(_byte_array: PackedFloat32Array, offset: int) -> void:
	_byte_array[offset + 0] = position.x
	_byte_array[offset + 1] = position.y
	_byte_array[offset + 2] = position.z
	_byte_array[offset + 3] = scale.x * 0.5
