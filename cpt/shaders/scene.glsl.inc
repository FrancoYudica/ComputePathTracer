/**
Iterates through all the scene and finds the closest intersection.
*/
void intersectScene(Ray ray, out HitRecord record) {
    record = createHitRecord();

    for (int i = 0; i < int(sphereCount); i++) {
        sphereIntersection(ray, spheres[i], record);
    }

    if (triangleCount == 0) {
        return;
    }

    // Triangle triangle;
    // for (int i = 0; i < int(triangleCount); i++) {
    //   triangle = triangles[i];
    //   vec3 v0 = vertices[int(triangle.indices.x)];
    //   vec3 v1 = vertices[int(triangle.indices.y)];
    //   vec3 v2 = vertices[int(triangle.indices.z)];
    //   bool h =
    //       triangleIntersection(ray, v0, v1, v2, triangle.materialIndex,
    //       record);
    //   if (h && (record.t < nearestHit.t || nearestHit.t < 0.0)) {
    //     nearestHit = record;
    //     didHit = true;
    //   }
    // }

    int nodeStack[32];
    nodeStack[0] = 0;
    int stackIndex = 0;

    while (stackIndex >= 0) {
        // Pop node from stack
        int pendingNodeIndex = nodeStack[stackIndex--];
        BVHNode node = bvhNodes[pendingNodeIndex];

        // Test intersection with AABB
        if (aabbIntersection(ray, node.aabb)) {
            // Iterate through all its primitives and test intersections
            for (int i = 0; i < int(node.primitiveCount); i++) {
                int primitiveIndex = i + int(node.primitiveStartIndex);

                Triangle triangle = triangles[primitiveIndex];
                Vertex v0 = vertices[int(triangle.indices.x)];
                Vertex v1 = vertices[int(triangle.indices.y)];
                Vertex v2 = vertices[int(triangle.indices.z)];
                triangleIntersection(ray, v0, v1, v2, triangle.materialIndex,
                                     record);
            }

            int right = int(node.rightChildIndex);
            int left = int(node.leftChildIndex);

            // 0 means no child. Also, avoids overflowing the stack
            if (right != 0 && stackIndex < 31) {
                nodeStack[++stackIndex] = right;
            }
            if (left != 0 && stackIndex < 31) {
                nodeStack[++stackIndex] = left;
            }
        }
    }
}