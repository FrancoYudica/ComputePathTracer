// PCG-like hash
uint pcgHash(uint seed) {
  uint state = seed * 747796405u + 2891336453u;
  uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
  return (word >> 22u) ^ word;
}

// Float in [0, 1) from hash
float hashToFloat(uint hash) { return float(hash) / 4294967296.0; }

// Improved random float using PCG hash
float randomFloat(uint seed) { return hashToFloat(pcgHash(seed)); }

float randomFloat(float min, float max, uint seed) {
  return min + (max - min) * randomFloat(seed);
}

// Better seed combination - use bit manipulation instead of arithmetic
uint combineSeed(uint a, uint b) { return pcgHash(a ^ (b << 16u)); }

uint combineSeed(uint a, uint b, uint c) {
  return pcgHash(a ^ (b << 16u) ^ (c << 8u));
}

// Random vec3 using improved hash
vec3 randomVec3(uint seed) {
  uint h1 = pcgHash(seed);
  uint h2 = pcgHash(h1);
  uint h3 = pcgHash(h2);
  return vec3(hashToFloat(h1), hashToFloat(h2), hashToFloat(h3));
}

vec3 randomVec3(float min, float max, uint seed) {
  return min + (max - min) * randomVec3(seed);
}

vec3 randomVec3N(uint seed) {
  return normalize(vec3(-1.0) + 2.0 * randomVec3(seed));
}



vec2 randomVec2(uint seed) {
  uint h1 = pcgHash(seed);
  uint h2 = pcgHash(h1);
  return vec2(hashToFloat(h1), hashToFloat(h2));
}

vec2 randomVec2(float min, float max, uint seed) {
  return min + (max - min) * randomVec2(seed);
}

vec3 randomUnitVec3(uint seed) {
  vec3 rand = randomVec3(seed);

  float theta = 2.0 * 3.14159265359 * rand.x;
  float phi = acos(2.0 * rand.y - 1.0);

  float sinPhi = sin(phi);
  return vec3(sinPhi * cos(theta), sinPhi * sin(theta), cos(phi));
}