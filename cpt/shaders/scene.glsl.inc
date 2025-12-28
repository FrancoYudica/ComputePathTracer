

/**
Iterates through all the scene and finds the closest intersection.
*/
void intersectScene(Ray ray, out HitRecord nearestRecord) {
    nearestRecord = createHitRecord();

    for (int i = 0; i < int(sphereCount); i++) {
        sphereIntersection(ray, spheres[i], nearestRecord);
    }

    if (triangleCount == 0) {
        return;
    }

    int nodeStack[MAX_BVH_DEPTH];
    float nodeDistanceStack[MAX_BVH_DEPTH];
    nodeStack[0] = 0;
    nodeDistanceStack[0] = 0.0;
    int stackIndex = 0;

    while (stackIndex >= 0) {
        // Pop node and its distance from stack
        int pendingNodeIndex = nodeStack[stackIndex];
        float nodeDistance = nodeDistanceStack[stackIndex];
        stackIndex--;

        // Early rejection: skip nodes farther than closest hit
        if (nodeDistance >= nearestRecord.t) {
            continue;
        }

        BVHNode node = bvhNodes[pendingNodeIndex];

        // Check if this is a leaf node
        if (node.primitiveCount > 0) {
            HitRecord testRecord = createHitRecord();
            // Test all primitives in this leaf
            for (int i = 0; i < int(node.primitiveCount); i++) {
                int primitiveIndex = int(node.primitiveStartIndex) + i;
                Triangle triangle = triangles[primitiveIndex];
                Vertex v0 = vertices[int(triangle.indices.x)];
                Vertex v1 = vertices[int(triangle.indices.y)];
                Vertex v2 = vertices[int(triangle.indices.z)];
                bool hit = triangleIntersection(
                    ray, v0, v1, v2, triangle.materialIndex, testRecord);

                // Reached a closer hit
                if (hit && testRecord.t < nearestRecord.t) {
                    // Test for alpha masking
                    float alpha = getMaterialAlpha(
                        getMaterial(triangle.materialIndex), testRecord.uv);

                    if (alpha > 0.5) {
                        // Update not discarded by alpha mask
                        nearestRecord = testRecord;
                    }
                }
            }
        } else {
            // Internal node - test and push children
            int right = int(node.rightChildIndex);
            int left = int(node.leftChildIndex);

            float tLeft, tRight;
            bool hitLeft =
                (left != 0) && aabbIntersection(ray, bvhNodes[left].aabb,
                                                nearestRecord.t, tLeft);
            bool hitRight =
                (right != 0) && aabbIntersection(ray, bvhNodes[right].aabb,
                                                 nearestRecord.t, tRight);

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
}