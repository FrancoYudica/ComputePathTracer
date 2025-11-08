#define EPSILON 0.0001

struct HitRecord {
	float t;
	vec3 p;
	vec3 n;
	bool frontFace;
};

vec3 rayAt(Ray ray, float t) { return ray.origin + ray.direction * t; }

bool sphereIntersection(Ray ray, Sphere sphere, out HitRecord record) {
	vec3 oc = ray.origin - sphere.center;
	float a = dot(ray.direction, ray.direction);
	float half_b = dot(ray.direction, oc);
	float c = dot(oc, oc) - sphere.radius * sphere.radius;
	float discriminant = half_b * half_b - a * c;

	if (discriminant < 0.0) {
	    return false;
	} 

	float sqrtd = sqrt(discriminant);

	float root = (-half_b - sqrtd) / a;
	if (root < 0.0) {
		root = (-half_b + sqrtd) / a;

		if (root < 0.0) {
			return false;
		}
	}
	record.t = root;
	record.p = rayAt(ray, root);
	record.n = (record.p - sphere.center) / sphere.radius;
	// set frontFace so the normal always points against the incoming ray
	record.frontFace = dot(ray.direction, record.n) < 0.0;
	if (!record.frontFace) {
		record.n = -record.n;
	}

	return true;
}

vec3 randomVec3(float seed) {
    // Better hash with different constants for each component
    float rand1 = fract(sin(seed * 12.9898) * 43758.5453);
    float rand2 = fract(sin(seed * 78.233) * 43758.5453);
    float rand3 = fract(sin(seed * 157.543) * 43758.5453);
    return vec3(rand1, rand2, rand3);
}

vec3 randomVec3(float min, float max, float seed) {
    return min + (max - min) * randomVec3(seed);
}

// Use the spherical coordinate method - more reliable
vec3 randomUnitVec3(float seed) {
    vec3 rand = randomVec3(seed);
    
    float theta = 2.0 * 3.14159265359 * rand.x;
    float phi = acos(2.0 * rand.y - 1.0);
    
    float sinPhi = sin(phi);
    return vec3(
        sinPhi * cos(theta),
        sinPhi * sin(theta),
        cos(phi)
    );
}

vec3 randomInHemisphereVec3(vec3 normal, float seed) {
    vec3 randomDir = randomUnitVec3(seed);
    
    // Flip to correct hemisphere
    if (dot(randomDir, normal) < 0.0) {
        randomDir = -randomDir;
    }

    return randomDir;
}