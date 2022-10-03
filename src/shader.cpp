#define GLEW_STATIC 1   // This allows linking with Static Library on Windows, without DLL
#include <GL/glew.h>
#include <glm.hpp>
#include <iostream>
#include <fstream>
#include "shader.h";

using std::ifstream;
using std::istreambuf_iterator;
using glm::vec2;
using glm::vec3;
using glm::vec4;

Shader::Shader(string vertexshaderpath, string fragmentshaderpath, string geometryshaderpath) {
    // Create vertex, fragment, (geometry) shaders
    int v_id = LoadShader(vertexshaderpath, GL_VERTEX_SHADER);
    int g_id = 0;
    int f_id = LoadShader(fragmentshaderpath, GL_FRAGMENT_SHADER);
    if (geometryshaderpath != "") {
        g_id = LoadShader(geometryshaderpath, GL_GEOMETRY_SHADER);
    }
    // Link Shaders
    if (geometryshaderpath != "") { // repeated 'if' is intentional
        int shaders[] = { v_id, g_id, f_id };
        id = LinkShaders(shaders, 3);
    }
    else {
        int shaders[] = { v_id, f_id };
        id = LinkShaders(shaders, 2);
    }
}


// Loads shader from file, registers with gl, returns shader id.
// shader_type should be GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, etc
int Shader::LoadShader(string filename, GLenum shader_type) {
    // Load source file
    ifstream f(filename);
    string shadersource((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
    char* shadersourceptr = &shadersource[0];
    // Create shader and compile it
    int shader = glCreateShader(shader_type);
    glShaderSource(shader, 1, &shadersourceptr, NULL);
    glCompileShader(shader);
    int success;
    char infoLog[512];
    // Check for shader compile errors
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        string shader_type_str = "UNKNOWN";
        switch (shader_type) {
        case GL_VERTEX_SHADER: shader_type_str = "VERTEX"; break;
        case GL_GEOMETRY_SHADER: shader_type_str = "GEOMETRY"; break;
        case GL_FRAGMENT_SHADER: shader_type_str = "FRAGMENT"; break;
        }
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::" << shader_type_str << "::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    return shader;
}

// Given an array of shaders, compile and return shader program
int Shader::LinkShaders(int* shader_array, int size)
{
    // link shaders
    int shader_program = glCreateProgram();
    for (int i = 0; i < size; i++) { // attach each shader to program
        glAttachShader(shader_program, shader_array[i]);
    }
    glLinkProgram(shader_program);

    // check for linking errors
    int success;
    char infoLog[512];
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader_program, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    for (int i = 0; i < size; i++) { // delete shaders
        glDeleteShader(shader_array[i]);
    }

    return shader_program;
}

// Gets uniform location from shader. Has caching of locations.
int Shader::GetUniformPosition(string name) {
    int location = -1;
    if (locations.count(name) == 0) {
        location = glGetUniformLocation(id, name.c_str());
        if (location == -1) {
            // std::cerr << "Uniform name " + name + " does not exist." << std::endl;
        }
        locations[name] = location;
    }
    else {
        location = locations[name];
    }
    return location;
}

// Sets uniform value in shader
void Shader::SetMatrix4fv(string name, mat4 value) {
    int location = GetUniformPosition(name);
    glUniformMatrix4fv(location, 1, GL_FALSE, &value[0][0]);
}
void Shader::Set2fv(string name, vec2 value) {
    int location = GetUniformPosition(name);
    glUniform2fv(location, 1, &value[0]);
}
void Shader::Set3fv(string name, vec3 value) {
    int location = GetUniformPosition(name);
    glUniform3fv(location, 1, &value[0]);
}
void Shader::Set4fv(string name, vec4 value) {
    int location = GetUniformPosition(name);
    glUniform4fv(location, 1, &value[0]);
}
void Shader::Set1f(string name, float value) {
    int location = GetUniformPosition(name);
    glUniform1f(location, value);
}
void Shader::Set1i(string name, int value) {
    int location = GetUniformPosition(name);
    glUniform1i(location, value);
}