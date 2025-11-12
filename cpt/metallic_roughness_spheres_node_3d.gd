extends Node3D

@export var size: int = 5
@export var radius: float = 1.0

func _ready() -> void:
	var sphere_scene := load("res://geometric_objects/procedural_sphere.tscn")
	
	for i in range(size):
		var x = (float(i) - size / 2.0) * radius * 1.5
		for j in range(size):
			var z = (float(j) - size / 2.0) * radius * 1.5
			var sphere = sphere_scene.instantiate()
			sphere.position.x = x
			sphere.position.z = z
			sphere.metal = float(i) / (size - 1)
			sphere.roughness = float(j) / (size - 1)
			add_child(sphere)
