class_name CameraManager extends RefCounted

static func get_buffer_size():
	return 16 * 2 * 4

static func get_camera_bytes(camera: Camera3D, width, height) -> PackedByteArray:
	# Build camera matrices
	var view = camera.get_camera_transform().affine_inverse()
	var projection = camera.get_camera_projection() # Scene camera projection. Viewport dependent fov
	
	# Re-calculates projection with scene aspect ratio
	var corrected_projection = Projection.create_perspective(
		camera.fov,
		float(width) / float(height),
		projection.get_z_near(),
		projection.get_z_far()
	)

	var view_floats = transform3d_to_mat4_floats(view)
	var proj_floats = PackedFloat32Array()
	for c in range(4):
		for r in range(4):
			proj_floats.append(corrected_projection[c][r])
			
	# Combine view + projection
	var camera_data = PackedFloat32Array()
	camera_data.append_array(view_floats)
	camera_data.append_array(proj_floats)
	return camera_data.to_byte_array()

# Helper: converts a Transform3D to a 4x4 float array (column-major)
static func transform3d_to_mat4_floats(t: Transform3D) -> PackedFloat32Array:
	var col0: Vector3 = t.basis.x
	var col1: Vector3 = t.basis.y
	var col2: Vector3 = t.basis.z
	var col3: Vector3 = t.origin   # translation (origin)
	# Column-major 4x4: columns 0..3, each has 4 components (x,y,z,w).
	# For the 4th element in each column, use 0 for rotational columns, 1 for translation column.
	var arr := PackedFloat32Array()
	# Column 0
	arr.append(col0.x); arr.append(col0.y); arr.append(col0.z); arr.append(0.0)
	# Column 1
	arr.append(col1.x); arr.append(col1.y); arr.append(col1.z); arr.append(0.0)
	# Column 2
	arr.append(col2.x); arr.append(col2.y); arr.append(col2.z); arr.append(0.0)
	# Column 3 (translation)
	arr.append(col3.x); arr.append(col3.y); arr.append(col3.z); arr.append(1.0)
	return arr
