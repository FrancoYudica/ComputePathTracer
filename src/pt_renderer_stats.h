
#ifndef PT_RENDERER_STATS_H
#define PT_RENDERER_STATS_H

#include <godot_cpp/classes/resource.hpp>

namespace godot {

    /**
     * Frame stats
     */
    class PTRendererStats : public Resource {
        GDCLASS(PTRendererStats, Resource)

    private:
        uint32_t sphere_count = 0;
        uint32_t triangle_count = 0;
        uint32_t vertex_count = 0;
        uint32_t texture_count = 0;
        uint32_t material_count = 0;
        uint32_t bvh_node_count = 0;

    protected:
        static void _bind_methods();

    public:
        uint32_t get_sphere_count() const { return sphere_count; }
        uint32_t get_triangle_count() const { return triangle_count; }
        uint32_t get_vertex_count() const { return vertex_count; }
        uint32_t get_texture_count() const { return texture_count; }
        uint32_t get_material_count() const { return material_count; }
        uint32_t get_bvh_node_count() const { return bvh_node_count; }

        void set_sphere_count(uint32_t count) { sphere_count = count; }
        void set_triangle_count(uint32_t count) { triangle_count = count; }
        void set_vertex_count(uint32_t count) { vertex_count = count; }
        void set_texture_count(uint32_t count) { texture_count = count; }
        void set_material_count(uint32_t count) { material_count = count; }
        void set_bvh_node_count(uint32_t count) { bvh_node_count = count; }

        void reset();
    };

}  // namespace godot

#endif