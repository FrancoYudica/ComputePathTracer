extends Node

signal moved

@export var camera: Camera3D

const MOUSE_SENSITIVITY = 0.002

# The camera movement speed (tweakable using the mouse wheel).
var move_speed := 75

# Stores where the camera is wanting to go (based on pressed keys and speed modifier).
var motion := Vector3()

# Stores the effective camera velocity.
var velocity := Vector3()

# The initial camera node rotation.
@onready var initial_rotation := camera.rotation.y

var _tracking_mouse: bool = false

func _input(event: InputEvent) -> void:
	if event.is_action_released("mouse_down"):
		Input.set_mouse_mode(Input.MOUSE_MODE_VISIBLE)
		_tracking_mouse = false

	if event is InputEventMouseMotion and _tracking_mouse:
		# Horizontal mouse look.
		camera.rotation.y -= event.relative.x * MOUSE_SENSITIVITY
		# Vertical mouse look, clamped to -90..90 degrees.
		camera.rotation.x = clamp(camera.rotation.x - event.relative.y * MOUSE_SENSITIVITY, deg_to_rad(-90), deg_to_rad(90))
		moved.emit()
		
func _unhandled_input(event: InputEvent) -> void: 
	if event.is_action_pressed("mouse_down"):
		Input.set_mouse_mode(Input.MOUSE_MODE_CAPTURED)
		_tracking_mouse = true

func _process(delta: float) -> void:
	motion.x = Input.get_action_strength("move_right") -  Input.get_action_strength("move_left")
	motion.y = Input.get_action_strength("move_up") -  Input.get_action_strength("move_down")
	motion.z = Input.get_action_strength("move_backward") -  Input.get_action_strength("move_forward")
	
	# Normalize motion
	motion = motion.normalized()

	# Speed modifier.
	if Input.is_action_pressed("sprint"):
		motion *= 2

	# Rotate the motion based on the camera angle.
	var look_dir = motion \
		.rotated(Vector3(0, 1, 0), camera.rotation.y - initial_rotation) \
		.rotated(Vector3(1, 0, 0), cos(camera.rotation.y) * camera.rotation.x) \
		.rotated(Vector3(0, 0, 1), -sin(camera.rotation.y) * camera.rotation.x)

	# Add motion, apply friction and velocity.
	velocity += look_dir * move_speed * delta
	velocity *= pow(0.75, delta * 60.0)
	
	if velocity.length_squared() < 0.1:
		velocity = Vector3.ZERO
	
	if velocity.length_squared() > 0 or motion.length_squared() > 0:
		moved.emit()
	
	camera.position += velocity * delta
