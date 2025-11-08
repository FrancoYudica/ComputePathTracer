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
  float width;
  float height;
  float randomFrameSeed1;
  float randomFrameSeed2;
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
bool intersectScene(Ray ray, out HitRecord nearestHit) {
  nearestHit.t = -1.0;
  Sphere sphere;
  bool didHit = false;
  for (int i = 0; i < int(sphereCount); i++) {
    sphere = spheres[i];
    HitRecord record;
    bool h = sphereIntersection(ray, sphere, record);
    if (h && (record.t < nearestHit.t || nearestHit.t < 0.0)) {
      nearestHit = record;
      didHit = true;
    }
  }
  return didHit;
}

vec3 sampleSky(Ray ray) {
  float t = 0.5 * (ray.direction.y + 1.0);
  return mix(vec3(1.0, 1.0, 1.0), vec3(0.5, 0.7, 1.0), t);
}

// Non recursive version
vec3 pathTrace(Ray ray, float seed) {

  HitRecord hitRecord;
  int depth = 5;
  vec3 accumulatedColor = vec3(1.0);
  while (depth-- > 0) {

    // Better seed variation: use pixel ID, frame seed, AND depth
    seed += float(depth);

    bool didHit = intersectScene(ray, hitRecord);

    if (!didHit) {
      accumulatedColor *= sampleSky(ray);
      break;
    }

    vec3 hitPoint = rayAt(ray, hitRecord.t);
    vec3 reflectedDir = normalize(randomInHemisphereVec3(hitRecord.n, seed));
    ray = Ray(hitPoint + hitRecord.n * EPSILON, reflectedDir);
    vec3 localColor = vec3(1.0, 1.0, 1.0);
    accumulatedColor *= localColor * 0.5;
  }

  return accumulatedColor;
}

void main() {
  ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);

  // Don’t go out of bounds
  if (pixel.x >= params.width || pixel.y >= params.height)
    return;

  // Compute normalized coordinates (0.0-1.0)
  Ray ray = generateRay(pixel);
  float seed = mod(float(pixel.x) + float(pixel.y) * params.width +
                       float(params.randomFrameSeed1),
                   params.randomFrameSeed2);

  int samples = 1;
  vec3 colorSum = vec3(0.0);
  for (int i = 0; i < samples; i++) {
    seed += float(i);
    colorSum += pathTrace(ray, seed);
  }

  vec4 color = vec4(colorSum / float(samples), 1.0);

  // Write pixel color
  imageStore(out_image, pixel, color);
}
