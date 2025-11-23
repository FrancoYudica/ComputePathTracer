#ifndef PT_MATERIAL_H
#define PT_MATERIAL_H

#include <godot_cpp/classes/resource.hpp>

namespace godot {

    enum MaterialType {
        MATERIAL_TYPE_LAMBERTIAN = 0,
        MATERIAL_TYPE_METAL = 1,
        MATERIAL_TYPE_DIELECTRIC = 2,
        MATERIAL_TYPE_EMISSIVE = 3,
    };

    class PTMaterial : public Resource {
        GDCLASS(PTMaterial, Resource)
    private:
        MaterialType material_type = MATERIAL_TYPE_LAMBERTIAN;
        Color color;
        float metallic;
        float roughness;
        float refraction_index;
        float emission;

    protected:
        static void _bind_methods();

    public:
        PTMaterial();
        ~PTMaterial() {}

        void set_color(const Color& p_color);
        Color get_color() const;

        void set_metallic(float p_metallic);
        float get_metallic() const;

        void set_roughness(float p_roughness);
        float get_roughness() const;

        void set_refraction_index(float p_refraction_index);
        float get_refraction_index() const;

        void set_emission(float p_emission);
        float get_emission() const;

        MaterialType get_material_type() const;
        void set_material_type(MaterialType p_material_type);

        size_t get_hash() const;

        String to_string() const;
    };
}  // namespace godot

VARIANT_ENUM_CAST(godot::MaterialType);

#endif