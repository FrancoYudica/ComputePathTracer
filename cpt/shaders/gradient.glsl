#[compute]
#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(rgba8, set = 0, binding = 0) uniform image2D out_image;

// Push constants to get image size
layout(push_constant) uniform PushConstants {
  int width;
  int height;
}
params;

void main() {
  ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);

  // Don’t go out of bounds
  if (pixel.x >= params.width || pixel.y >= params.height)
    return;

  // Compute normalized coordinates (0.0–1.0)
  vec2 uv = vec2(pixel) / vec2(params.width, params.height);

  // Simple gradient: horizontal red → vertical blue
  vec4 color = vec4(uv.x, uv.y, 0.5, 1.0);

  // Write pixel color
  imageStore(out_image, pixel, color);
}
