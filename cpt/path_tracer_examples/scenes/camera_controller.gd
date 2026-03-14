class_name CameraController extends Node

signal moved


const MOUSE_SENSITIVITY = 0.002

# The camera movement speed (tweakable using the mouse wheel).
var move_speed := 75

# Stores where the camera is wanting to go (based on pressed keys and speed modifier).
var motion := Vector3()

# Stores the effective camera velocity.
var velocity := Vector3()

@export var camera: Camera3D

var _tracking_mouse: bool = false

var fov: float:
	get:
		return fov
	set(value):
		fov = value
		camera.fov = fov
		moved.emit()

func _input(event: InputEvent) -> void:
	if event.is_action_released("mouse_down"):
		Input.set_mouse_mode(Input.MOUSE_MODE_VISIBLE)
		_tracking_mouse = false
		
func _unhandled_input(event: InputEvent) -> void: 
	if event.is_action_pressed("mouse_down"):
		Input.set_mouse_mode(Input.MOUSE_MODE_CAPTURED)
		_tracking_mouse = true

	if event is InputEventMouseMotion and _tracking_mouse:
		# Horizontal mouse look.
		camera.rotation.y -= event.relative.x * MOUSE_SENSITIVITY
		# Vertical mouse look, clamped to -90..90 degrees.
		camera.rotation.x = clamp(camera.rotation.x - event.relative.y * MOUSE_SENSITIVITY, deg_to_rad(-90), deg_to_rad(90))
		moved.emit()

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
	var yaw := camera.rotation.y
	var pitch := camera.rotation.x
	var look_dir = motion \
		.rotated(Vector3(0, 1, 0), yaw) \
		.rotated(Vector3(1, 0, 0), cos(yaw) * pitch) \
		.rotated(Vector3(0, 0, 1), -sin(yaw) * pitch)

	# Add motion, apply friction and velocity.
	velocity += look_dir * move_speed * delta
	velocity *= pow(0.75, delta * 60.0)
	
	if velocity.length_squared() < 0.1:
		velocity = Vector3.ZERO
	
	if velocity.length_squared() > 0 or motion.length_squared() > 0:
		moved.emit()
	
	camera.position += velocity * delta
