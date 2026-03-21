extends PanelContainer

@export var reload_scene_button: Button
@export var play_stop_button: TextureButton
@export var manager: Node
@export var camera_controller: CameraController

func _ready():
	reload_scene_button.pressed.connect(func(): manager.reload_scene())
	camera_controller.moved.connect(
		func():
			manager.resume()
			play_stop_button.button_pressed = false
	)
	
	play_stop_button.toggled.connect(
		func(toggled_on):
			if toggled_on:
				manager.pause()
			else:
				manager.resume()
	)
