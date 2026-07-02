// Vertex shader: textured quad with UBO-based transform.
//
// Inputs:
//   location 0: vec2 pos      — vertex position in clip space
//   location 1: vec3 color    — per-vertex tint (multiplied into fragment color)
//   location 2: vec2 uv       — texture coordinate
//
// Outputs (to fragment):
//   location 0: vec3 vColor
//   location 1: vec2 vUV
//
// Uniforms (set 0):
//   binding 0: UniformBuffer { mat4 mvp; }
//
// Compile:
//   glslangValidator -V triangle.vert.glsl -o vert.spv
// or:
//   glslc triangle.vert.glsl -o vert.spv
#version 450

layout(location = 0) in vec2 pos;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec3 vColor;
layout(location = 1) out vec2 vUV;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 mvp;
} ubo;

void main()
{
    vColor = color;
    vUV = uv;
    gl_Position = ubo.mvp * vec4(pos, 0.0, 1.0);
}
