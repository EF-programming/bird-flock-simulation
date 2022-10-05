#include "simulation_state.h"
#include <array>
#include <string>
#include <iostream>

void SimulationState::CreateFlocks() {
  int num_of_flocks = rand() % (max_flocks - min_flocks + 1) + min_flocks;
  //int num_of_flocks = 3;
  for (int i = 0; i < num_of_flocks; ++i) {
    flocks[i] = Flock();
    flocks[i].start_index = i * max_birds_in_flock;
    int num_of_birds = CreateBirds(flocks[i].start_index);
    flocks[i].end_index = flocks[i].start_index + num_of_birds;
  }
}

int SimulationState::CreateBirds(int start_index) {
  int num_of_birds = rand() % (max_birds_in_flock - min_birds_in_flock + 1) + min_birds_in_flock;
  //int num_of_birds = 3;
  for (int i = 0; i < num_of_birds; ++i) {
    birds[i] = Bird();
  }
  return num_of_birds;
}

void SimulationState::SimulateBirdPair(int a, int b) {
  vec3 delta = birds[b].pos - birds[a].pos;
  float distance = glm::length(delta);
  if (distance < separation_dist) {
    vec3 force = (separation_dist - distance) * glm::normalize(delta) * separation_force_coefficient;
    birds[a].force -= force;
    birds[b].force += force;
  }
}

void SimulationState::SimulateFlock(int i) {
  for (int j = flocks[i].start_index; j < flocks[i].end_index; ++j) {
    // Separation: each bird steers to avoid other birds in flock
    for (int k = j + 1; k < flocks[i].end_index; ++k) {
      SimulateBirdPair(j, k);
    }
    // Alignment: each bird steers towards the average direction of flock birds
    birds[j].force += flocks[i].avgdir * flock_alignment_coefficient;
    // Cohesion: each bird steers towards the average position of flock birds
    vec3 delta = flocks[i].avgpos - birds[j].pos;
    birds[j].force += glm::normalize(delta) * flock_cohesion_coefficient;
  }

  for (int j = flocks[i].start_index; j < flocks[i].end_index; ++j) {
    // rotate bird then move bird

    birds[j].force = vec3{ 0,0,0 };
  }

  // calc avg dir and pos
  vec3 dir{ 0,0,0 };
  vec3 pos{ 0,0,0 };
  for (int j = flocks[i].start_index; j < flocks[i].end_index; ++j) {
    dir += birds[j].dir;
    pos += birds[j].pos;
  }
  int num_of_birds = flocks[i].end_index - flocks[i].start_index;
  flocks[i].avgdir = dir / (float)num_of_birds;
  flocks[i].avgpos = pos / (float)num_of_birds;
}

void SimulationState::Simulate() {
  for (int i = 0; i < max_flocks; ++i) {
    SimulateFlock(i);
  }
}