/**
Iterates through all the scene and finds the closest intersection.
*/
bool intersectScene(Ray ray, out HitRecord nearestHit) {
  nearestHit.t = -1.0;
  bool didHit = false;
  HitRecord record;

  Sphere sphere;
  for (int i = 0; i < int(sphereCount); i++) {
    sphere = spheres[i];
    bool h = sphereIntersection(ray, sphere, record);
    if (h && (record.t < nearestHit.t || nearestHit.t < 0.0)) {
      nearestHit = record;
      didHit = true;
    }
  }

  // Triangle triangle;
  // for (int i = 0; i < int(triangleCount); i++) {
  //   triangle = triangles[i];
  //   vec3 v0 = vertices[int(triangle.indices.x)];
  //   vec3 v1 = vertices[int(triangle.indices.y)];
  //   vec3 v2 = vertices[int(triangle.indices.z)];
  //   bool h =
  //       triangleIntersection(ray, v0, v1, v2, triangle.materialIndex, record);
  //   if (h && (record.t < nearestHit.t || nearestHit.t < 0.0)) {
  //     nearestHit = record;
  //     didHit = true;
  //   }
  // }

  int nodeStack[16];
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
        vec3 v0 = vertices[int(triangle.indices.x)];
        vec3 v1 = vertices[int(triangle.indices.y)];
        vec3 v2 = vertices[int(triangle.indices.z)];
        bool h = triangleIntersection(ray, v0, v1, v2, triangle.materialIndex, record);
        if (h && (record.t < nearestHit.t || nearestHit.t < 0.0)) {
          nearestHit = record;
          didHit = true;
        }

      }

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

  return didHit;
}

vec3 sampleSky(Ray ray) {
  vec2 uv = vec2(atan(ray.direction.z, ray.direction.x) / (2.0 * PI) + 0.5,
                 acos(clamp(ray.direction.y, -1.0, 1.0)) / PI);

  return texture(skybox, uv).rgb * settings.environmentEnergy;
}