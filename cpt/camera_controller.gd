extends Camera3D

const MOUSE_SENSITIVITY = 0.002

# The camera movement speed (tweakable using the mouse wheel).
var move_speed := 0.5

# Stores where the camera is wanting to go (based on pressed keys and speed modifier).
var motion := Vector3()

# Stores the effective camera velocity.
var velocity := Vector3()

# The initial camera node rotation.
var initial_rotation := rotation.y

func _ready() -> void:
	Input.set_mouse_mode(Input.MOUSE_MODE_CAPTURED)

func _input(event: InputEvent) -> void: 
	if event is InputEventMouseMotion:
		# Horizontal mouse look.
		rotation.y -= event.relative.x * MOUSE_SENSITIVITY
		# Vertical mouse look, clamped to -90..90 degrees.
		rotation.x = clamp(rotation.x - event.relative.y * MOUSE_SENSITIVITY, deg_to_rad(-90), deg_to_rad(90))
		
	if event is InputEventKey and event.pressed and event.keycode == KEY_ESCAPE: 
		Input.set_mouse_mode(Input.MOUSE_MODE_VISIBLE)

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
		.rotated(Vector3(0, 1, 0), rotation.y - initial_rotation) \
		.rotated(Vector3(1, 0, 0), cos(rotation.y) * rotation.x) \
		.rotated(Vector3(0, 0, 1), -sin(rotation.y) * rotation.x)

	# Add motion, apply friction and velocity.
	velocity += look_dir * move_speed
	velocity *= 0.9
	position += velocity * delta
