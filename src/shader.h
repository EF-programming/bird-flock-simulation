#pragma once
#pragma once
#define GLEW_STATIC 1   // This allows linking with Static Library on Windows, without DLL
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <string>
#include <unordered_map>
using std::string;
using std::unordered_map;
using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;

struct Shader {
	int id; // id of shader in opengl
	unordered_map<string, int> locations{};
	Shader(string, string, string = "");
	int LoadShader(string filename, GLenum shader_type);
	int LinkShaders(int* shader_array, int size);
	int GetUniformPosition(string name);
	void SetMatrix4fv(string, mat4);
	void Set2fv(string, vec2);
	void Set3fv(string, vec3);
	void Set4fv(string, vec4);
	void Set1f(string, float);
	void Set1i(string, int);
};

