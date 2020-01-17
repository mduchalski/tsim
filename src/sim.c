#include "sim.h"

#include <math.h>

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

void sort_agents(agent_state_type *ags, const int ags_n) {
    int j;
    agent_state_type tmp;

    for(int i = 1; i < ags_n; i++) {
        tmp = ags[i];
        for(j = i-1; j >= 0 && agents_cmp(tmp, ags[j]); j--)
            ags[j+1] = ags[j];
        ags[j+1] = tmp;
    }
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
