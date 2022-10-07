#version 330 core

in vec2 uv;

uniform vec3 uniform_color;
uniform sampler2D main_texture;

out vec4 frag_color;

void main()
{
	vec4 texture_rgba = texture(main_texture, uv);

	frag_color = texture_rgba;

	
};
