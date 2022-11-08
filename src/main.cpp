#include <iostream>
#include <sstream>
#include <string>
#define GLEW_STATIC 1
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" // For loading textures
#include "shader.h"
#include "simulation_state.h"
#include <CL/opencl.h>
#include "kernels.h"

using glm::vec3;
using glm::mat4;
using std::stringstream;

//class SimulateFlocks {
//  SimulationState* sim_state;
//
//public:
//  SimulateFlocks(SimulationState* sim) {
//    sim_state = sim;
//  }
//  void operator() (const tbb::blocked_range<size_t>& r) const {
//    for (size_t i = r.begin(); i != r.end(); ++i) {
//      sim_state->SimulateFlock(i);
//    }
//  }
//};

// Load a texture from a file. Texture loading code from https://learnopengl.com/Getting-started/Textures
GLuint LoadTextureAlpha(string filename) {
  int texture_width;
  int texture_height;
  int texture_num_channels;
  unsigned char* data = stbi_load(filename.c_str(), &texture_width, &texture_height, &texture_num_channels, STBI_rgb_alpha);
  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture_width, texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);
  stbi_image_free(data);
  return texture;
};

void DrawBird(Bird bird, Shader shader) {
  mat4 model = mat4(1.0f);
  model = glm::translate(model, bird.pos);
  model = glm::rotate(model, atan2(bird.dir.y, bird.dir.x), vec3(0.0f, 0.0f, 1.0f));
  shader.SetMatrix4fv("model", model);

  glDrawArrays(GL_TRIANGLES, 0, 3);
}

void DrawScene(SimulationState* state, Shader grid_shader, Shader bird_shader, int texture_grid, int grid_vao, int triangle_vao, GLFWwindow* window, vec3* flock_colors) {
  while (state->simulation_active) {

    mat4 view = mat4(1.0f);
    view = glm::translate(view, vec3(0.0f, 0.0f, -120.0f));
    mat4 projection;
    projection = glm::perspective(glm::radians(45.0f), 1024.0f / 768.0f, 0.1f, 170.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw grid
    glBindTexture(GL_TEXTURE_2D, texture_grid);
    glBindVertexArray(grid_vao);
    glUseProgram(grid_shader.id);
    grid_shader.SetMatrix4fv("model", mat4(1.0f));
    grid_shader.SetMatrix4fv("view", view);
    grid_shader.SetMatrix4fv("projection", projection);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Draw birds
    glBindVertexArray(triangle_vao);
    glUseProgram(bird_shader.id);
    bird_shader.SetMatrix4fv("view", view);
    bird_shader.SetMatrix4fv("projection", projection);

    for (int i = 0; i < state->num_of_flocks; ++i) {
      bird_shader.Set3fv("color", flock_colors[i]);
      for (int j = state->flocks[i].start_index; j < state->flocks[i].end_index; ++j)
      {
        DrawBird(state->birds[j], bird_shader);
      }
    }
    glfwSwapBuffers(window);
  }
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
  glEnable(GL_BLEND);
  glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO); // Use basic transparency blending proportional to alpha

  float triangle_points[] = {
     0.5f,  0.0f,  0.0f,
     -0.25f, -0.25f,  0.0f,
    -0.25f, 0.25f,  0.0f
  };

  GLuint triangle_vbo = 0;
  glGenBuffers(1, &triangle_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, triangle_vbo);
  glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(float), triangle_points, GL_STATIC_DRAW);

  GLuint triangle_vao = 0;
  glGenVertexArrays(1, &triangle_vao);
  glBindVertexArray(triangle_vao);
  glBindBuffer(GL_ARRAY_BUFFER, triangle_vbo);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
  glEnableVertexAttribArray(0);

  float grid_points[] = {
     70.0f,  70.0f, 0.0f, 10.0f, 10.0f,  // top right
     70.0f, -70.0f, 0.0f, 10.0f, 0.0f, // bottom right
    -70.0f,  70.0f, 0.0f, 0.0f, 10.0f, // top left 

     70.0f, -70.0f, 0.0f, 10.0f, 0.0f, // bottom right
    -70.0f, -70.0f, 0.0f, 0.0f, 0.0f, // bottom left
    -70.0f,  70.0f, 0.0f, 0.0f, 10.0f  // top left
  };

  GLuint grid_vbo = 0;
  glGenBuffers(1, &grid_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, grid_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(grid_points), grid_points, GL_STATIC_DRAW);

  GLuint grid_vao = 0;
  glGenVertexArrays(1, &grid_vao);
  glBindVertexArray(grid_vao);
  glBindBuffer(GL_ARRAY_BUFFER, grid_vbo);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3* sizeof(float)));
  glEnableVertexAttribArray(1);

  GLuint texture_grid = LoadTextureAlpha("textures/grid.png");

  Shader bird_shader = Shader("shaders/bird_v.glsl", "shaders/bird_f.glsl");
  Shader grid_shader = Shader("shaders/grid_v.glsl", "shaders/grid_f.glsl");

  vec3 flock_colors[] = {
    vec3(0.901961f, 0.623529f, 0.0f),
    vec3(0.337255f, 0.705882f, 0.913725f),
    vec3(0.0f, 0.619608f, 0.45098f),
    vec3(0.941176f, 0.894118f, 0.258824f),
    vec3(0.0f, 0.447059f, 0.698039f),
    vec3(0.835294f, 0.368627f, 0.0f),
    vec3(0.8f, 0.47451f, 0.654902f),
  };

  string window_title = "Bird Flock Simulation";

  cl_float* p_birds = &(state.birds[0].pos[0]);


  //tbb::task_group group;
  //group.run([&] { 
  //  float time_of_last_fps_update = 0;
  //  int update_count = 0;

  //  while (state.simulation_active) {
  //    float time = (float)glfwGetTime();
  //    state.delta_time = time - state.last_frame_time;
  //    if (state.delta_time < 0.001f) {
  //      continue;
  //    }
  //    state.last_frame_time = time;

  //    tbb::parallel_for(tbb::blocked_range<size_t>(0, state.num_of_flocks), SimulateFlocks(&state), tbb::auto_partitioner());

  //    update_count += 1;
  //    if (time - time_of_last_fps_update >= 1.0f) {
  //      stringstream ss;
  //      ss << "Bird Flock Simulation" << " " << "Sim/s: " << update_count;
  //      window_title = ss.str();
  //      glfwSetWindowTitle(window, ss.str().c_str());
  //      update_count = 0;
  //      time_of_last_fps_update = time;
  //    }
  //  }
  //});


  float time_of_last_fps_update = 0;
  int update_count = 0;

  while (!glfwWindowShouldClose(window)) {
    //state.Simulate();
    //tbb::parallel_for(tbb::blocked_range<size_t>(0, state.num_of_flocks), SimulateFlocks(&state), tbb::auto_partitioner());

    mat4 view = mat4(1.0f);
    view = glm::translate(view, vec3(0.0f, 0.0f, -120.0f));
    mat4 projection;
    projection = glm::perspective(glm::radians(45.0f), 1024.0f / 768.0f, 0.1f, 170.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw grid
    glBindTexture(GL_TEXTURE_2D, texture_grid);
    glBindVertexArray(grid_vao);
    glUseProgram(grid_shader.id);
    grid_shader.SetMatrix4fv("model", mat4(1.0f));
    grid_shader.SetMatrix4fv("view", view);
    grid_shader.SetMatrix4fv("projection", projection);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Draw birds
    glBindVertexArray(triangle_vao);
    glUseProgram(bird_shader.id);
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

    float time = (float)glfwGetTime();
    update_count += 1;
    if (time - time_of_last_fps_update >= 1.0f) {
      stringstream ss;
      ss << window_title << " " << "Draws/s: " << update_count;
      glfwSetWindowTitle(window, ss.str().c_str());
      update_count = 0;
      time_of_last_fps_update = time;
    }
  }

  state.StopSim();
  //group.wait();
}


