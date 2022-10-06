#include <iostream>
#include <sstream>
#include <string>
#define GLEW_STATIC 1
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include "shader.h"
#include "simulation_state.h"

using glm::vec3;
using glm::mat4;
using std::stringstream;

void DrawBird(Bird bird, Shader shader) {
  mat4 model = mat4(1.0f);
  model = glm::translate(model, bird.pos);
  model = glm::rotate(model, atan2(bird.dir.y, bird.dir.x), vec3(0.0f, 0.0f, 1.0f));
  shader.SetMatrix4fv("model", model);

  glDrawArrays(GL_TRIANGLES, 0, 3);
}

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

  // Initialize random seed
  srand((unsigned long)time(0));

  SimulationState state = SimulationState();
  state.CreateFlocks();

  // Enable multi-sampling (anti-aliasing)
  glfwWindowHint(GLFW_SAMPLES, 16);
  glEnable(GL_MULTISAMPLE);

  GLFWwindow* window = glfwCreateWindow(1024, 768, "Bird Flock Simulation", NULL, NULL);
  if (window == NULL)
  {
    std::cerr << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(0);

  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

  glewExperimental = true; // Needed for core profile
  if (glewInit() != GLEW_OK) {
    std::cerr << "Failed to create GLEW" << std::endl;
    glfwTerminate();
    return -1;
  }

  // Assign state to opengl window var to be able to retrieve it in glfw callbacks (to be able to access state in callback code)
  glfwSetWindowUserPointer(window, &state);

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
     0.5f,  0.0f,  0.0f,
     -0.25f, -0.25f,  0.0f,
    -0.25f, 0.25f,  0.0f
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

  Shader bird_shader = Shader("shaders/bird_v.glsl", "shaders/bird_f.glsl");

  vec3 flock_colors[] = {
    vec3(0.901961f, 0.623529f, 0.0f),
    vec3(0.337255f, 0.705882f, 0.913725f),
    vec3(0.0f, 0.619608f, 0.45098f),
    vec3(0.941176f, 0.894118f, 0.258824f),
    vec3(0.0f, 0.447059f, 0.698039f),
    vec3(0.835294f, 0.368627f, 0.0f),
    vec3(0.8f, 0.47451f, 0.654902f),
  };

  float time_of_last_fps_update = 0;
  int update_count = 0;

  while (!glfwWindowShouldClose(window)) {
    float time = (float)glfwGetTime();
    state.delta_time = time - state.last_frame_time;
    state.last_frame_time = time;

    state.Simulate();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(bird_shader.id);

    glBindVertexArray(vao);

    mat4 view = mat4(1.0f);
    view = glm::translate(view, vec3(0.0f, 0.0f, -100.0f));
    mat4 projection;
    projection = glm::perspective(glm::radians(45.0f), 1024.0f / 768.0f, 0.1f, 115.0f);
    bird_shader.SetMatrix4fv("view", view);
    bird_shader.SetMatrix4fv("projection", projection);

    for (int i = 0; i < state.num_of_flocks; ++i) {
      bird_shader.Set3fv("color", flock_colors[i]);
      for (int j = state.flocks[i].start_index; j < state.flocks[i].end_index; ++j)
      {
        DrawBird(state.birds[j], bird_shader);
      }
    }
    glfwSwapBuffers(window);

    glfwPollEvents();

    update_count += 1;
    if (time - time_of_last_fps_update >= 1.0f) {
      stringstream ss;
      ss << "Bird Flock Simulation" << " " << "FPS: " << update_count;
      glfwSetWindowTitle(window, ss.str().c_str());
      update_count = 0;
      time_of_last_fps_update = time;
    }
  }

  state.StopSim();
}


