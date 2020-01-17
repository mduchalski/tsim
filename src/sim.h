#ifndef SIM_H
#define SIM_H
#define MAX_PREDECESSORS 4

#include "types.h"

#include <stdbool.h>

int find_pred(int*, const int, const net_type*);
bool simple_inter(const double, const int, const int, const double, const double, const net_type*);
bool inter_open(const double, const int, const int, const int, const net_type*);
int first_on_next_edge(const int, const agent_state_type*, const int*, const int);
double idm_accel(const agent_state_type, const agent_params_type, const double, const double);

#endif
