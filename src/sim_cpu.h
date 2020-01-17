#ifdef __cplusplus
extern "C" {
#endif

#ifndef SIM_CPU_H
#define SIM_CPU_H

#include "types.h"

void agent_sim(agent_state_type*, const int, const double, const double, const net_type*,
    const agent_state_type*, const agent_params_type*, const int*, const int);
void sim_cpu(const char*, const double, const double, const net_type*, const agents_type*);

#endif

#ifdef __cplusplus
}
#endif