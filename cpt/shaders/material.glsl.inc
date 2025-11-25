MaterialSample createMaterialSample() {
    MaterialSample materialSample;
    materialSample.albedo = vec3(0.0);
    materialSample.isEmissive = false;
    materialSample.ray = createRay(vec3(0.0), vec3(0.0));
    return materialSample;
}

vec3 getMaterialAlbedo(Material mat, vec2 uv) {
    return mat.albedo *
           texture(textureArray, vec3(uv.x, uv.y, mat.albedoTextureIndex)).rgb;
}

void sampleLambertian(Material mat, Ray ray, HitRecord hit, uint seed,
                      inout MaterialSample result) {
    vec3 direction;

    vec3 albedo = getMaterialAlbedo(mat, hit.uv);

    if (randomFloat(seed) > mat.metal) {
        // Diffuse reflection with roughness
        direction = normalize(hit.n + randomVec3N(seed));
        result.albedo = albedo * 0.5;
    } else {
        // Specular reflection with roughness
        vec3 perfectReflection = reflect(ray.direction, hit.n);

        // Roughness perturbs the reflection direction
        // roughness = 0: perfect mirror
        // roughness = 1: very rough/blurry reflection
        direction = normalize(perfectReflection +
                              mat.roughness * randomVec3N(pcgHash(seed)));

        // Metals tint their reflections
        vec3 specularTint = mix(vec3(1.0), albedo, mat.metal);
        result.albedo = specularTint;
    }

    result.ray = createRay(hit.p + hit.n * EPSILON, direction);
}

void sampleMetallic(Material mat, Ray ray, HitRecord hit, uint seed,
                    inout MaterialSample result) {
    // Perfect reflection + some roughness. If roughness is 0, it's a
    // perfect mirror.
    vec3 direction = normalize(reflect(ray.direction, hit.n)) +
                     mat.roughness * randomVec3N(seed);

    result.albedo = getMaterialAlbedo(mat, hit.uv);

    result.ray = createRay(hit.p + hit.n * EPSILON, direction);
}

void sampleDielectric(Material mat, Ray ray, HitRecord hit, uint seed,
                      inout MaterialSample result) {
    // Refraction ratio
    float ri =
        hit.frontFace ? (1.0 / mat.refractionIndex) : mat.refractionIndex;

    vec3 unitDirection = normalize(ray.direction);

    float cosTheta = min(dot(-unitDirection, hit.n), 1.0);
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    bool cannotRefract = ri * sinTheta > 1.0;
    vec3 direction;

    // Schlick approximation for reflectance
    float reflectProb = reflectance(cosTheta, ri);
    vec3 rayOriginOffset = hit.n;

    if (cannotRefract || reflectProb > randomFloat(0.0, 1.0, seed)) {
        direction = reflect(unitDirection, hit.n);
    } else {
        direction = refract(unitDirection, hit.n, ri);
        // Flips the normal when refracted
        rayOriginOffset = -hit.n;
    }
    direction =
        normalize(direction + mat.roughness * randomVec3(-1.0, 1.0, seed));

    result.albedo = getMaterialAlbedo(mat, hit.uv);
    result.ray = createRay(hit.p + rayOriginOffset * EPSILON, direction);
}

void sampleEmissive(Material mat, Ray ray, HitRecord hit, uint seed,
                    inout MaterialSample result) {
    result.isEmissive = true;
    result.albedo = getMaterialAlbedo(mat, hit.uv) * mat.emission;
}

MaterialSample sampleMaterial(Material mat, Ray ray, HitRecord hit, uint seed) {
    MaterialSample result = createMaterialSample();

    switch (int(mat.type)) {
        case 0:
            sampleLambertian(mat, ray, hit, seed, result);
            break;
        case 1:
            sampleMetallic(mat, ray, hit, seed, result);
            break;
        case 2:
            sampleDielectric(mat, ray, hit, seed, result);
            break;
        case 3:
            sampleEmissive(mat, ray, hit, seed, result);
            break;
    }
    return result;
}
