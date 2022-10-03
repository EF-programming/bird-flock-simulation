#include <iostream>
#include <string>
#define GLEW_STATIC 1   // This allows linking with Static Library on Windows, without DLL
#include <GL/glew.h>    // Include GLEW - OpenGL Extension Wrangler
#include <GLFW/glfw3.h> // GLFW provides a cross-platform interface for creating a graphical context, initializing OpenGL and binding inputs
#include <glm.hpp>  // GLM is an optimized math library with syntax to similar to OpenGL Shading Language
#include "shader.h"
#include "simulation_state.h"

int main(int argc, char* argv[])
{
  glfwInit();

#if defined(PLATFORM_OSX)	
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#endif

  SimulationState state = SimulationState();

  GLFWwindow* window = glfwCreateWindow(1024, 768, "Comp371 - Project", NULL, NULL);
  if (window == NULL)
  {
    std::cerr << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

  // Initialize GLEW
  glewExperimental = true; // Needed for core profile
  if (glewInit() != GLEW_OK) {
    std::cerr << "Failed to create GLEW" << std::endl;
    glfwTerminate();
    return -1;
  }

  // Assign state to opengl window var to be able to retrieve it in glfw callbacks (to be able to access state in callback code)
  glfwSetWindowUserPointer(window, &state);

  // Initialize random seed
  srand((unsigned long)time(0));

  // Handle window resize events
  glfwSetFramebufferSizeCallback(window, [](GLFWwindow* w, int width, int height) {
    //WindowState* state = static_cast<WindowState*>(glfwGetWindowUserPointer(w));
    //state->window_width = width;
    //state->window_height = height;
    glViewport(0, 0, width, height);
    });

  // Key input
  glfwSetKeyCallback(window, [](GLFWwindow* w, int key, int scancode, int action, int mods) {
    SimulationState* state = static_cast<SimulationState*>(glfwGetWindowUserPointer(w));
    switch (key) {
      case GLFW_KEY_ESCAPE: {
        if (action == GLFW_PRESS) {
          glfwSetWindowShouldClose(w, true);
        }
      }
      break;
    }
    });

  // Set background color
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

  // Respect triangle overlap when drawing (don't draw triangles out of order, draw in reverse depth order)
  glEnable(GL_DEPTH_TEST);
  // Don't draw non-visible faces
  //glEnable(GL_CULL_FACE);

  float points[] = {
     0.0f,  0.5f,  0.0f,
     0.5f, -0.5f,  0.0f,
    -0.5f, -0.5f,  0.0f
  };

  GLuint vbo = 0;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(float), points, GL_STATIC_DRAW);

  GLuint vao = 0;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

  Shader triangle_shader = Shader("shaders/triangle_v.glsl", "shaders/triangle_f.glsl");

  // Entering Main Loop
  while (!glfwWindowShouldClose(window)) {

    // wipe the drawing surface clear
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(triangle_shader.id);
    glBindVertexArray(vao);
    // draw points 0-3 from the currently bound VAO with current in-use shader
    glDrawArrays(GL_TRIANGLES, 0, 3);
    // update other events like input handling 
    glfwPollEvents();
    // put the stuff we've been drawing onto the display
    glfwSwapBuffers(window);
  }
}
