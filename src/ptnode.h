#ifndef PT_NODE_H
#define PT_NODE_H

#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/ref.hpp>

#include "ptmaterial.h"

namespace godot {

    enum PTNodeType { NODE_TYPE_TRIANGLE_MESH = 0, NODE_TYPE_SPHERE = 1 };

    class PTNode : public MeshInstance3D {
        GDCLASS(PTNode, MeshInstance3D)
    private:
        PTNodeType node_type = NODE_TYPE_TRIANGLE_MESH;
        Ref<PTMaterial> material;

    protected:
        static void _bind_methods();

    public:
        PTNode();
        ~PTNode();

        void _ready() override;

        void set_material(Ref<PTMaterial> p_material);
        Ref<PTMaterial> get_material() const;

        void set_node_type(PTNodeType p_node_type);
        PTNodeType get_node_type() const;
    };
}  // namespace godot

VARIANT_ENUM_CAST(godot::PTNodeType);

#endif