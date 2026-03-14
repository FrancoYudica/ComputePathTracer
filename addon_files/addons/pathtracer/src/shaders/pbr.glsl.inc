// Trowbridge-Reitz GGX
float distributionGGX(vec3 N, vec3 H, float a)
{
    
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;
    float denom = NdotH2 * (a2 - 1.0f) + 1.0f;
    denom = PI * denom * denom;
    return a2 / denom;
}

float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0f;
    float k = r * r / 8.0f;
    
    float denom = NdotV * (1.0f - k) + k;
    return NdotV / denom;
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float ggx1 = geometrySchlickGGX(NdotV, roughness);
    float ggx2 = geometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

/**
sampleGGX - Samples a direction based on the GGX distribution
@param N - Surface normal
@param V - View direction
@param roughness - Surface roughness
@param xi - 2D random sample in [0,1]
@param L - Output sampled light direction
@param pdf - Output probability density function value
@param H - Output half-vector
*/

/*
void sampleGGX(vec3 N, vec3 V, float roughness, vec2 xi,
               out vec3 L, out float pdf, out vec3 H)
{
    float a = roughness * roughness;
    float a2 = a * a;
    
    // Sample half-vector
    float phi = 2.0 * PI * xi.x;
    float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (a2 - 1.0) * xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    
    vec3 H_tangent = vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
    
    // Build tangent frame
    vec3 up = abs(N.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
    
    H = normalize(tangent * H_tangent.x + bitangent * H_tangent.y + N * H_tangent.z);
    L = reflect(-V, H);
    
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);
    float D = distributionGGX(N, H, roughness);
    
    pdf = (D * NdotH) / (4.0 * HdotV + 1e-6);
}*/

// Helpers: stable tangent frame
void buildOrthonormalBasis(vec3 N, out vec3 T, out vec3 B) {
    // Stable basis: pick helper axis based on N.z
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    T = normalize(cross(up, N));
    B = cross(N, T);
}

// VNDF sampling in N-local space (Heitz)
vec3 sampleGGXVNDF_local(vec3 Vlocal, float a, vec2 xi)
{
    vec3 Vh = normalize(vec3(a * Vlocal.x, a * Vlocal.y, Vlocal.z));
    float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
    vec3 T1 = (lensq > 0.0) ? vec3(-Vh.y, Vh.x, 0.0) / sqrt(lensq)
                            : vec3(1.0, 0.0, 0.0);
    vec3 T2 = cross(Vh, T1);

    // Sample a point on the disk
    float r = sqrt(xi.x);
    float phi = 2.0 * PI * xi.y;
    float t1 = r * cos(phi);
    float t2 = r * sin(phi);

    // Reproject to hemisphere
    float s = 0.5 * (1.0 + Vh.z);
    t2 = mix(sqrt(max(0.0, 1.0 - t1 * t1)), t2, s);

    vec3 Nh = t1 * T1 + t2 * T2 + sqrt(max(0.0, 1.0 - t1*t1 - t2*t2)) * Vh;

    // Unstretch
    vec3 Hlocal = normalize(vec3(a * Nh.x, a * Nh.y, max(0.0, Nh.z)));
    return Hlocal;
}

// High-level VNDF sampler: transforms between world and local frames
vec3 sampleGGXVNDF(vec3 N, vec3 V_world, float roughness, vec2 xi)
{
    // safety
    roughness = max(roughness, 1e-4);
    float a = max(roughness * roughness, 0.001);  

    // Build world->local basis
    vec3 T, B;
    buildOrthonormalBasis(N, T, B);

    // Transform V into local space where N = (0,0,1)
    vec3 Vlocal = vec3(dot(V_world, T), dot(V_world, B), dot(V_world, N));
    Vlocal = normalize(Vlocal);

    // Sample H in local space (VNDF)
    vec3 Hlocal = sampleGGXVNDF_local(Vlocal, a, xi);

    // Transform H back to world space
    vec3 H_world = normalize(Hlocal.x * T + Hlocal.y * B + Hlocal.z * N);
    return H_world;
}

void sampleGGX(vec3 N, vec3 V_world, float roughness, vec2 xi,
               out vec3 L_world, out float pdf, out vec3 H_world)
{
    roughness = max(roughness, 0.02);
    float a = roughness * roughness;

    // VNDF sampling
    H_world = sampleGGXVNDF(N, V_world, roughness, xi);

    // Ensure hemisphere safety
    if (dot(H_world, N) < 0.0) H_world = -H_world;

    // Reflect
    L_world = normalize(reflect(-V_world, H_world));

    float NdotH = max(dot(N, H_world), 0.0);
    float NdotL = max(dot(N, L_world), 0.0);
    float HdotV = max(dot(H_world, V_world), 0.0);

    if (NdotL <= 0.0 || HdotV <= 1e-6) {
        pdf = 0.0;
        return;
    }

    float D = distributionGGX(N, H_world, roughness);
    pdf = max((D * NdotH) / (4.0 * HdotV), 0.0);
}
