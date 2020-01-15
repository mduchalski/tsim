#ifndef SIM_H
#define SIM_H
#define MAX_PREDECESSORS 4

#include "types.h"

#include <stdbool.h>

int find_pred(int*, const int, const net_type*);
bool simple_inter(const double, const int, const int, const double, const double, const net_type*);
bool inter_open(const double, const int, const int, const int, const net_type*);
void sort_agents(agent_type *ags, const int ags_n);
int first_on_next_edge(const int, const agent_type*, const int);
double idm_accel(const agent_type, double, double);

#endif
