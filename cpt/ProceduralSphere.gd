class_name ProceduralSphere extends GeometricObject


func load_bytes(_byte_array: PackedFloat32Array, offset: int) -> void:
	_byte_array[offset + 0] = global_position.x
	_byte_array[offset + 1] = global_position.y
	_byte_array[offset + 2] = global_position.z
	_byte_array[offset + 3] = scale.x * 0.5
