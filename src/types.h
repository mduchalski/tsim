#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    ALWAYS_OPEN = 0,
    SIMPLE = 1
} _inter_type;

typedef struct __attribute__((__packed__)) {
    double v0;
    double s0;
    double T;
    double a;
    double b;
    int route_end;
} agent_params_type;

typedef struct __attribute__((__packed__)) {
    int uid;
    double x;
    double v;
    int prev;
    int next;
    int route_pos;
} agent_state_type;

typedef struct __attribute__((__packed__)) {
    _inter_type type_id;
    double *params;
} inter_type;

typedef struct {
    agent_state_type* states;
    agent_params_type* params;
    int *routes;
    int count;
} agents_type;

typedef struct {
    double **weights;
    inter_type *inters;
    int nodes_n;
} net_type;

void dealloc_net(net_type*);
void dealloc_agents(agents_type*);
bool agents_cmp(const agent_state_type, const agent_state_type);
bool agents_edge_cmp(const agent_state_type, const agent_state_type);

#endif