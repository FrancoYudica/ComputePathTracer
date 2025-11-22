#[compute]
#version 450
#include "types.glsl.inc"

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(rgba8, set = 0, binding = 0) uniform image2D outImage;
layout(rgba32f, set = 0, binding = 1) uniform image2D accumulationImage;
layout(set = 0, binding = 2) uniform sampler2D skybox;

layout(std430, set = 1, binding = 0) buffer Settings {
  float samples;
  float maxBounces;
  float environmentEnergy;
  float cameraAperture;
  float cameraFocalDistance;
  float debugMode; // 0 = off, 1 = visualize BVH
}
settings;

layout(std430, set = 2, binding = 0) buffer CameraData {
  mat4 view;
  mat4 projection;
}
cameraData;


layout(std140, set = 3, binding = 0) buffer Spheres {
  float sphereCount;
  Sphere spheres[];
};

layout(std140, set = 3, binding = 1) buffer Triangles {
  float triangleCount;
  Triangle triangles[];
};

layout(std140, set = 3, binding = 2) buffer Vertices { vec3 vertices[]; };

layout(std140, set = 3, binding = 3) buffer Materials { Material materials[]; };

layout(std140, set = 3, binding = 4) buffer BVHNodes { BVHNode bvhNodes[]; };

// Push constants to get image size
layout(push_constant) uniform PushConstants {
  float width;
  float height;
  float frameNumber; // Ranges in [1, inf)
  float randomFrameSeed2;
}
params;

#include "common.glsl.inc"
#include "random.glsl.inc"
#include "scene.glsl.inc"
#include "debug.glsl.inc"

Ray generateRay(vec2 pixel, uint seed) {
  // 1. Pixel -> NDC (-1, 1)
  vec2 uv = (pixel + 0.5) / vec2(params.width, params.height);
  uv.y = 1.0 - uv.y;
  uv = uv * 2.0 - 1.0;

  // 2. Reconstruct world-space point on near plane
  mat4 invViewProj = inverse(cameraData.projection * cameraData.view);
  vec4 ndc = vec4(uv, -1.0, 1.0); // z = -1 = near plane in Vulkan
  vec4 worldPos = invViewProj * ndc;
  worldPos /= worldPos.w;

  // 3. Get camera position in world space
  vec3 cameraPosition = inverse(cameraData.view)[3].xyz;

  // 4. Direction from camera to the reconstructed world position
  vec3 pinholeDir = normalize(worldPos.xyz - cameraPosition);

  // 5. Depth of field
  // Compute the focal point in world space
  vec3 focalPoint = cameraPosition + pinholeDir * settings.cameraFocalDistance;

  // Sample random point on a unit disk
  vec2 xi = vec2(randomFloat(seed), randomFloat(seed + 19283));
  float r = sqrt(xi.x);
  float theta = 2.0 * 3.14159265 * xi.y;
  vec2 disk = vec2(r * cos(theta), r * sin(theta));

  // Convert disk sample to world space lens position
  // Extract camera basis from inverse(view)
  mat3 camRot = mat3(inverse(cameraData.view));

  vec3 lensOffset = camRot[0] * disk.x * settings.cameraAperture +
                    camRot[1] * disk.y * settings.cameraAperture;

  vec3 lensPos = cameraPosition + lensOffset;

  // Recompute ray direction toward focal point
  vec3 direction = normalize(focalPoint - lensPos);

  return createRay(lensPos, direction);
}

// Non recursive version
vec3 pathTrace(Ray ray, uint seed) {

  HitRecord hitRecord;
  vec3 radiance = vec3(0.0);
  vec3 throughput = vec3(1.0);

  for (int depth = 0; depth < int(settings.maxBounces); depth++) {

    // Generate new seed for each bounce
    seed = pcgHash(seed + uint(depth * 17));

    bool didHit = intersectScene(ray, hitRecord);

    if (!didHit) {
      radiance += sampleSky(ray) * throughput;
      break;
    }

    vec3 hitPoint = rayAt(ray, hitRecord.t);
    vec3 reflectedDir;

    // Fetch material
    Material hitMaterial = materials[int(hitRecord.materialIndex)];

    // Lambertian
    if (hitMaterial.type == 0.0) {

      float metalness = hitMaterial.metal;
      float roughness = hitMaterial.roughness;

      if (randomFloat(seed) > metalness) {
        // Diffuse reflection with roughness
        reflectedDir = normalize(hitRecord.n + randomVec3N(seed));
        throughput *= hitMaterial.albedo * 0.5;

      } else {
        // Specular reflection with roughness
        vec3 perfectReflection = reflect(ray.direction, hitRecord.n);

        // Roughness perturbs the reflection direction
        // roughness = 0: perfect mirror
        // roughness = 1: very rough/blurry reflection
        reflectedDir = normalize(perfectReflection +
                                 roughness * randomVec3N(pcgHash(seed)));

        // Metals tint their reflections
        vec3 specularTint = mix(vec3(1.0), hitMaterial.albedo, metalness);
        throughput *= specularTint;
      }
    }
    // Metallic
    else if (hitMaterial.type == 1.0) {

      // Perfect reflection + some roughness. If roughness is 0, it's a perfect
      // mirror.
      reflectedDir = normalize(reflect(ray.direction, hitRecord.n)) +
                     hitMaterial.roughness * randomVec3N(seed);
      throughput *= hitMaterial.albedo;

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
      reflectedDir = normalize(direction + hitMaterial.roughness *
                                               randomVec3(-1.0, 1.0, seed));
      throughput *= hitMaterial.albedo;
    }

    else if (hitMaterial.type == 3.0) {
      // Emissive material - stop the path and accumulate emission
      radiance += throughput * hitMaterial.albedo * hitMaterial.emission;
      break;
    }

    else {
      return vec3(0.0, 0.0, 0.0); // Black for error
    }
    ray = createRay(hitPoint + reflectedDir * EPSILON, reflectedDir);
  }

  return radiance;
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

  vec3 frameColor = vec3(0.0);

  int debugMode = int(settings.debugMode);

  // Debug modes
  if (debugMode > 0)
  {
    Ray ray = generateRay(pixel, baseSeed);
    switch (debugMode) {
      case 1:
        frameColor =  debugTraceBVH(ray, baseSeed);
        break;

      case 2:
        frameColor = debugNormal(ray, baseSeed);
        break;

      case 3:
        frameColor = debugDepth(ray, baseSeed);
        break;
    }
  }

  // No debug mode. Normal rendering
  else {
    vec3 colorSum = vec3(0.0);
    for (int i = 0; i < int(settings.samples); i++) {
      uint sampleSeed = combineSeed(baseSeed, uint(i));
      vec2 jitteredPixel = vec2(pixel) + randomVec2(0.0, 1.0, sampleSeed);
      Ray ray = generateRay(jitteredPixel, sampleSeed);
      colorSum += pathTrace(ray, sampleSeed);
    }
    frameColor = colorSum / settings.samples;
  }

  // Write pixel color
  vec3 writeColor;

  if (params.frameNumber > 1.0) {
    // Accumulate with previous frames
    vec4 accumulated = imageLoad(accumulationImage, pixel);
    accumulated.rgb += frameColor;
    accumulated.w += 1.0;

    // Write color is the average
    writeColor = accumulated.rgb / accumulated.w;
    imageStore(accumulationImage, pixel, accumulated);
  } else {

    // First frame, initialize accumulation buffer
    imageStore(accumulationImage, pixel, vec4(frameColor, 1.0));
    writeColor = frameColor;
  }

  // Apply gamma only for display
  vec3 displayColor = pow(writeColor, vec3(1.0 / 2.2));
  imageStore(outImage, pixel, vec4(displayColor, 1.0));
}
