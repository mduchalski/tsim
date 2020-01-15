#include "sim.h"

#include <math.h>

int find_pred(int *pred, const int node, const net_type *net)
{
    int i = 0;
    for(int j = 0; j < net->nodes_n; j++)
        if(net->weights[j][node])
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
        return simple_inter(t, from, through, net->inters[through].params[0],
            net->inters[through].params[1], net);
    
    return true; // ALWAYS_OPEN and invalid entries   
}

void sort_agents(agent_type *ags, const int ags_n) {
    int j;
    agent_type tmp;

    for(int i = 1; i < ags_n; i++) {
        tmp = ags[i];
        for(j = i-1; j >= 0 && agents_cmp(tmp, ags[j]); j--)
            ags[j+1] = ags[j];
        ags[j+1] = tmp;
    }
}

int first_on_next_edge(const int i, const agent_type *ags, const int ags_n) {
    agent_type t;
    t.prev = ags[i].next;
    t.next = ags[i].params->route[ags[i].route_pos];
    int l = 0, r = ags_n, m;
    while (l < r) {
        m = (l+r)/2;
        if(agents_edge_cmp(ags[m], t))
            l = m+1;
        else r = m;
    }
    
    if(ags[l].prev == t.prev && ags[l].next == t.next)
        return l;
    return -1;
}

double idm_accel(const agent_type ag, double x_ahead, double v_ahead) {
    double ss = ag.params->s0 + ag.v*ag.params->T + ag.v*(ag.v-v_ahead)/(2*sqrt(ag.params->a*ag.params->b));
    return ag.params->a*(1 - pow(ag.v/ag.params->v0, 4) - pow(ss/(x_ahead-ag.x), 2));
}
