float sphereIntersection(Ray ray, Sphere sphere) {
  vec3 oc = ray.origin - sphere.center;
  float a = dot(ray.direction, ray.direction);
  float b = 2.0 * dot(oc, ray.direction);
  float c = dot(oc, oc) - sphere.radius * sphere.radius;
  float discriminant = b * b - 4.0 * a * c;

  if (discriminant < 0.0) {
    return -1.0; // No intersection
  } else {
    return (-b - sqrt(discriminant)) / (2.0 * a); // Nearest intersection
  }
}

vec3 sphereNormal(vec3 point, Sphere sphere) {
  return normalize(point - sphere.center);
}