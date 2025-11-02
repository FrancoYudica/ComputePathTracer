extends Node

@export var renderer: Renderer

var _texture: Texture2DRD

func _ready() -> void:
	_texture = Texture2DRD.new()
	get_parent().texture = _texture
	RenderingServer.connect("frame_pre_draw", _pre_draw)
	
func _pre_draw():
	_texture.texture_rd_rid = renderer.get_texture_rid()
