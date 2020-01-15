#ifndef SIM_CPU_H
#define SIM_CPU_H

#include "types.h"

void agent_sim(agent_type*, const int, const double, const double, const net_type*, const agent_type*, const int);
void sim_cpu(const char*, const double, const double, const net_type*, const agent_type*, const int ags_n);

#endif