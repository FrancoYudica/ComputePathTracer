extends Node

@export var renderer: PTRenderer

var _texture: Texture2DRD

func _ready() -> void:
	_texture = Texture2DRD.new()
	get_parent().texture = _texture
	renderer.connect("texture_changed", _change_texture)
	
func _change_texture(texture_rid: RID):
	_texture.texture_rd_rid = RID()
	_texture.texture_rd_rid = texture_rid
