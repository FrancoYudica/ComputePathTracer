#[compute]
#version 450

#include "primitives.glsl.inc"

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(rgba8, set = 0, binding = 0) uniform image2D out_image;
layout(rgba32f, set = 1, binding = 0) uniform image2D accumulation_image;

layout(std430, set = 2, binding = 0) buffer CameraData {
  mat4 view;
  mat4 projection;
}
camera_data;

layout(std140, set = 3, binding = 0) buffer Spheres {
  float sphereCount;
  Sphere spheres[];
};

layout(std140, set = 3, binding = 1) buffer Materials { Material materials[]; };

// Push constants to get image size
layout(push_constant) uniform PushConstants {
  float width;
  float height;
  float frameNumber; // Ranges in [1, inf)
  float randomFrameSeed2;
  float samples;
  float frameWeight;
}
params;

#include "common.glsl.inc"
#include "random.glsl.inc"

Ray generateRay(vec2 pixel) {
  // 1. Pixel -> NDC (-1, 1)
  vec2 uv = (pixel + 0.5) / vec2(params.width, params.height);
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
vec3 pathTrace(Ray ray, uint seed) {

  HitRecord hitRecord;
  int depth = 15;
  vec3 accumulatedColor = vec3(1.0);

  while (depth-- > 0) {

    // Generate new seed for each bounce
    seed = pcgHash(seed + uint(depth * 17));

    bool didHit = intersectScene(ray, hitRecord);

    if (!didHit) {
      accumulatedColor *= sampleSky(ray);
      break;
    }

    vec3 hitPoint = rayAt(ray, hitRecord.t);
    vec3 reflectedDir;

    // Fetch material
    Material hitMaterial = materials[int(hitRecord.materialIndex)];

    float energyLoss = 0.5;

    // Lambertian
    if (hitMaterial.type == 0.0) {
      reflectedDir = hitRecord.n + randomVec3N(seed);
    }
    // Metallic
    else if (hitMaterial.type == 1.0) {

      // Perfect reflection + some fuzziness. If fuzziness is 0, it's a perfect
      // mirror.
      reflectedDir = normalize(reflect(ray.direction, hitRecord.n)) +
                     hitMaterial.fuzz * randomVec3(-1.0, 1.0, seed);

    }
    // Dielectric
    else if (hitMaterial.type == 2.0) {
      // Refraction ratio
      float ri = hitRecord.frontFace ? (1.0 / hitMaterial.refractionIndex)
                                     : hitMaterial.refractionIndex;

      vec3 unitDirection = normalize(ray.direction);

      float cosTheta = min(dot(-unitDirection, hitRecord.n), 1.0);
      float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

      bool cannotRefract = ri * sinTheta > 1.0;
      vec3 direction;

      // Schlick approximation for reflectance
      float reflectProb = reflectance(cosTheta, ri);

      if (cannotRefract || reflectProb > randomFloat(0.0, 1.0, seed)) {
        direction = reflect(unitDirection, hitRecord.n);
      } else {
        direction = refract(unitDirection, hitRecord.n, ri);
      }
      reflectedDir =
          normalize(direction + hitMaterial.fuzz * randomVec3(-1.0, 1.0, seed));

      energyLoss = 1.0; // No energy loss for dielectrics
    }

    else if (hitMaterial.type == 3.0) {
      // Emissive material - stop the path and accumulate emission
      accumulatedColor *= hitMaterial.albedo * hitMaterial.emission;
      break;
    }

    else {
      return vec3(0.0, 0.0, 0.0); // Black for error
    }
    ray = Ray(hitPoint + reflectedDir * EPSILON, reflectedDir);

    vec3 localColor = hitMaterial.albedo;
    accumulatedColor *= localColor * energyLoss;
  }

  return accumulatedColor;
}

void main() {
  ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);

  // Don’t go out of bounds
  if (pixel.x >= params.width || pixel.y >= params.height)
    return;

  // Convert frame seeds to uints for better hashing
  uint frameSeed1 = uint(params.frameNumber);
  uint frameSeed2 = uint(params.randomFrameSeed2);
  uint pixelIndex = uint(pixel.x) + uint(pixel.y) * uint(params.width);

  // Initial seed combining pixel location and frame seeds
  uint baseSeed = combineSeed(pixelIndex, frameSeed1, frameSeed2);

  vec3 colorSum = vec3(0.0);
  for (int i = 0; i < int(params.samples); i++) {

    uint sampleSeed = combineSeed(baseSeed, uint(i));

    vec2 jitteredPixel = vec2(pixel) + randomVec2(0.0, 1.0, sampleSeed);
    Ray ray = generateRay(jitteredPixel);
    colorSum += pathTrace(ray, sampleSeed);
  }

  vec3 frameColor = colorSum / params.samples;

  // Gamma correction 2.2
  frameColor = pow(frameColor, vec3(1.0 / 2.2)); // Gamma correction

  // Write pixel color
  vec3 writeColor = frameColor;

  if (params.frameNumber > 1.0) {
    vec3 existingColor = imageLoad(accumulation_image, pixel).rgb;
    writeColor = existingColor * (1.0 - params.frameWeight) +
                 frameColor * params.frameWeight;
  }

  // Write to accumulation buffer and output image
  imageStore(accumulation_image, pixel, vec4(writeColor, 1.0));
  imageStore(out_image, pixel, vec4(writeColor, 1.0));
}
