#version 330 core
layout (location = 0) in vec3 vp;
layout (location = 1) in vec2 uv_temp;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 uv;

void main() {
  uv = uv_temp;
  gl_Position = projection * view * model * vec4(vp, 1.0);
};