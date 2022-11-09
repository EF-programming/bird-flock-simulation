#pragma once
#include <glm.hpp>
#include <thread>
#include <CL/opencl.h>

using glm::vec3;
using std::thread;

struct Bird {
  vec3 pos;
  vec3 dir;
};

struct Flock {
  vec3 avgdir{ 0,0,0 };
  vec3 avgpos{ 0,0,0 };
};

struct SimulationState {
  static constexpr float world_size_x_start = -50.0f;
  static constexpr float world_size_x_end = 50.0f;
  static constexpr float world_size_y_start = -50.0f;
  static constexpr float world_size_y_end = 50.0f;
  static constexpr float world_size_z_start = 20.0f;
  static constexpr float world_size_z_end = 75.0f;
  static const int min_flocks = 6;
  static const int max_flocks = 7;
  static const int min_birds_in_flock = 199;
  static const int max_birds_in_flock = 200;
  static const int max_birds = max_flocks * max_birds_in_flock;
  static constexpr float bird_mov_speed = 2.0f;
  static constexpr float bird_rot_speed = 0.4f; // in rad per second
  static constexpr float separation_dist = 4.0f;
  static constexpr float separation_force_coefficient = 2.0f;
  static constexpr float flock_alignment_coefficient = 0.5f;
  static constexpr float flock_cohesion_coefficient = 0.5f;

  Flock flocks[max_flocks]{};
  Bird birds[max_birds]{};
  cl_uint bird_to_flock[max_birds]{};
  cl_uint flock_ranges[max_flocks * 2];
  int num_of_flocks;

  thread threads[max_flocks]{};
  bool flock_update_requests[max_flocks]{};
  bool simulation_active = true;

  float delta_time = 0;
  float last_frame_time = 0;

  void CreateFlocks();
  int CreateBirds(int start_index, int flock);
  void StopSim();

  void SimulateBirdPair(int index_a, int index_b);
  void SimulateFlock(int index);
  void Simulate();
  void CalcFlockAvgs(int i);
};

