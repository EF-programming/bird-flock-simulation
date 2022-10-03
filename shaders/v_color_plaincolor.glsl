#version 330 core

layout(location = 0) in vec3 att_position;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * model * vec4(att_position.x, att_position.y, att_position.z, 1.0);
}
