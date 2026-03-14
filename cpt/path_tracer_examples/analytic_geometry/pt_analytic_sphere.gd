extends Node3D

@export var material: BaseMaterial3D


func _ready() -> void:
	
	# Hide mesh, just use on the editor
	$MeshInstance3D.visible = false

	var mesh: SphereMesh = $MeshInstance3D.mesh as SphereMesh
	
	if mesh == null:
		return
		
	$PTAnalyticalGeometry.material = material
