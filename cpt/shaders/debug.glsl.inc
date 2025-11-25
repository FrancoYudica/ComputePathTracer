// Heat map color scheme: blue -> green -> yellow -> red
vec3 heatMapColor(float value, float maxValue) {
    float t = clamp(value / maxValue, 0.0, 1.0);

    if (t < 0.25) {
        return mix(vec3(0.0, 0.0, 1.0), vec3(0.0, 1.0, 1.0), t * 4.0);
    } else if (t < 0.5) {
        return mix(vec3(0.0, 1.0, 1.0), vec3(0.0, 1.0, 0.0), (t - 0.25) * 4.0);
    } else if (t < 0.75) {
        return mix(vec3(0.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0), (t - 0.5) * 4.0);
    } else {
        return mix(vec3(1.0, 1.0, 0.0), vec3(1.0, 0.0, 0.0), (t - 0.75) * 4.0);
    }
}

vec3 debugTraceBVH(Ray ray, uint seed) {
    int nodeStack[16];
    nodeStack[0] = 0;
    int stackIndex = 0;

    int intersections = 0;

    while (stackIndex >= 0) {
        // Pop node from stack
        int pendingNodeIndex = nodeStack[stackIndex--];
        BVHNode node = bvhNodes[pendingNodeIndex];

        if (aabbIntersection(ray, node.aabb)) {
            intersections++;

            int right = int(node.rightChildIndex);
            int left = int(node.leftChildIndex);

            // 0 means no child. Also, avoids overflowing the stack
            if (right != 0 && stackIndex < 15) {
                nodeStack[++stackIndex] = right;
            }
            if (left != 0 && stackIndex < 15) {
                nodeStack[++stackIndex] = left;
            }
        }
    }

    return heatMapColor(float(intersections), 50.0);
}

vec3 debugNormal(Ray ray, uint seed) {
    HitRecord record;
    intersectScene(ray, record);
    if (record.didHit) {
        return record.n * 0.5 + 0.5;
    }

    return ray.direction * 0.5 + 0.5;
}

vec3 debugDepth(Ray ray, uint seed) {
    HitRecord record;
    ray.direction = normalize(ray.direction);
    intersectScene(ray, record);
    if (record.didHit) {
        return vec3(record.t / 20.0);  // Assuming max depth of 100 units
    }
    return vec3(1.0);
}

vec3 debugUV(Ray ray, uint seed) {
    HitRecord record;
    intersectScene(ray, record);
    if (record.didHit) {
        return vec3(record.uv, 0.0);
    }
    return vec3(0.0);
}