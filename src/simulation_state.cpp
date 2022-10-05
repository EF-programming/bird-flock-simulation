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

void SimulationState::SimulateBirdPair(int i, int j) {
  std::cout << i << " " << j << std::endl;
}

void SimulationState::SimulateFlock(int i) {
  for (int j = flocks[i].start_index; j < flocks[i].end_index; ++j) {
    for (int k = j + 1; k < flocks[i].end_index; ++k) {
      SimulateBirdPair(j, k);
    }
  }
}

void SimulationState::Simulate() {
  for (int i = 0; i < max_flocks; ++i) {
    SimulateFlock(i);
  }
}