#[compute]
#version 450

#include "primitives.glsl.inc"

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(rgba8, set = 0, binding = 0) uniform image2D out_image;

layout(std430, set = 1, binding = 0) buffer CameraData {
  mat4 view;
  mat4 projection;
}
camera_data;

layout(std140, set = 2, binding = 0) buffer Spheres {
  float sphereCount;
  Sphere spheres[];
};

// Push constants to get image size
layout(push_constant) uniform PushConstants {
  int width;
  int height;
  vec2 pad;
}
params;

#include "common.glsl.inc"

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

/**
Iterates through all the scene and finds the closest intersection.
*/
float intersectScene(Ray ray, out vec3 normal) {
  float closestT = -1.0;
  float t;
  Sphere sphere;
  for (int i = 0; i < int(sphereCount); i++) {
    sphere = spheres[i];

    t = sphereIntersection(ray, sphere);
    if (t > 0.0 && (t < closestT || closestT < 0.0)) {
      closestT = t;
      normal = sphereNormal(ray.origin + t * ray.direction, sphere);
    }
  }
  return closestT;
}

vec3 sampleSky(Ray ray) {
  float t = 0.5 * (ray.direction.y + 1.0);
  return mix(vec3(1.0, 1.0, 1.0), vec3(0.5, 0.7, 1.0), t);
}

vec3 pathTrace(Ray ray) {

  int maxDepth = 5;
  vec3 color = vec3(0.0);
  float t;
  vec3 normal;
  float accumulationFactor = 1.0;
  bool escaped = false;

  while (maxDepth-- > 0) {

    t = intersectScene(ray, normal);

    if (t < 0.0) {
      // No intersection, sample sky and terminate.
      // Multiply by accumulationFactor so deeper bounces contribute less.
      color += sampleSky(ray) * accumulationFactor;
      escaped = true;
      break;
    }

    vec3 hitPoint = ray.origin + t * ray.direction;
    float diffuse = max(dot(normal, normalize(vec3(1.0, 1.0, 1.0))), 0.0);
    vec3 reflectedDir = reflect(ray.direction, normal);
    vec3 localColor = diffuse * vec3(1.0, 0.5, 0.3);
    color += localColor * accumulationFactor;

    // Update ray for next bounce
    ray = Ray(hitPoint + normal * 0.001, reflectedDir);

    accumulationFactor *= 0.5; // Simple energy loss
  }

  // If we exited the loop because we reached max depth (not because we hit the
  // sky), we should also account for the remaining un-terminated throughput by
  // sampling the environment along the current ray with the leftover
  // accumulationFactor.
  if (!escaped) {
    color += sampleSky(ray) * accumulationFactor;
  }

  return color;
}

void main() {
  ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);

  // Don’t go out of bounds
  if (pixel.x >= params.width || pixel.y >= params.height)
    return;

  // Compute normalized coordinates (0.0-1.0)
  Ray ray = generateRay(pixel);
  vec4 color = vec4(pathTrace(ray), 1.0);

  // Write pixel color
  imageStore(out_image, pixel, color);
}
