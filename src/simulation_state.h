#pragma once
#include <glm.hpp>
#include <thread>

using glm::vec3;
using std::thread;

struct Bird {
  vec3 pos;
  vec3 dir;
  vec3 force;
};

struct Flock {
  int start_index;
  int end_index; // exclusive
  vec3 avgdir{ 0,0,0 };
  vec3 avgpos{ 0,0,0 };
};

struct SimulationState {
  static constexpr float world_size_x_start = -30.0f;
  static constexpr float world_size_x_end = 30.0f;
  static constexpr float world_size_y_start = -30.0f;
  static constexpr float world_size_y_end = 30.0f;
  static constexpr float world_size_z_start = 20.0f;
  static constexpr float world_size_z_end = 35.0f;
  const static int min_flocks = 3;
  const static int max_flocks = 7;
  const static int min_birds_in_flock = 10;
  const static int max_birds_in_flock = 20;
  const static int max_birds = max_flocks * max_birds_in_flock;
  float bird_mov_speed = 2.0f;
  float bird_rot_speed = 0.4f; // in rad per second
  float separation_dist = 4.0f;
  float separation_force_coefficient = 2.0f;
  float flock_alignment_coefficient = 0.5f;
  float flock_cohesion_coefficient = 0.5f;

  Flock flocks[max_flocks]{};
  Bird birds[max_birds]{};
  int num_of_flocks;

  thread threads[max_flocks]{};
  bool flock_update_requests[max_flocks]{};

  float delta_time = 0;
  float last_frame_time = 0;

  void CreateFlocks();
  int CreateBirds(int start_index);
  bool simulation_active = true;
  void StopSim();

  void SimulateBirdPair(int index_a, int index_b);
  void SimulateFlock(int index);
  void Simulate();
  void CalcFlockAvgs(int i);
};

