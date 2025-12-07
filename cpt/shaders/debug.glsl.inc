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
    int boxIntersections = 1;
    int triangleIntersections = 0;

    int nodeStack[MAX_BVH_DEPTH];
    float nodeDistanceStack[MAX_BVH_DEPTH];
    nodeStack[0] = 0;
    nodeDistanceStack[0] = 0.0;
    int stackIndex = 0;
    float closestT = MAX_FLOAT;

    while (stackIndex >= 0) {
        // Pop node and its distance from stack
        int pendingNodeIndex = nodeStack[stackIndex];
        float nodeDistance = nodeDistanceStack[stackIndex];
        stackIndex--;

        // Early rejection: skip nodes farther than closest hit
        if (nodeDistance >= closestT) {
            continue;
        }

        BVHNode node = bvhNodes[pendingNodeIndex];

        // Check if this is a leaf node
        if (node.primitiveCount > 0) {
            // Test all primitives in this leaf
            for (int i = 0; i < int(node.primitiveCount); i++) {
                int primitiveIndex = int(node.primitiveStartIndex) + i;
                Triangle triangle = triangles[primitiveIndex];
                Vertex v0 = vertices[int(triangle.indices.x)];
                Vertex v1 = vertices[int(triangle.indices.y)];
                Vertex v2 = vertices[int(triangle.indices.z)];

                HitRecord record = createHitRecord();
                bool hit = triangleIntersection(ray, v0, v1, v2,
                                                triangle.materialIndex, record);

                if (hit && record.t < closestT) {
                    // Update closest t when we find a closer hit
                    closestT = record.t;
                }
                triangleIntersections++;
            }
        } else {
            // Internal node - test and push children
            int right = int(node.rightChildIndex);
            int left = int(node.leftChildIndex);

            float tLeft, tRight;
            bool hitLeft =
                (left != 0) &&
                aabbIntersection(ray, bvhNodes[left].aabb, closestT, tLeft);
            bool hitRight =
                (right != 0) &&
                aabbIntersection(ray, bvhNodes[right].aabb, closestT, tRight);

            boxIntersections += 2;

            // Push farther child first, then closer (so closer is popped first)
            if (hitLeft && hitRight) {
                // Pushes left and then right (pops right first)
                if (tLeft > tRight) {
                    if (stackIndex < MAX_BVH_NODE_INDEX) {
                        stackIndex++;
                        nodeStack[stackIndex] = left;
                        nodeDistanceStack[stackIndex] = tLeft;
                    }
                    if (stackIndex < MAX_BVH_NODE_INDEX) {
                        stackIndex++;
                        nodeStack[stackIndex] = right;
                        nodeDistanceStack[stackIndex] = tRight;
                    }
                }
                // Pushes right and then left (pops left first)
                else {
                    if (stackIndex < MAX_BVH_NODE_INDEX) {
                        stackIndex++;
                        nodeStack[stackIndex] = right;
                        nodeDistanceStack[stackIndex] = tRight;
                    }
                    if (stackIndex < MAX_BVH_NODE_INDEX) {
                        stackIndex++;
                        nodeStack[stackIndex] = left;
                        nodeDistanceStack[stackIndex] = tLeft;
                    }
                }
            }
            // Pushes left child only
            else if (hitLeft && stackIndex < MAX_BVH_NODE_INDEX) {
                stackIndex++;
                nodeStack[stackIndex] = left;
                nodeDistanceStack[stackIndex] = tLeft;
            }
            // Pushes right child only
            else if (hitRight && stackIndex < MAX_BVH_NODE_INDEX) {
                stackIndex++;
                nodeStack[stackIndex] = right;
                nodeDistanceStack[stackIndex] = tRight;
            }
        }
    }

    float boxValue =
        float(boxIntersections) / settings.debugBVHBoxIntersectionsThreshold;
    float triangleValue = float(triangleIntersections) /
                          settings.debugBVHTriangleIntersectionsThreshold;

    if (max(boxValue, triangleValue) > 1.0) {
        return vec3(1.0);
    }

    return vec3(triangleValue, 0.0, boxValue);
    // return heatMapColor(float(boxIntersections), 100.0);
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
        return vec3(record.t / 20.0);
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

vec3 debugRayCast(Ray ray, uint seed) {
    HitRecord record;
    intersectScene(ray, record);

    if (record.didHit) {
        Material material = getMaterial(record.materialIndex);
        vec3 albedo = getMaterialAlbedo(material, record.uv);
        return albedo;
    }

    return sampleSky(ray);
}

vec3 debugMetallic(Ray ray, uint seed) {
    HitRecord record;
    intersectScene(ray, record);

    if (record.didHit) {
        Material material = getMaterial(record.materialIndex);
        return vec3(getMaterialMetallic(material, record.uv));
    }

    return vec3(0.0);
}

vec3 debugRoughness(Ray ray, uint seed) {
    HitRecord record;
    intersectScene(ray, record);

    if (record.didHit) {
        Material material = getMaterial(record.materialIndex);
        return vec3(getMaterialRoughness(material, record.uv));
    }

    return vec3(0.0);
}

vec3 debugEmission(Ray ray, uint seed) {
    HitRecord record;
    intersectScene(ray, record);

    if (record.didHit) {
        Material material = getMaterial(record.materialIndex);
        return getMaterialEmission(material, record.uv);
    }

    return vec3(0.0);
}