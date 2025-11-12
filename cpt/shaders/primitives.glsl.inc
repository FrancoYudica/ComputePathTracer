struct Material {
  float type; // 0: diffuse, 1: metal, 2: dielectric, 3: emissive
  float metal;
  float roughness; 
  float refractionIndex; // for dielectric
  vec3 albedo;
  float emission;
};

struct Ray {
  vec3 origin;
  vec3 direction;
};

struct Sphere {
  vec3 center;
  float radius;
  float materialIndex;
};