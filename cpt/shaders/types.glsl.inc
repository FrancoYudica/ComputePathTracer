struct Ray {
    vec3 origin;
    vec3 direction;
    vec3 invDirection;
};

struct HitRecord {
    float t;
    vec3 p;
    vec3 n;
    vec2 uv;
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
    vec3 albedo;
    float type;  // 0: diffuse, 1: metal, 2: dielectric, 3: emissive
    vec3 emission;
    float metal;
    float roughness;
    float refractionIndex;  // for dielectric

    // Texture indices (0 means default)
    float albedoTextureIndex;
    float metallicTextureIndex;
    float roughnessTextureIndex;
    float emissionTextureIndex;

    // Texture channels
    float metallicTextureChannel;
    float roughnessTextureChannel;
    float emissionEnergyMultiplier;
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
    vec3 emission;
    vec2 uv;
    bool isEmissive;
    Ray ray;
};

struct Vertex {
    vec3 position;
    vec3 color;
    vec3 normal;
    vec2 uv;
};