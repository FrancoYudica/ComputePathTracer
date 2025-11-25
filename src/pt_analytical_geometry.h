#ifndef PT_NODE_H
#define PT_NODE_H

#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/ref.hpp>

namespace godot {

    enum PTAnalyticalGeometryType { NODE_TYPE_SPHERE = 0 };

    /**
     * PTAnalyticalGeometry represents an analytical geometry
     * in the scene, such as a sphere. The intersection methods
     * for these geometries are computed analytically, making them
     * efficient for ray tracing.
     */
    class PTAnalyticalGeometry : public Node3D {
        GDCLASS(PTAnalyticalGeometry, Node3D)
    private:
        PTAnalyticalGeometryType node_type = NODE_TYPE_SPHERE;
        Ref<Material> material;

    protected:
        static void _bind_methods();

    public:
        PTAnalyticalGeometry();
        ~PTAnalyticalGeometry();

        void _ready() override;

        void set_material(Ref<Material> p_material);
        Ref<Material> get_material() const;

        void set_node_type(PTAnalyticalGeometryType p_node_type);
        PTAnalyticalGeometryType get_node_type() const;
    };
}  // namespace godot

VARIANT_ENUM_CAST(godot::PTAnalyticalGeometryType);

#endif