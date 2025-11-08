extends PanelContainer

@export var rasterized_panel: Control

func _on_rasterized_check_box_toggled(toggled_on: bool) -> void:
	rasterized_panel.visible = toggled_on
