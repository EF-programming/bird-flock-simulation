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
using std::endl;
using std::cout;

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

void DrawBird(Bird& bird, Shader& shader) {
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

  GLFWwindow* window = glfwCreateWindow(1600, 1000, "Bird Flock Simulation", NULL, NULL);
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
  // Ignore camera near and far plane for clipping
  glEnable(GL_DEPTH_CLAMP);

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
     200.0f,  200.0f, 0.0f, 35.0f, 35.0f,  // top right
     200.0f, -200.0f, 0.0f, 35.0f, 0.0f, // bottom right
    -200.0f,  200.0f, 0.0f, 0.0f, 35.0f, // top left 

     200.0f, -200.0f, 0.0f, 35.0f, 0.0f, // bottom right
    -200.0f, -200.0f, 0.0f, 0.0f, 0.0f, // bottom left
    -200.0f,  200.0f, 0.0f, 0.0f, 35.0f  // top left
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


  // OpenCL variables

  cl_float* p_birds = &(state.birds[0].pos[0]);

  cl_uint* p_bird_to_flock = state.bird_to_flock;

  cl_float* p_flock_avgs = &(state.flocks[0].avgdir[0]);

  cl_uint* p_flock_ranges = state.flock_ranges;

  cl_int err;

  cl_uint num_platforms;

  cl_context gpu_context;
  cl_context cpu_context;

  cl_command_queue queue_gpu;
  cl_command_queue queue_cpu;

  cl_program program_gpu;
  cl_program program_cpu;

  cl_kernel simulate_bird_kernel;
  cl_kernel calc_flock_avgs_kernel;

  cl_mem birds_buffer_gpu, bird_to_flock_buffer, flock_avgs_buffer_gpu, flock_ranges_buffer_gpu, time_input_buffer;
  cl_mem birds_buffer_cpu, flock_avgs_buffer_cpu, flock_ranges_buffer_cpu;

  size_t birds_buffer_size = (sizeof(cl_float) * 6 * state.max_birds), bird_to_flock_buffer_size = (sizeof(cl_uint) * state.max_birds), flock_avgs_buffer_size = (sizeof(cl_float) * 6 * state.max_flocks), flock_ranges_buffer_size = (sizeof(cl_uint) * 2 * state.max_flocks), time_input_buffer_size = sizeof(cl_float);

  // OpenCL setup

  err = clGetPlatformIDs(0, NULL, &num_platforms);
  cl_platform_id* platform_ids = new cl_platform_id[num_platforms];
  err = clGetPlatformIDs(num_platforms, platform_ids, NULL);
  const cl_context_properties properties[] = { CL_CONTEXT_PLATFORM, (cl_context_properties)platform_ids[0], 0 };
  const cl_context_properties properties2[] = { CL_CONTEXT_PLATFORM, (cl_context_properties)(platform_ids[1]), 0};


  gpu_context = clCreateContextFromType(properties, CL_DEVICE_TYPE_GPU, NULL, NULL, &err);
  cpu_context = clCreateContextFromType(properties2, CL_DEVICE_TYPE_CPU, NULL, NULL, &err);


  // Setup GPU kernel
  size_t databytes;
  err = clGetContextInfo(gpu_context, CL_CONTEXT_DEVICES, 0, NULL, &databytes);

  cl_device_id device_ids[6];

  clGetContextInfo(gpu_context, CL_CONTEXT_DEVICES, databytes, device_ids, NULL);

  queue_gpu = clCreateCommandQueue(gpu_context, device_ids[0], 0, &err);

  const char* source[1] = { char_simulate_bird }; // array of pointers where each pointer points to a string
  cl_uint count = 1; // size of the source array

  // Create Program with all kernels
  program_gpu = clCreateProgramWithSource(gpu_context, count, source, NULL, &err);

  // Build Program
  err = clBuildProgram(program_gpu, NULL, 0, NULL, NULL, NULL);

  char compiler_output[4000]{}; // size must be larger than 'length' var
  size_t length;
  clGetProgramBuildInfo(program_gpu, device_ids[0], CL_PROGRAM_BUILD_LOG, sizeof(compiler_output), compiler_output, &length);
  //printf("%s", compiler_output);

  // Create Kernels
  simulate_bird_kernel = clCreateKernel(program_gpu, "simulate_bird", &err);

  float delta_time = 0;
  // Setup Buffers
  birds_buffer_gpu = clCreateBuffer(gpu_context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, birds_buffer_size, p_birds, &err);
  bird_to_flock_buffer = clCreateBuffer(gpu_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, bird_to_flock_buffer_size, p_bird_to_flock, &err);
  flock_avgs_buffer_gpu = clCreateBuffer(gpu_context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, flock_avgs_buffer_size, p_flock_avgs, &err);
  flock_ranges_buffer_gpu = clCreateBuffer(gpu_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, flock_ranges_buffer_size, p_flock_ranges, &err);
  time_input_buffer = clCreateBuffer(gpu_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, time_input_buffer_size, &delta_time, &err);

  // Set arguments
  err = clSetKernelArg(simulate_bird_kernel, 0, sizeof(cl_mem), (void*)&birds_buffer_gpu);
  err = clSetKernelArg(simulate_bird_kernel, 1, sizeof(cl_mem), (void*)&bird_to_flock_buffer);
  err = clSetKernelArg(simulate_bird_kernel, 2, sizeof(cl_mem), (void*)&flock_avgs_buffer_gpu);
  err = clSetKernelArg(simulate_bird_kernel, 3, sizeof(cl_mem), (void*)&flock_ranges_buffer_gpu);
  err = clSetKernelArg(simulate_bird_kernel, 4, sizeof(cl_mem), (void*)&time_input_buffer);
  
  size_t gpu_work_dims[1]{ state.max_birds };




  // Setup CPU kernel
  err = clGetContextInfo(cpu_context, CL_CONTEXT_DEVICES, 0, NULL, &databytes);

  clGetContextInfo(cpu_context, CL_CONTEXT_DEVICES, databytes, device_ids, NULL);

  queue_cpu = clCreateCommandQueue(cpu_context, device_ids[0], 0, &err);

  source[0] = { char_calc_flock_avgs }; // array of pointers where each pointer points to a string
  count = 1; // size of the source array

  // Create Program with all kernels
  program_cpu = clCreateProgramWithSource(cpu_context, count, source, NULL, &err);

  // Build Program
  err = clBuildProgram(program_cpu, NULL, 0, NULL, NULL, NULL);

  clGetProgramBuildInfo(program_cpu, device_ids[0], CL_PROGRAM_BUILD_LOG, sizeof(compiler_output), compiler_output, &length);
  //printf("%s", compiler_output);

  // Create Kernels
  calc_flock_avgs_kernel = clCreateKernel(program_cpu, "calc_flock_avgs", &err);

  // Setup Buffers
  birds_buffer_cpu = clCreateBuffer(cpu_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, birds_buffer_size, p_birds, &err);
  flock_avgs_buffer_cpu = clCreateBuffer(cpu_context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, flock_avgs_buffer_size, p_flock_avgs, &err);
  flock_ranges_buffer_cpu = clCreateBuffer(cpu_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, flock_ranges_buffer_size, p_flock_ranges, &err);

  // Set arguments
  err = clSetKernelArg(calc_flock_avgs_kernel, 0, sizeof(cl_mem), (void*)&birds_buffer_cpu);
  err = clSetKernelArg(calc_flock_avgs_kernel, 1, sizeof(cl_mem), (void*)&flock_avgs_buffer_cpu);
  err = clSetKernelArg(calc_flock_avgs_kernel, 2, sizeof(cl_mem), (void*)&flock_ranges_buffer_cpu);

  size_t cpu_work_dims[1]{ state.num_of_flocks };

  bool sim_running = true;
  int final_fps = 0;
  int final_ticks = 0;
  int final_flock_avgs_ticks = 0;

  std::thread sim_thread([&]() {
    float time_of_last_tick_update = 0;
    int update_count = 0;
    float last_tick_update_time = 0;

    while (sim_running) {
      float sim_time = (float)glfwGetTime();
      delta_time = sim_time - last_tick_update_time;
      if (delta_time < (1.0f / 31)) { // cap the update rate to 30/s
        continue;
      }
      last_tick_update_time = sim_time;

      err = clEnqueueWriteBuffer(queue_gpu, time_input_buffer, CL_TRUE, 0, time_input_buffer_size, &delta_time, 0, NULL, NULL);
      err = clEnqueueWriteBuffer(queue_gpu, flock_avgs_buffer_gpu, CL_TRUE, 0, flock_avgs_buffer_size, p_flock_avgs, 0, NULL, NULL);

      // Run the kernel
      err = clEnqueueNDRangeKernel(queue_gpu, // command queue
        simulate_bird_kernel, // kernel
        1, // the number of dimensions used (1 to 3) (ex give 2 to work on a 2d matrix)
        NULL, // useless param, always NULL
        gpu_work_dims, // an array containing the size of each dimension for the entire kernel (for example m and n for a 2d matrix)
        NULL, // an array containing the size of each dimension for a single work group (a kernel is separated into work groups). NULL means let OpenCL automatically decide
        0, // event thing
        NULL, // event thing
        NULL // event thing
      );
      clFinish(queue_gpu);
      err = clEnqueueReadBuffer(queue_gpu, birds_buffer_gpu, CL_TRUE, 0, birds_buffer_size, p_birds, 0, NULL, NULL);

      update_count += 1;
      if (sim_time - time_of_last_tick_update >= 1.0f) {
        final_ticks = update_count;
        update_count = 0;
        time_of_last_tick_update = sim_time;
      }
    }
  });

  std::thread avgs_thread([&]() {
    float time_of_last_tick_update = 0;
    int update_count = 0;
    float last_tick_update_time = 0;
    float delta = 0;

    while (sim_running) {
      float ttime = (float)glfwGetTime();
      delta = ttime - last_tick_update_time;
      if (delta < (1.0f / 31)) { // cap the update rate to 30/s
        continue;
      }
      last_tick_update_time = ttime;

      err = clEnqueueWriteBuffer(queue_cpu, birds_buffer_cpu, CL_TRUE, 0, birds_buffer_size, p_birds, 0, NULL, NULL);

      // Run the kernel
      err = clEnqueueNDRangeKernel(queue_cpu, // command queue
        calc_flock_avgs_kernel, // kernel
        1, // the number of dimensions used (1 to 3) (ex give 2 to work on a 2d matrix)
        NULL, // useless param, always NULL
        cpu_work_dims, // an array containing the size of each dimension for the entire kernel (for example m and n for a 2d matrix)
        NULL, // an array containing the size of each dimension for a single work group (a kernel is separated into work groups). NULL means let OpenCL automatically decide
        0, // event thing
        NULL, // event thing
        NULL // event thing
      );
      clFinish(queue_cpu);

      err = clEnqueueReadBuffer(queue_cpu, flock_avgs_buffer_cpu, CL_TRUE, 0, flock_avgs_buffer_size, p_flock_avgs, 0, NULL, NULL);

      update_count += 1;
      if (ttime - time_of_last_tick_update >= 1.0f) {
        final_flock_avgs_ticks = update_count;
        update_count = 0;
        time_of_last_tick_update = ttime;
      }
    }
    });

  float time_of_last_title_update = 0;
  float time_of_last_draw = 0;
  int update_count = 0;
  float last_sim_update_time = 0;

  while (!glfwWindowShouldClose(window)) {
    float draw_time = (float)glfwGetTime();
    if (draw_time - time_of_last_draw < 1.0f / 31) {
      continue;
    }
    time_of_last_draw = draw_time;

    mat4 view = mat4(1.0f);
    view = glm::translate(view, vec3(0.0f, 0.0f, -200.0f));
    mat4 projection;
    projection = glm::perspective(glm::radians(45.0f), 1600.0f / 1000.0f, 0.1f, 1000.0f);

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
      int start = state.flock_ranges[i * 2];
      int end = state.flock_ranges[i * 2 + 1];
      for (int j = start; j < end; ++j)
      {
        DrawBird(state.birds[j], bird_shader);
      }
    }
    glfwSwapBuffers(window);

    glfwPollEvents();

    update_count += 1;
    if (draw_time - time_of_last_title_update >= 1.0f) {
      final_fps = update_count;
      stringstream ss;
      ss << "Bird Flock Simulation" << " Sim/s: " << final_ticks << " Avg/s: " << final_flock_avgs_ticks << " Draws/s: " << final_fps;
      glfwSetWindowTitle(window, ss.str().c_str());
      update_count = 0;
      time_of_last_title_update = draw_time;
    }
  }

  sim_running = false;
  sim_thread.join();
  avgs_thread.join();

  clReleaseMemObject(birds_buffer_gpu);
  clReleaseMemObject(birds_buffer_cpu);
  clReleaseMemObject(bird_to_flock_buffer);
  clReleaseMemObject(flock_avgs_buffer_gpu);
  clReleaseMemObject(flock_avgs_buffer_cpu);
  clReleaseMemObject(flock_ranges_buffer_gpu);
  clReleaseMemObject(flock_ranges_buffer_cpu);
  clReleaseMemObject(time_input_buffer);
  delete[] platform_ids;
  clReleaseContext(gpu_context);
  clReleaseKernel(simulate_bird_kernel);
  clReleaseProgram(program_gpu);
  clReleaseCommandQueue(queue_gpu);
}
