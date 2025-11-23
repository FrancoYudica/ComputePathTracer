struct Ray {
    vec3 origin;
    vec3 direction;
    vec3 invDirection;
};

struct HitRecord {
    float t;
    vec3 p;
    vec3 n;
    bool frontFace;
    float materialIndex;
    vec3 color;
    bool didHit;
};

struct AABB {
    vec3 min;
    vec3 max;
};

struct BVHNode {
    AABB aabb;
    float primitiveStartIndex;
    float primitiveCount;
    float leftChildIndex;   // 0 when leaf
    float rightChildIndex;  // 0 when leaf
};

struct Material {
    float type;  // 0: diffuse, 1: metal, 2: dielectric, 3: emissive
    float metal;
    float roughness;
    float refractionIndex;  // for dielectric
    vec3 albedo;
    float emission;
};

struct Sphere {
    vec3 center;
    float radius;
    float materialIndex;
};

struct Triangle {
    vec3 indices;  // vertices indices
    float materialIndex;
};

struct MaterialSample {
    vec3 albedo;
    bool isEmissive;
    Ray ray;
};

struct Vertex {
    vec3 position;
    vec3 color;
    vec3 normal;
};