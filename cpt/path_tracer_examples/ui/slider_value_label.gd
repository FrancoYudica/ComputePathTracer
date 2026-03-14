extends Label

@export var slider: Slider
@export var suffix: String = ""
@export var integer: bool = false

func _ready() -> void:
	slider.value_changed.connect(_set_value)
	_set_value(slider.value)

func round_to_dec(num, digit):
	return round(num * pow(10.0, digit)) / pow(10.0, digit)

func _set_value(value):
	text = str(value if not integer else int(value)) + suffix
