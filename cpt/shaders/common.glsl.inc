#define EPSILON 0.0001

struct HitRecord {
  float t;
  vec3 p;
  vec3 n;
  bool frontFace;
  float materialIndex;
};

vec3 rayAt(Ray ray, float t) { return ray.origin + ray.direction * t; }

bool sphereIntersection(Ray ray, Sphere sphere, out HitRecord record) {
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
  record.t = root;
  record.p = rayAt(ray, root);
  record.n = (record.p - sphere.center) / sphere.radius;
  record.materialIndex = sphere.materialIndex;
  // set frontFace so the normal always points against the incoming ray
  record.frontFace = dot(ray.direction, record.n) < 0.0;
  if (!record.frontFace) {
    record.n = -record.n;
  }

  return true;
}

bool triangleIntersection(
  Ray ray, 
  vec3 v0,
  vec3 v1,
  vec3 v2,
  float materialIndex,
  out HitRecord record) {
  vec3 edge1 = v1 - v0;
  vec3 edge2 = v2 - v0;
  vec3 h = cross(ray.direction, edge2);
  float a = dot(edge1, h);
  if (abs(a) < EPSILON) {
    return false; // Ray is parallel to triangle
  }
  float f = 1.0 / a;
  vec3 s = ray.origin - v0;
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
  if (t < EPSILON) {
    return false; // Intersection behind ray origin
  }
  
  record.t = t;
  record.p = rayAt(ray, t);
  record.n = normalize(cross(edge1, edge2));
  record.materialIndex = materialIndex;
  
  // set frontFace so the normal always points against the incoming ray
  record.frontFace = dot(ray.direction, record.n) < 0.0;
  if (!record.frontFace) {
    record.n = -record.n;
  }

  return true;
}

/**
 * Schlick's approximation for reflectance
 */
float reflectance(float cosine, float refIdx) {
  float r0 = (1.0 - refIdx) / (1.0 + refIdx);
  r0 = r0 * r0;
  return r0 + (1.0 - r0) * pow((1.0 - cosine), 5.0);
}