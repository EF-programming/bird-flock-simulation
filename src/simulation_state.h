#pragma once
#include <glm.hpp>

using glm::vec3;

struct Bird {
  vec3 pos;
  vec3 dir;
  vec3 force;
};

struct Flock {
  int start_index;
  int end_index; // exclusive
  vec3 avgdir{0,0,0};
  vec3 avgpos{0,0,0};
};

struct SimulationState {
  const static int min_flocks = 3;
  const static int max_flocks = 7;
  const static int min_birds_in_flock = 10;
  const static int max_birds_in_flock = 20;
  float bird_mov_speed = 1.0f;
  float bird_rot_speed = 55.0f;
  Flock flocks[max_flocks]{};
  Bird birds[max_flocks * max_birds_in_flock]{};

  void CreateFlocks();
  int CreateBirds(int start_index);

  void SimulateBirdPair(int index_a, int index_b);
  void SimulateFlock(int index);
  void Simulate();
};

