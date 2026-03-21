extends Control

@export var save_file_dialog: FileDialog
@export var sub_viewport: SubViewport
@export var screenshot_button: Button

func _ready() -> void:
	save_file_dialog.file_selected.connect(_file_saved)
	screenshot_button.pressed.connect(save_file_dialog.show)
	
func _on_screenshot_button_pressed() -> void:
	save_file_dialog.show()

func _file_saved(file):
	var texture = sub_viewport.get_texture()
	var image = texture.get_image()
	print(file)
	var status = image.save_png(file)
	if status != OK:
		push_error("Unable to save image at: %s" % file)
	else:
		print("Saved screenshot at: %s" % file)
