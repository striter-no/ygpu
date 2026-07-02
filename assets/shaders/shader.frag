// Fragment shader: textured quad with per-vertex tint.
//
// Inputs:
//   location 0: vec3 vColor   — interpolated tint from vertex shader
//   location 1: vec2 vUV      — interpolated texture coordinate
//
// Outputs:
//   location 0: vec4 fragColor
//
// Uniforms (set 0):
//   binding 1: combined texture sampler (texture + sampler)
//
// Compile:
//   glslangValidator -V triangle.frag.glsl -o frag.spv
// or:
//   glslc triangle.frag.glsl -o frag.spv
#version 450

layout(location = 0) in vec3 vColor;
layout(location = 1) in vec2 vUV;
layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 1) uniform sampler2D tex;

void main()
{
    vec4 sampled = texture(tex, vUV);
    fragColor = vec4(vColor, 1.0) * sampled;
}
