#include "sim_cpu.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>

#define DECELL_MAX 10.0
#define MAX_PREDECESSORS 4

int find_pred(int *pred, const int node, const net_type *net)
{
    int i = 0;
    for(int j = 0; j < net->nodes_n; j++)
        if(net->weights[j*net->nodes_n + node])
            pred[i++] = j;
    return i;
}

bool simple_inter(const double t, const int from, const int through,
    const double timeout, const double offset, const net_type *net)
{
    int pred[MAX_PREDECESSORS];
    int pred_n = find_pred(pred, through, net);
    return pred[(int)(fmod(t+offset, (double)pred_n*timeout) / timeout)] == from;
}

bool inter_open(const double t, const int from, const int through,
    const int to, const net_type *net) 
{
    if(net->inters[through].type_id == SIMPLE)
        return simple_inter(t, from, through, 
            net->inters_params[net->inters[through].params_start],           // offset
            net->inters_params[net->inters[through].params_start + 1], net); // timeout
    
    return true; // ALWAYS_OPEN and invalid entries   
}

int first_on_next_edge(const int i, const agent_state_type *states, 
    const int *routes, const int ags_n) {
    agent_state_type t;
    t.prev = states[i].next;
    t.next = routes[states[i].route_pos];
    int l = 0, r = ags_n, m;
    while (l < r) {
        m = (l+r)/2;
        if(agents_edge_cmp(states[m], t))
            l = m+1;
        else r = m;
    }
    
    if(states[l].prev == t.prev && states[l].next == t.next)
        return l;
    return -1;
}

double idm_accel(const agent_state_type state, const agent_params_type params,
    const double x_ahead, const double v_ahead) {
    double ss = params.s0 + state.v*params.T + state.v*(state.v-v_ahead)/(2*sqrt(params.a*params.b));
    return params.a*(1 - pow(state.v/params.v0, 4) - pow(ss/(x_ahead-state.x), 2));
}

void agent_sim(agent_state_type *states, const int i, const double t,
    const double t_step, const net_type *net, const agent_state_type *states_prev,
    const agent_params_type *params, const int *routes, const int ags_count)
{
    if(states_prev[i].next < 0)
        return; // agent is inactive

    double x_ahead, v_ahead;
    if(i+1 < ags_count && states_prev[i+1].next == states_prev[i].next && 
        states_prev[i].prev == states_prev[i+1].prev) {
        // there is an agent ahead on the same edge
        x_ahead = states_prev[i+1].x;
        v_ahead = states_prev[i+1].v;
    }
    else if(states_prev[i].route_pos == params[states_prev[i].uid].route_end) {
        // agent is approaching its destination with no agents ahead
        x_ahead = DBL_MAX;
        v_ahead = 0.0;
    }
    //else if (true) {
    else if(inter_open(t, states_prev[i].prev, states_prev[i].next, -1, net)) {
        // there is a open intersection ahead of an agent
        int j = first_on_next_edge(i, states_prev, routes, ags_count);
        if(j != -1) {
            // there is an agent on the next edge
            x_ahead = net->weights[states_prev[i].prev*net->nodes_n + states_prev[i].next]+states_prev[j].x;
            v_ahead = states_prev[j].v;
        }
        else {
            x_ahead = DBL_MAX;
            v_ahead = 0;
        }
    }
    else {
        // there is a closed intersection ahead of an agent
        x_ahead = net->weights[states_prev[i].prev*net->nodes_n + states_prev[i].next];
        v_ahead = 0;
    }

    states[i].x += t_step*states_prev[i].v;
    double accel = idm_accel(states_prev[i], params[states_prev[i].uid], x_ahead, v_ahead);
    if(accel < -DECELL_MAX)
        accel = -DECELL_MAX;
    states[i].v += t_step*accel;
    if(states[i].v < 0.0)
        states[i].v = 0.0;

    if(states[i].x > net->weights[states_prev[i].prev*net->nodes_n + states_prev[i].next]) {
        if(params[states_prev[i].uid].route_end == 0 || 
            states_prev[i].route_pos == params[states_prev[i].uid].route_end)
            states[i].next = -1;
        else {
            states[i].x -= net->weights[states_prev[i].prev*net->nodes_n + states_prev[i].next];
            states[i].prev = states[i].next;
            states[i].next = routes[states[i].route_pos];
            states[i].route_pos++;
        }
    }
}

void sim_cpu(const char* out_filename, const double t_step, const double t_final,
    const net_type *net, const agents_type *ags)
{
    agent_state_type *states      = (agent_state_type*)malloc(ags->count * sizeof(*states));
    agent_state_type *states_prev = (agent_state_type*)malloc(ags->count * sizeof(*states_prev));
    memcpy(states, ags->states, ags->count * sizeof(*ags->states));
    FILE *f = fopen(out_filename, "wb");

    int steps = (int)floor(t_final / t_step);
    fwrite(&t_step, sizeof(t_step), 1, f);
    fwrite(&steps, sizeof(steps), 1, f);
    fwrite(&ags->count, sizeof(ags->count), 1, f);
    for(int i = 0; i < steps; i++) {
        sort_agents(states, ags->count);
        fwrite(states, ags->count * sizeof(*states), 1, f);
        memcpy(states_prev, states, ags->count * sizeof(*states));
        for(int j = 0; j < ags->count; j++)
            agent_sim(states, j, (double)i*t_step, t_step, net, states_prev,
                ags->params, ags->routes, ags->count);
    }

    fclose(f);
    f = NULL;
    free(states);
    states = NULL;
    free(states_prev);
    states_prev = NULL;
}
