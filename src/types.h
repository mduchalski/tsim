#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    ALWAYS_OPEN = 0,
    SIMPLE = 1
} _inter_type;

typedef struct {
    _inter_type type_id;
    int params_start;
} inter_type;

typedef struct {
    /* This struct organizes agent's time-invariant parameters */
    double v0;
    double s0;
    double T;
    double a;
    double b;
    int route_end;
} agent_params_type;

typedef struct {
    /* This struct organizes agent's state - set of descriptors that change over
     * time (uid is here for housekeeping) */
    int uid;
    double x;
    double v;
    int prev;
    int next;
    int route_pos;
} agent_state_type;

typedef struct {
    /* This struct organizes a set of agents' complete definitions - states,
     * parameters and routes and is used to store initial conditions
     * states: agents' states, usually at t=0, agent_state_type struct array 
     * params: agents' time-invariant parameters, agent_params_type struct array
     * routes: array of all agents' route's nodes
     * routes_len: size of routes array
     * count: size of states and params arrays */
    agent_state_type* states;
    agent_params_type* params;
    int *routes;
    int routes_len;
    int count;
} agents_type;

typedef struct {
    /* This struct organizes full definition of the road network
     * weights: flattened matrix of edge weights (format analogous to an adjecancy matrix)
     * inters: array of intersections 
     * inters_params: array of intersections' parameters
     * inters_params_len: size of inters_params_len array
     * nodes_n: number of nodes in the network */
    double *weights;
    inter_type *inters;
    double *inters_params;
    int inters_params_len;
    int nodes_n;
} net_type;

void dealloc_net(net_type*);
void dealloc_agents(agents_type*);
bool agents_cmp(const agent_state_type, const agent_state_type);
bool agents_edge_cmp(const agent_state_type, const agent_state_type);
void sort_agents(agent_state_type *ags, const int ags_n);
void ic_fromfile(const char*, net_type*, agents_type*);

#endif