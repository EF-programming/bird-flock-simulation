#version 330 core

out vec4 frag_color;
uniform vec3 uniform_color;

void main()
{
   frag_color = vec4(uniform_color, 1.0);
};
