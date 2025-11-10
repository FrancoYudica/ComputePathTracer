extends PanelContainer

@export var rasterized_panel: Control
@export var renderer: Renderer
@export var rendered_sub_viewport_container: SubViewportContainer
@export var camera_controller: Node
@export var fov_label: Label

func _on_rasterized_check_box_toggled(toggled_on: bool) -> void:
	rasterized_panel.visible = toggled_on


func _on_samples_spin_box_value_changed(value: float) -> void:
	renderer.samples = int(value)


func _on_render_scale_spin_box_value_changed(value: float) -> void:
	rendered_sub_viewport_container.stretch_shrink = int(1.0 / (value / 100))


func _on_fov_h_slider_value_changed(value: float) -> void:
	camera_controller.fov = value
