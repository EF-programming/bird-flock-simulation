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


  // OpenCL variables

  cl_float* p_birds = &(state.birds[0].pos[0]);
  
  cl_uint* p_bird_to_flock = state.bird_to_flock;

  cl_float* p_flock_avgs = &(state.flocks[0].avgdir[0]);

  cl_uint* p_flock_ranges = state.flock_ranges;

  cl_int err;

  cl_uint num_platforms;
  cl_platform_id platform_ids[1];

  cl_uint num_devices;
  cl_device_id device_ids[2];

  cl_context m_context;

  cl_command_queue queue_gpu;

  cl_program program;

  cl_kernel simulate_bird_kernel;

  cl_mem birds_buffer, bird_to_flock_buffer, flock_avgs_buffer, flock_ranges_buffer, time_input_buffer;

  size_t birds_buffer_size = (sizeof(cl_float) * 6 * state.max_birds), bird_to_flock_buffer_size = (sizeof(cl_uint) * state.max_birds), flock_avgs_buffer_size = (sizeof(cl_float) * 2 * state.max_flocks), flock_ranges_buffer_size = (sizeof(cl_uint) * 2 * state.max_flocks), time_input_buffer_size = sizeof(cl_float);

  // OpenCL setup

  err = clGetPlatformIDs(0, nullptr, &num_platforms);

  std::cout << "\nNumber of Platforms are " << num_platforms << "!" << endl;

  err = clGetPlatformIDs(num_platforms, platform_ids, &num_platforms);

  err = clGetDeviceIDs(platform_ids[0], CL_DEVICE_TYPE_ALL, 0, nullptr, &num_devices);

  cout << "There are " << num_devices << " Device(s) the Platform!" << endl;

  err = clGetDeviceIDs(platform_ids[0], CL_DEVICE_TYPE_ALL, num_devices, device_ids, nullptr);

  cout << "\nChecking  Device " << 1 << "..." << endl;

  // Determine Device Types
  cl_device_type m_type;
  clGetDeviceInfo(device_ids[0], CL_DEVICE_TYPE, sizeof(m_type), &m_type, nullptr);
  if (m_type & CL_DEVICE_TYPE_CPU)
  {
    err = clGetDeviceIDs(platform_ids[0], CL_DEVICE_TYPE_CPU, 1, &device_ids[0], nullptr);
  }
  else if (m_type & CL_DEVICE_TYPE_GPU)
  {
    cout << "Device is a GPU" << endl;
    err = clGetDeviceIDs(platform_ids[0], CL_DEVICE_TYPE_GPU, 1, &device_ids[0], nullptr);
  }
  else if (m_type & CL_DEVICE_TYPE_ACCELERATOR)
  {
    err = clGetDeviceIDs(platform_ids[0], CL_DEVICE_TYPE_ACCELERATOR, 1, &device_ids[0], nullptr);
  }
  else if (m_type & CL_DEVICE_TYPE_DEFAULT)
  {
    err = clGetDeviceIDs(platform_ids[0], CL_DEVICE_TYPE_DEFAULT, 1, &device_ids[0], nullptr);
  }
  else
  {
    std::cerr << "\nDevice " << 1 << " is unknowned!" << endl;
  }

  // Create Context
  const cl_context_properties properties[] = { CL_CONTEXT_PLATFORM, (cl_context_properties)platform_ids[0], 0 };

  m_context = clCreateContext(properties, num_devices, device_ids, nullptr, nullptr, &err);
  //m_context = clCreateContextFromType(0, CL_DEVICE_TYPE_GPU, NULL, NULL, &err);

  // Setup Command Queues
  queue_gpu = clCreateCommandQueue(m_context, device_ids[0], 0, &err);


  const char* source[1] = { char_simulate_bird };
  cl_uint count = 1;

  // Create Program with all kernels
  program = clCreateProgramWithSource(m_context, count, source, NULL, &err);

  // Build Program
  err = clBuildProgram(program, num_devices, device_ids, nullptr, nullptr, nullptr);

  // Create Kernels
  simulate_bird_kernel = clCreateKernel(program, "simulate_bird", &err);

  float test = 5.0f;
  // Setup Buffers
  birds_buffer = clCreateBuffer(m_context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, birds_buffer_size, p_birds, &err);
  bird_to_flock_buffer = clCreateBuffer(m_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, bird_to_flock_buffer_size, p_bird_to_flock, &err);
  flock_avgs_buffer = clCreateBuffer(m_context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, flock_avgs_buffer_size, p_flock_avgs, &err);
  flock_ranges_buffer = clCreateBuffer(m_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, flock_ranges_buffer_size, p_flock_ranges, &err);
  time_input_buffer = clCreateBuffer(m_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, time_input_buffer_size, &test, &err);

  // Set arguments
  err = clSetKernelArg(simulate_bird_kernel, 0, sizeof(cl_mem), (void*)&birds_buffer);
  err = clSetKernelArg(simulate_bird_kernel, 1, sizeof(cl_mem), (void*)&bird_to_flock_buffer);
  err = clSetKernelArg(simulate_bird_kernel, 2, sizeof(cl_mem), (void*)&flock_avgs_buffer);
  err = clSetKernelArg(simulate_bird_kernel, 3, sizeof(cl_mem), (void*)&flock_ranges_buffer);
  err = clSetKernelArg(simulate_bird_kernel, 4, sizeof(cl_mem), (void*)&time_input_buffer);

  size_t work_dims[1]{ state.max_birds };

  // Run the kernel
  err = clEnqueueNDRangeKernel(queue_gpu, // command queue
    simulate_bird_kernel, // kernel
    1, // the number of dimensions used (1 to 3) (ex give 2 to work on a 2d matrix)
    NULL, // useless param, always NULL
    work_dims, // an array containing the size of each dimension for the entire kernel (for example m and n for a 2d matrix)
    NULL, // an array containing the size of each dimension for a single work group (a kernel is separated into work groups). NULL means let OpenCL automatically decide
    0, // event thing
    NULL, // event thing
    NULL // event thing
  );

  err = clEnqueueReadBuffer(queue_gpu, birds_buffer, CL_TRUE, 0, birds_buffer_size, p_birds, 0, NULL, NULL);


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

  //while (!glfwWindowShouldClose(window)) {
  //  //state.Simulate();
  //  //tbb::parallel_for(tbb::blocked_range<size_t>(0, state.num_of_flocks), SimulateFlocks(&state), tbb::auto_partitioner());

  //  mat4 view = mat4(1.0f);
  //  view = glm::translate(view, vec3(0.0f, 0.0f, -120.0f));
  //  mat4 projection;
  //  projection = glm::perspective(glm::radians(45.0f), 1024.0f / 768.0f, 0.1f, 170.0f);

  //  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  //  // Draw grid
  //  glBindTexture(GL_TEXTURE_2D, texture_grid);
  //  glBindVertexArray(grid_vao);
  //  glUseProgram(grid_shader.id);
  //  grid_shader.SetMatrix4fv("model", mat4(1.0f));
  //  grid_shader.SetMatrix4fv("view", view);
  //  grid_shader.SetMatrix4fv("projection", projection);
  //  glDrawArrays(GL_TRIANGLES, 0, 6);

  //  // Draw birds
  //  glBindVertexArray(triangle_vao);
  //  glUseProgram(bird_shader.id);
  //  bird_shader.SetMatrix4fv("view", view);
  //  bird_shader.SetMatrix4fv("projection", projection);

  //  for (int i = 0; i < state.num_of_flocks; ++i) {
  //    bird_shader.Set3fv("color", flock_colors[i]);
  //    for (int j = state.flock_ranges[i * 2]; j < state.flock_ranges[i * 2 + 1]; ++j)
  //    {
  //      DrawBird(state.birds[j], bird_shader);
  //    }
  //  }
  //  glfwSwapBuffers(window);

  //  glfwPollEvents();

  //  float time = (float)glfwGetTime();
  //  update_count += 1;
  //  if (time - time_of_last_fps_update >= 1.0f) {
  //    stringstream ss;
  //    ss << window_title << " " << "Draws/s: " << update_count;
  //    glfwSetWindowTitle(window, ss.str().c_str());
  //    update_count = 0;
  //    time_of_last_fps_update = time;
  //  }
  //}

  state.StopSim();
  //group.wait();

  clReleaseMemObject(birds_buffer);
  clReleaseMemObject(bird_to_flock_buffer);
  clReleaseMemObject(flock_avgs_buffer);
  clReleaseMemObject(flock_ranges_buffer);
  clReleaseMemObject(time_input_buffer);
 // clReleaseDevice(device_ids[0]);
  clReleaseContext(m_context);
  clReleaseKernel(simulate_bird_kernel);
  clReleaseProgram(program);
  clReleaseCommandQueue(queue_gpu);
}


