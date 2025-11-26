vec3 rayAt(Ray ray, float t) { return ray.origin + ray.direction * t; }

/* Standard way of creating a ray */
Ray createRay(vec3 origin, vec3 direction) {
    Ray ray;
    ray.origin = origin;
    ray.direction = direction;
    ray.invDirection = 1.0 / ray.direction;
    return ray;
}

HitRecord createHitRecord() {
    HitRecord record;
    record.t = MAX_FLOAT;
    record.didHit = false;
    record.color = vec3(1.0);
    return record;
}

bool sphereIntersection(Ray ray, Sphere sphere, inout HitRecord record) {
    vec3 oc = ray.origin - sphere.center;
    float a = dot(ray.direction, ray.direction);
    float half_b = dot(ray.direction, oc);
    float c = dot(oc, oc) - sphere.radius * sphere.radius;
    float discriminant = half_b * half_b - a * c;

    if (discriminant < 0.0) {
        return false;
    }

    float sqrtd = sqrt(discriminant);

    float root = (-half_b - sqrtd) / a;
    if (root < 0.0) {
        root = (-half_b + sqrtd) / a;

        if (root < 0.0) {
            return false;
        }
    }

    if (root < EPSILON || root > record.t) {
        return false;  // Intersection behind ray origin
    }

    record.t = root;
    record.p = rayAt(ray, root);
    record.n = (record.p - sphere.center) / sphere.radius;
    record.materialIndex = sphere.materialIndex;
    // set frontFace so the normal always points against the incoming ray
    record.frontFace = dot(ray.direction, record.n) < 0.0;
    if (!record.frontFace) {
        record.n = -record.n;
    }
    record.didHit = true;

    return true;
}

// Ray-AABB intersection test, by the slab method.
bool aabbIntersection(Ray ray, AABB aabb) {
    float tx1 = (aabb.min.x - ray.origin.x) * ray.invDirection.x;
    float tx2 = (aabb.max.x - ray.origin.x) * ray.invDirection.x;

    float tmin = min(tx1, tx2);
    float tmax = max(tx1, tx2);

    float ty1 = (aabb.min.y - ray.origin.y) * ray.invDirection.y;
    float ty2 = (aabb.max.y - ray.origin.y) * ray.invDirection.y;

    tmin = max(tmin, min(ty1, ty2));
    tmax = min(tmax, max(ty1, ty2));

    float tz1 = (aabb.min.z - ray.origin.z) * ray.invDirection.z;
    float tz2 = (aabb.max.z - ray.origin.z) * ray.invDirection.z;

    tmin = max(tmin, min(tz1, tz2));
    tmax = min(tmax, max(tz1, tz2));

    return tmax >= max(0.0, tmin);
}

bool triangleIntersectionFlat(Ray ray, Vertex v0, Vertex v1, Vertex v2,
                              float materialIndex, inout HitRecord record) {
    vec3 edge1 = v1.position.xyz - v0.position.xyz;
    vec3 edge2 = v2.position.xyz - v0.position.xyz;
    vec3 h = cross(ray.direction, edge2);
    float a = dot(edge1, h);

    if (abs(a) < EPSILON) {
        return false;
    }

    float f = 1.0 / a;
    vec3 s = ray.origin - v0.position.xyz;
    float u = f * dot(s, h);

    if (u < 0.0 || u > 1.0) {
        return false;
    }

    vec3 q = cross(s, edge1);
    float v = f * dot(ray.direction, q);

    if (v < 0.0 || u + v > 1.0) {
        return false;
    }

    float t = f * dot(edge2, q);

    if (t < EPSILON || t > record.t) {
        return false;
    }

    record.t = t;
    record.p = rayAt(ray, t);
    record.materialIndex = materialIndex;

    // Use geometric normal (no interpolation)
    record.n = normalize(cross(edge1, edge2));

    record.frontFace = dot(ray.direction, record.n) < 0.0;
    if (!record.frontFace) {
        record.n = -record.n;
    }

    record.didHit = true;
    return true;
}

void triangleIntersection(Ray ray, Vertex v0, Vertex v1, Vertex v2,
                          float materialIndex, inout HitRecord record) {
    vec3 edge1 = v1.position - v0.position;
    vec3 edge2 = v2.position - v0.position;

    vec3 h = cross(ray.direction, edge2);
    float a = dot(edge1, h);

    // Check if ray is parallel to triangle
    if (abs(a) < 1e-8) return;

    float f = 1.0 / a;
    vec3 s = ray.origin - v0.position;
    float u = f * dot(s, h);

    // Check u bounds
    if (u < 0.0 || u > 1.0) return;

    vec3 q = cross(s, edge1);
    float v = f * dot(ray.direction, q);

    // Check v bounds and u+v
    if (v < 0.0 || u + v > 1.0) return;

    float t = f * dot(edge2, q);

    // Check if intersection is valid
    if (t <= EPSILON || t >= record.t) return;

    // Valid intersection
    record.t = t;
    record.p = ray.origin + ray.direction * t;
    record.materialIndex = materialIndex;
    record.didHit = true;

    // Interpolate normal (u and v are barycentric coords beta and gamma)
    float w = 1.0 - u - v;  // alpha
    record.n = normalize(w * v0.normal + u * v1.normal + v * v2.normal);
    record.color = w * v0.color + u * v1.color + v * v2.color;
    record.uv = w * v0.uv + u * v1.uv + v * v2.uv;

    // Set front face. Uses geometric normal
    vec3 geometricNormal = -cross(edge1, edge2);
    record.frontFace = dot(ray.direction, geometricNormal) < 0.0;
    if (!record.frontFace) {
        record.n = -record.n;
    }
}

/**
 * Schlick's approximation for reflectance
 */
float reflectance(float cosine, float refIdx) {
    float r0 = (1.0 - refIdx) / (1.0 + refIdx);
    r0 = r0 * r0;
    return r0 + (1.0 - r0) * pow((1.0 - cosine), 5.0);
}

vec3 reflectance(float cosine, vec3 refIdx) {
    vec3 r0 = (1.0 - refIdx) / (1.0 + refIdx);
    r0 = r0 * r0;
    return r0 + (1.0 - r0) * pow((1.0 - cosine), 5.0);
}

Material getMaterial(float materialIndex) {
    return materials[int(materialIndex)];
}

vec3 getMaterialAlbedo(Material mat, vec2 uv) {
    return mat.albedo *
           texture(textureArray, vec3(uv.x, uv.y, mat.albedoTextureIndex)).rgb;
}