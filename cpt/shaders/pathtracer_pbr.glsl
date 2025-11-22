#[compute]
#version 450

#include "primitives.glsl.inc"

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(rgba8, set = 0, binding = 0) uniform image2D outImage;

layout(rgba32f, set = 1, binding = 0) uniform image2D accumulationImage;

layout(std430, set = 2, binding = 0) buffer CameraData {
    mat4 view;
    mat4 projection;
    float aperture;
    float focalDistance;
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

// Push constants to get image size
layout(push_constant) uniform PushConstants {
    float width;
    float height;
    float frameNumber;  // Ranges in [1, inf)
    float randomFrameSeed2;
    float samples;
    float frameWeight;
}
params;

#include "common.glsl.inc"
#include "pbr.glsl.inc"
#include "random.glsl.inc"

Ray generateRay(vec2 pixel, uint seed) {
    // 1. Pixel -> NDC (-1, 1)
    vec2 uv = (pixel + 0.5) / vec2(params.width, params.height);
    uv.y = 1.0 - uv.y;
    uv = uv * 2.0 - 1.0;

    // 2. Reconstruct world-space point on near plane
    mat4 invViewProj = inverse(cameraData.projection * cameraData.view);
    vec4 ndc = vec4(uv, -1.0, 1.0);  // z = -1 = near plane in Vulkan
    vec4 worldPos = invViewProj * ndc;
    worldPos /= worldPos.w;

    // 3. Get camera position in world space
    vec3 cameraPosition = inverse(cameraData.view)[3].xyz;

    // 4. Direction from camera to the reconstructed world position
    vec3 pinholeDir = normalize(worldPos.xyz - cameraPosition);

    // 5. Depth of field
    // Compute the focal point in world space
    vec3 focalPoint = cameraPosition + pinholeDir * cameraData.focalDistance;

    // Sample random point on a unit disk
    vec2 xi = vec2(randomFloat(seed), randomFloat(seed + 19283));
    float r = sqrt(xi.x);
    float theta = 2.0 * 3.14159265 * xi.y;
    vec2 disk = vec2(r * cos(theta), r * sin(theta));

    // Convert disk sample to world space lens position
    // Extract camera basis from inverse(view)
    mat3 camRot = mat3(inverse(cameraData.view));

    vec3 lensOffset = camRot[0] * disk.x * cameraData.aperture +
                      camRot[1] * disk.y * cameraData.aperture;

    vec3 lensPos = cameraPosition + lensOffset;

    // Recompute ray direction toward focal point
    vec3 direction = normalize(focalPoint - lensPos);

    Ray ray;
    ray.origin = lensPos;
    ray.direction = direction;
    return ray;
}

/**
Iterates through all the scene and finds the closest intersection.
*/
bool intersectScene(Ray ray, out HitRecord nearestHit) {
    nearestHit.t = -1.0;
    Sphere sphere;
    Triangle triangle;
    bool didHit = false;
    HitRecord record;

    for (int i = 0; i < int(sphereCount); i++) {
        sphere = spheres[i];
        bool h = sphereIntersection(ray, sphere, record);
        if (h && (record.t < nearestHit.t || nearestHit.t < 0.0)) {
            nearestHit = record;
            didHit = true;
        }
    }

    for (int i = 0; i < int(triangleCount); i++) {
        triangle = triangles[i];
        vec3 v0 = vertices[int(triangle.indices.x)];
        vec3 v1 = vertices[int(triangle.indices.y)];
        vec3 v2 = vertices[int(triangle.indices.z)];
        bool h = triangleIntersection(ray, v0, v1, v2, triangle.materialIndex,
                                      record);
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
    int maxDepth = 15;
    vec3 radiance = vec3(0.0);
    vec3 throughput = vec3(1.0);

    for (int depth = 0; depth < maxDepth; depth++) {
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
            vec3 N = hitRecord.n;
            vec3 V = -normalize(ray.direction);
            vec3 albedo = hitMaterial.albedo;
            float metallic = hitMaterial.metal;
            float roughness = max(hitMaterial.roughness, 0.04);

            // Sample GGX for the direction
            vec3 L, H;
            float pdf;
            float NdotL, NdotV, HdotV;

            for (int i = 0; i < 5; i++) {
                vec2 xi = vec2(randomFloat(seed + uint(i * 17)),
                               randomFloat(pcgHash(seed + uint(i * 31))));
                sampleGGX(N, V, roughness, xi, L, pdf, H);

                NdotL = max(dot(N, L), 0.0);
                NdotV = max(dot(N, V), 0.0);
                HdotV = max(dot(H, V), 0.0);

                // Validate sample
                if (NdotL > 0.0 && NdotV > 0.0 && pdf > 1e-6) {
                    break;  // Found a valid sample
                }
            }

            // Evaluate complete PBR BRDF (diffuse + specular)
            vec3 F0 = mix(vec3(0.04), albedo, metallic);
            vec3 F = reflectance(HdotV, F0);

            // Specular term
            float NDF = distributionGGX(N, H, roughness);
            float G = geometrySmith(N, V, L, roughness);
            vec3 specular = (NDF * G * F) / max(4.0 * NdotV * NdotL, 1e-6);

            // Diffuse term (Lambertian)
            vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
            vec3 diffuse = kD * albedo / PI;

            // Combined BRDF
            vec3 brdf = diffuse + specular;

            // Monte Carlo estimator: BRDF * cos(theta) / pdf
            throughput *= (brdf * NdotL) / pdf;
            reflectedDir = L;

        }
        // Metallic
        else if (hitMaterial.type == 1.0) {
            // Perfect reflection + some roughness. If roughness is 0, it's a
            // perfect mirror.
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
            reflectedDir =
                normalize(direction +
                          hitMaterial.roughness * randomVec3(-1.0, 1.0, seed));
            throughput *= hitMaterial.albedo;
        }

        else if (hitMaterial.type == 3.0) {
            // Emissive material - stop the path and accumulate emission
            radiance += throughput * hitMaterial.albedo * hitMaterial.emission;
            break;
        }

        else {
            return vec3(0.0, 0.0, 0.0);  // Black for error
        }
        ray = Ray(hitPoint + reflectedDir * EPSILON, reflectedDir);
    }

    return radiance;
}

void main() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);

    // Don’t go out of bounds
    if (pixel.x >= params.width || pixel.y >= params.height) return;

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
        Ray ray = generateRay(jitteredPixel, sampleSeed);
        colorSum += pathTrace(ray, sampleSeed);
    }

    vec3 frameColor = colorSum / params.samples;

    // Gamma correction 2.2
    frameColor = pow(frameColor, vec3(1.0 / 2.2));  // Gamma correction

    // Write pixel color
    vec3 writeColor = frameColor;

    if (params.frameNumber > 1.0) {
        vec3 existingColor = imageLoad(accumulationImage, pixel).rgb;
        writeColor = existingColor * (1.0 - params.frameWeight) +
                     frameColor * params.frameWeight;
    }

    // Write to accumulation buffer and output image
    imageStore(accumulationImage, pixel, vec4(writeColor, 1.0));
    imageStore(outImage, pixel, vec4(writeColor, 1.0));
}
