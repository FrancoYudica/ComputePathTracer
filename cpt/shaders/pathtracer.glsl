#[compute]
#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(rgba8, set = 0, binding = 0) uniform image2D out_image;

layout(std430, set = 1, binding = 0) buffer CameraData {
  mat4 view;
  mat4 projection;
}
camera_data;

// Push constants to get image size
layout(push_constant) uniform PushConstants {
  int width;
  int height;
  vec2 pad;
}
params;

struct Ray {
  vec3 origin;
  vec3 direction;
};

Ray generateRay(ivec2 pixel) {
  // 1. Pixel -> NDC (-1, 1)
  vec2 uv = (vec2(pixel) + 0.5) / vec2(params.width, params.height);
  uv.y = 1.0 - uv.y;
  uv = uv * 2.0 - 1.0;

  // 2. Reconstruct world-space point on near plane
  mat4 invViewProj = inverse(camera_data.projection * camera_data.view);
  vec4 ndc = vec4(uv, -1.0, 1.0); // z = -1 = near plane in Vulkan
  vec4 worldPos = invViewProj * ndc;
  worldPos /= worldPos.w;

  // 3. Get camera position in world space
  vec3 origin = inverse(camera_data.view)[3].xyz;

  // 4. Direction from camera to the reconstructed world position
  vec3 direction = normalize(worldPos.xyz - origin);

  Ray ray;
  ray.origin = origin;
  ray.direction = direction;
  return ray;
}

float sphereIntersection(Ray ray) {
  // Simple sphere at (0,0,-1) with radius 0.5
  vec3 sphereCenter = vec3(0.0, 0.0, -1.0);
  float sphereRadius = 0.5;

  vec3 oc = ray.origin - sphereCenter;
  float a = dot(ray.direction, ray.direction);
  float b = 2.0 * dot(oc, ray.direction);
  float c = dot(oc, oc) - sphereRadius * sphereRadius;
  float discriminant = b * b - 4.0 * a * c;

  if (discriminant < 0.0) {
    return -1.0; // No intersection
  } else {
    return (-b - sqrt(discriminant)) / (2.0 * a); // Nearest intersection
  }
}

vec3 traceRay(Ray ray) {
  // Simple gradient based on ray direction
  //   float t = 0.5 * (ray.direction.y + 1.0);
  //   return mix(vec3(1.0, 1.0, 1.0), vec3(0.5, 0.7, 1.0), t);

  float t = sphereIntersection(ray);
  if (t > 0.0) {
    vec3 hitPoint = ray.origin + t * ray.direction;
    return 0.5 * (ray.direction + vec3(1.0));
  } else {
    // Background gradient
    float t = 0.5 * (ray.direction.y + 1.0);
    return mix(vec3(1.0, 1.0, 1.0), vec3(0.5, 0.7, 1.0), t);
  }
}

void main() {
  ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);

  // Don’t go out of bounds
  if (pixel.x >= params.width || pixel.y >= params.height)
    return;

  // Compute normalized coordinates (0.0-1.0)
  Ray ray = generateRay(pixel);
  vec4 color = vec4(traceRay(ray), 1.0);

  // Write pixel color
  imageStore(out_image, pixel, color);
}
