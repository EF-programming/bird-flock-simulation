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
