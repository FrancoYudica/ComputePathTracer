vec3 sampleSky(Ray ray) {
    vec2 uv = vec2(atan(ray.direction.z, ray.direction.x) / (2.0 * PI) + 0.5,
                   acos(clamp(ray.direction.y, -1.0, 1.0)) / PI);

    return texture(skybox, uv).rgb * settings.environmentEnergy;
}