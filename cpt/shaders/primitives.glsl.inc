struct Material {
  float type; // 0: diffuse, 1: metal, 2: dielectric, 3: emissive
  float emission;
  float fuzz; // for metal
  float refractionIndex; // for dielectric
  vec3 albedo;
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