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
    int route_len;
    int *route;
} agent_params_type;

typedef struct __attribute__((__packed__)) {
    int uid;
    double x;
    double v;
    int prev;
    int next;
    int route_pos;
    agent_params_type *params;
} agent_type;

typedef struct __attribute__((__packed__)) {
    _inter_type type_id;
    double *params;
} inter_type;

typedef struct {
    double **weights;
    inter_type *inters;
    int nodes_n;
} net_type;

void dealloc_net(net_type*);
void dealloc_agents(agent_type*, const int);
bool agents_cmp(const agent_type, const agent_type);
bool agents_edge_cmp(const agent_type, const agent_type);

#endif