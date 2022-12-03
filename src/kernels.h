#pragma once
#include <string>

using std::string;

const string simulate_bird_kernel =
"__kernel void simulate_bird(__global float* p_birds, __global unsigned int* p_bird_to_flock, __global float* p_flock_avgs, __global unsigned int* p_flock_ranges, __global float* delta_time)\n"
"{\n"
"	unsigned int gid = get_global_id(0);\n"
" unsigned int flock_index = p_bird_to_flock[gid];\n"
" "
" float3 pos_a = vload3(gid * 2, p_birds);\n"
" unsigned int flock_start = p_flock_ranges[flock_index * 2];\n"
" unsigned int flock_end = p_flock_ranges[flock_index * 2 + 1];\n"
" unsigned int index = flock_start;\n"
" float3 force = (float3)(0,0,0);\n"

// Simulate bird pairs (separation force)
" while (index < flock_end) {\n"
"  if (gid == index) {\n"
"   index += 1;\n"
"   continue;\n"
"  }\n"
"  float3 pos_b = vload3(index * 2, p_birds);\n"
"  float3 delta = pos_b - pos_a;\n"
"  float distance = length(delta);\n"
"  if (distance < 4.0f) {\n"
"   force -= (4.0f - distance) * normalize(delta) * 2.0f;\n"
"  }\n"
"  index += 1;\n"
" }\n"

// Flock alignment and cohesion forces
" float3 flock_dir = vload3(flock_index * 2, p_flock_avgs);\n"
" float3 flock_pos = vload3(flock_index * 2, p_flock_avgs + 3);\n"
" // Alignment: each bird steers towards the average direction of flock birds\n"
" force = force + flock_dir * 0.5f;\n"
" // Cohesion: each bird steers towards the average position of flock birds\n"
" force = force + normalize(flock_pos - pos_a) * 0.5f;\n"

// If out of world bonds, make birds turn around
" if (pos_a.x < -80.0f) {\n"
"  force += (float3)(1, 0, 0);\n"
" }\n"
" else if (pos_a.x > 80.0f) {\n"
"  force += (float3)(-1, 0, 0);\n"
" }\n"
" if (pos_a.y < -50.0f) {\n"
"  force += (float3)(0, 1, 0);\n"
" }\n"
" else if (pos_a.y > 50.0f) {\n"
"  force += (float3)(0, -1, 0);\n"
" }\n"
" if (pos_a.z < 20.0f) {\n"
"  force += (float3)(0, 0, 2);\n"
" }\n"
" else if (pos_a.z > 75.0f) {\n"
"  force += (float3)(0, 0, -1);\n"
" }"

// Rotate and move bird
" float3 dir_a = vload3(gid * 2, p_birds + 3);\n"
" force = normalize(force);\n"
" float3 ninety = normalize((cross(cross(dir_a, force), dir_a)));\n"
" dir_a = (float)cos(0.4f * delta_time[0]) * dir_a + (float)sin(0.4f * delta_time[0]) * ninety;\n"
" pos_a += dir_a * 2.0f * delta_time[0];\n"

" p_birds[gid * 6] = pos_a.x;\n"
" p_birds[gid * 6 + 1] = pos_a.y;\n"
" p_birds[gid * 6 + 2] = pos_a.z;\n"
" p_birds[gid * 6 + 3] = dir_a.x;\n"
" p_birds[gid * 6 + 4] = dir_a.y;\n"
" p_birds[gid * 6 + 5] = dir_a.z;\n"
"}\n";

const size_t char_simulate_bird_size = simulate_bird_kernel.length();
const char* char_simulate_bird = { (simulate_bird_kernel.c_str()) };


const string calc_flock_avgs_kernel =
"__kernel void calc_flock_avgs(__global float* p_birds, __global float* p_flock_avgs, __global unsigned int* p_flock_ranges)\n"
"{\n"
"	unsigned int flock_index = get_global_id(0);\n"
" unsigned int flock_start = p_flock_ranges[flock_index * 2];\n"
" unsigned int flock_end = p_flock_ranges[flock_index * 2 + 1];\n"
" float3 flock_dir = (float3)(0,0,0);\n"
" float3 flock_pos = (float3)(0,0,0);\n"

// Update flock averages
" for (int i = flock_start; i < flock_end; i++) {\n"
"  flock_dir += vload3(i * 2, p_birds + 3);\n"
"  flock_pos += vload3(i * 2, p_birds); \n"
" }\n"
" float num_of_birds = flock_end - flock_start;\n"
" flock_dir = normalize(flock_dir / (float)num_of_birds);\n"
" flock_pos = flock_pos / (float)num_of_birds;\n"
" p_flock_avgs[flock_index * 6] = flock_dir.x;\n"
" p_flock_avgs[flock_index * 6 + 1] = flock_dir.y;\n"
" p_flock_avgs[flock_index * 6 + 2] = flock_dir.z;\n"
" p_flock_avgs[flock_index * 6 + 3] = flock_pos.x;\n"
" p_flock_avgs[flock_index * 6 + 4] = flock_pos.y;\n"
" p_flock_avgs[flock_index * 6 + 5] = flock_pos.z;\n"
"}\n";

const size_t char_calc_flock_avgs_size = calc_flock_avgs_kernel.length();
const char* char_calc_flock_avgs = { (calc_flock_avgs_kernel.c_str()) };