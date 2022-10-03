#version 330 core
layout(location = 0) in vec3 att_position;
void main() {
  gl_Position = vec4(att_position, 1.0);
};