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

/**
 * Schlick's approximation for reflectance
 */
float reflectance(float cosine, float refIdx) {
  float r0 = (1.0 - refIdx) / (1.0 + refIdx);
  r0 = r0 * r0;
  return r0 + (1.0 - r0) * pow((1.0 - cosine), 5.0);
}