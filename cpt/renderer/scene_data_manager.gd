class_name SceneDataManager extends RefCounted

var _rd: RenderingDevice
var _tree: SceneTree

# Storage buffers
var _spheres_buffer: RID
var _triangles_buffer: RID
var _vertices_buffer: RID
var _materials_buffer: RID

# Material tracking
var _frame_materials: Dictionary[ObjectMaterial, int]

func _init(rd: RenderingDevice, tree: SceneTree):
	_rd = rd
	_tree = tree

func initialize(buffers: Dictionary):
	_spheres_buffer = buffers["spheres"]
	_triangles_buffer = buffers["triangles"]
	_vertices_buffer = buffers["vertices"]
	_materials_buffer = buffers["materials"]

func update_buffers():
	_frame_materials.clear()
	_update_spheres()
	_update_triangles_and_vertices()
	_update_materials()

func _push_material(material: ObjectMaterial) -> int:
	if not _frame_materials.has(material):
		_frame_materials[material] = _frame_materials.size()
	return _frame_materials[material]

func _update_spheres():
	var SPHERE_FLOATS = 8
	var spheres = _tree.get_nodes_in_group("procedural_sphere").filter(
		func(node: Node3D):
			return node.is_visible_in_tree()
	)

	var spheres_data = PackedFloat32Array()
	spheres_data.resize(spheres.size() * SPHERE_FLOATS + 4)
	spheres_data[0] = spheres.size()
	
	for i in range(spheres.size()):
		var sphere: ProceduralSphere = spheres[i] as ProceduralSphere
		var base_offset = i * SPHERE_FLOATS + 4
		var material_offset = 4
		sphere.load_bytes(spheres_data, base_offset)
		var material_index = _push_material(sphere.object_material)
		spheres_data[base_offset + material_offset] = material_index
		
	var sphere_bytes = spheres_data.to_byte_array()
	_rd.buffer_update(_spheres_buffer, 0, sphere_bytes.size(), sphere_bytes)

func _update_triangles_and_vertices():
	var meshes = _tree.get_nodes_in_group("mesh").filter(
		func(node: Node3D):
			return node.is_visible_in_tree()
	)

	var triangles_data = PackedFloat32Array()
	var vertices_data = PackedVector4Array()
	
	triangles_data.push_back(0) # Triangle count set later
	triangles_data.push_back(0)
	triangles_data.push_back(0)
	triangles_data.push_back(0)
	var total_triangle_count = 0
	
	for i in range(meshes.size()):
		var mesh_instance: MeshInstance3D = meshes[i] as MeshInstance3D
		var mesh = mesh_instance.mesh
		var mesh_material_index = _push_material(mesh_instance.object_material)
		
		for surface_index in mesh.get_surface_count():
			var surface_array = mesh.surface_get_arrays(surface_index)
			var vertices = surface_array[Mesh.ARRAY_VERTEX]
			
			var base_index_offset = vertices_data.size()
			for vertex_index in vertices.size():
				var v = mesh_instance.global_transform * vertices[vertex_index]
				vertices_data.push_back(Vector4(v.x, v.y, v.z, 0.0))
			
			var indices = surface_array[Mesh.ARRAY_INDEX]
			var triangle_count = indices.size() / 3
			total_triangle_count += triangle_count
			
			for triangle_index in triangle_count:
				var i0 = base_index_offset + indices[triangle_index * 3]
				var i1 = base_index_offset + indices[triangle_index * 3 + 1]
				var i2 = base_index_offset + indices[triangle_index * 3 + 2]
				triangles_data.push_back(i0)
				triangles_data.push_back(i1)
				triangles_data.push_back(i2)
				triangles_data.push_back(mesh_material_index)
	
	triangles_data[0] = total_triangle_count
	
	var triangles_bytes = triangles_data.to_byte_array()
	_rd.buffer_update(_triangles_buffer, 0, triangles_bytes.size(), triangles_bytes)

	var vertices_bytes = vertices_data.to_byte_array()
	_rd.buffer_update(_vertices_buffer, 0, vertices_bytes.size(), vertices_bytes)

func _update_materials():
	var materials_data = PackedFloat32Array()
	materials_data.resize(8 * _frame_materials.size())
	var materials := _frame_materials.keys()
	
	for material: ObjectMaterial in materials:
		var material_index = _frame_materials[material]
		var base = material_index * 8
		materials_data[base] = material.material_type
		materials_data[base + 1] = material.metal
		materials_data[base + 2] = material.roughness
		materials_data[base + 3] = material.refraction_index
		materials_data[base + 4] = material.color.r
		materials_data[base + 5] = material.color.g
		materials_data[base + 6] = material.color.b
		materials_data[base + 7] = material.emission
		
	var materials_bytes = materials_data.to_byte_array()
	_rd.buffer_update(_materials_buffer, 0, materials_bytes.size(), materials_bytes)

func cleanup():
	# Buffers are owned by GPUResourceManager, nothing to clean here
	pass
