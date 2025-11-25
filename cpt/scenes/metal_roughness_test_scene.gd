extends PathTraceScene

@export var size: int = 5
@export var radius: float = 1.0

func _ready() -> void:
	for i in range(size):
		var x = (float(i) - size / 2.0) * radius * 1.5
		for j in range(size):
			var z = (float(j) - size / 2.0) * radius * 1.5
			var sphere = PTAnalyticalGeometry.new()
			sphere.node_type = PTAnalyticalGeometry.NODE_TYPE_SPHERE
			sphere.position.x = x
			sphere.position.z = z
			
			var material = StandardMaterial3D.new()
			material.metallic = float(i) / (size - 1)
			material.roughness = float(j) / (size - 1)
			sphere.material = material
			
			add_child(sphere)
