struct Material {
  vec3 albedo;
  float type; // 0: diffuse, 1: metal, 2: dielectric
  float fuzz; // for metal
  float refractionIndex; // for dielectric
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