#include "simulation_state.h"
#include <string>
#include <iostream>
#include <thread>

using std::thread;

void SimulationState::CreateFlocks() {
  num_of_flocks = rand() % (max_flocks - min_flocks + 1) + min_flocks;
  for (int i = 0; i < num_of_flocks; ++i) {
    flocks[i] = Flock();
    flock_ranges[i * 2] = i * max_birds_in_flock;
    int num_of_birds = CreateBirds(flock_ranges[i * 2], i);
    flock_ranges[i * 2 + 1] = flock_ranges[i * 2] + num_of_birds;
    CalcFlockAvgs(i);
  }
}

int SimulationState::CreateBirds(int start_index, int flock) {
  int num_of_birds = rand() % (max_birds_in_flock - min_birds_in_flock + 1) + min_birds_in_flock;
  //int num_of_birds = 3;
  for (int i = start_index; i < start_index + num_of_birds; ++i) {
    Bird b{};
    float pos_x = (rand() / (float)RAND_MAX) * (world_size_x_end - world_size_x_start) + world_size_x_start;
    float pos_y = (rand() / (float)RAND_MAX) * (world_size_y_end - world_size_y_start) + world_size_y_start;
    float pos_z = (rand() / (float)RAND_MAX) * (world_size_z_end - world_size_z_start) + world_size_z_start;
    b.pos = vec3{ pos_x, pos_y, pos_z };
    b.dir = glm::normalize(b.pos); // Just so birds don't have the same initial direction.
    birds[i] = b;
    bird_to_flock[i] = flock;
  }
  return num_of_birds;
}

void SimulationState::CalcFlockAvgs(int i) {
  vec3 dir{ 0,0,0 };
  vec3 pos{ 0,0,0 };
  for (int j = flock_ranges[i * 2]; j < flock_ranges[i * 2 + 1]; ++j) {
    dir += birds[j].dir;
    pos += birds[j].pos;
  }
  int num_of_birds = flock_ranges[i * 2 + 1] - flock_ranges[i * 2];
  dir = glm::normalize(dir / (float)num_of_birds);
  pos = pos / (float)num_of_birds;

  flocks[i].avgdir = dir;
  flocks[i].avgpos = pos;
}

void SimulationState::SimulateBirdPair(int a, int b) {
  vec3 delta = birds[b].pos - birds[a].pos;
  float distance = glm::length(delta);
  if (distance < separation_dist) {
    vec3 force = (separation_dist - distance) * glm::normalize(delta) * separation_force_coefficient;
    //birds[a].force -= force;
    //birds[b].force += force;
  }
}

void SimulationState::SimulateFlock(int i) {
  //for (int j = flock_ranges[i * 2]; j < flock_ranges[i * 2 + 1]; ++j) {
  //  // Separation: each bird steers to avoid other birds in flock
  //  for (int k = j + 1; k < flock_ranges[i * 2 + 1]; ++k) {
  //    SimulateBirdPair(j, k);
  //  }
  //  // Alignment: each bird steers towards the average direction of flock birds
  //  birds[j].force += flocks[i].avgdir * flock_alignment_coefficient;
  //  // Cohesion: each bird steers towards the average position of flock birds
  //  vec3 delta = flocks[i].avgpos - birds[j].pos;
  //  birds[j].force += glm::normalize(delta) * flock_cohesion_coefficient;
  //}

  //for (int j = flocks[i].start_index; j < flocks[i].end_index; ++j) {
  //  if (birds[j].pos.x < world_size_x_start) {
  //    birds[j].force += vec3(1, 0, 0);
  //  }
  //  else if (birds[j].pos.x > world_size_x_end) {
  //    birds[j].force += vec3(-1, 0, 0);
  //  }
  //  if (birds[j].pos.y < world_size_y_start) {
  //    birds[j].force += vec3(0, 1, 0);
  //  }
  //  else if (birds[j].pos.y > world_size_y_end) {
  //    birds[j].force += vec3(0, -1, 0);
  //  }
  //  if (birds[j].pos.z < world_size_z_start) {
  //    birds[j].force += vec3(0, 0, 1);
  //  }
  //  else if (birds[j].pos.z > world_size_z_end) {
  //    birds[j].force += vec3(0, 0, -1);
  //  }
  //  // rotate bird then move bird
  //  vec3 a = birds[j].dir;
  //  vec3 b = glm::normalize(birds[j].force);
  //  vec3 ninety = glm::normalize((glm::cross(glm::cross(a, b), a)));
  //  birds[j].dir = (float)cos(bird_rot_speed * delta_time) * a + (float)sin(bird_rot_speed * delta_time) * ninety;
  //  birds[j].pos += birds[j].dir * bird_mov_speed * delta_time;
  //  birds[j].force = vec3{ 0,0,0 };
  //}

  //CalcFlockAvgs(i);
}

void SimulationState::Simulate() {
  
  for (int i = 0; i < num_of_flocks; ++i) {
    flock_update_requests[i] = true;
  }
  bool all_updated = false;
  while (!all_updated) {
    all_updated = true;
    for (int i = 0; i < num_of_flocks; ++i) {
      if (flock_update_requests[i]) {
        all_updated = false;
      }
    }
  }
}

void SimulationState::StopSim() {
  simulation_active = false;
  //for (int i = 0; i < num_of_flocks; ++i) {
  //  threads[i].join();
  //}
}