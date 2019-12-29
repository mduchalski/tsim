#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <float.h>
#include <math.h>

typedef struct __attribute__((__packed__)) {
    int uid;
    double v0;
    double s0;
    double T;
    double a;
    double b;
    int route_len;
    int *route;
} agent_params_type;

typedef struct __attribute__((__packed__)) {
    double x;
    double v;
    int prev;
    int next;
    int route_pos;
    agent_params_type *params;
} agent_type;

void ic_fromfile(const char* name, double ***weights, agent_type **ags, int *nodes_n, int *ags_n)
{
    FILE *f = fopen("net.bin", "rb");

    fread(nodes_n, sizeof(int), 1, f);

    *weights = malloc((*nodes_n) * sizeof(**weights));
    (*weights)[0] = malloc((*nodes_n)*(*nodes_n) * sizeof(***weights));
    for(int i = 1; i < *nodes_n; i++)
        (*weights)[i] = (*weights)[0] + (*nodes_n)*i;
    fread((*weights)[0], sizeof(***weights), (*nodes_n)*(*nodes_n), f);

    fread(ags_n, sizeof(int), 1, f);
    *ags = malloc(*ags_n * sizeof(**ags));
    fread(*ags, sizeof(**ags), *ags_n, f);
    
    agent_params_type *ags_pars = malloc(*ags_n * sizeof(agent_params_type));
    fread(ags_pars, sizeof(*ags_pars), *ags_n, f);
    int route_bytes;
    for(int i = 0; i < *ags_n; i++) {
        route_bytes = ags_pars[i].route_len * sizeof(*ags_pars->route);
        ags_pars[i].route = malloc(route_bytes);
        fread(ags_pars[i].route, route_bytes, 1, f);
        (*ags)[i].params = ags_pars + i;
    }

    fclose(f);
    f = NULL;
}

bool agents_cmp(const agent_type a, const agent_type b)
{   
    // comparison function for lexicographical sorting of agent_type struct arrays
    // with order (prev, next, x)
    if(a.prev != b.prev)
        return a.prev < b.prev;
    else if(a.next != b.next)
        return a.next < b.next;
    else return a.x < b.x;
}

int agents_edge_cmp(const agent_type a, const agent_type b)
{
    if(a.prev != b.prev)
        return a.prev < b.prev ? -1 : 1;
    else if(a.next != b.next)
        return a.next < b.next ? -1 : 1;
    else return 0;
}

void sort_agents(agent_type *ags, const int ags_n) {
    int j;
    agent_type tmp;

    for(int i = 2; i < ags_n; i++) {
        tmp = ags[i];
        for(j = i-1; j >= 0 && agents_cmp(tmp, ags[j]); j--)
            ags[j+1] = ags[j];
        ags[j+1] = tmp;
    }
}

void _print_agent(const agent_type ag)
{
    printf("\tx = %.1f, v = %.1f, prev = %d, next = %d, route_pos = %d\n"
        "\tuid = %d, v0 = %.1f, s0 = %.1f, T = %.1f, a = %.1f, b = %.1f, route_len" 
        " = %d\n\troute = ", ag.x, ag.v, ag.prev, ag.next, ag.route_pos,
        ag.params->uid, ag.params->v0, ag.params->s0, ag.params->T, ag.params->a,
        ag.params->b, ag.params->route_len);
        for(int i = 0; i < ag.params->route_len - 1; i++)
            printf("%d, ", ag.params->route[i]);
        if(ag.params->route_len)
            printf("%d", ag.params->route[ag.params->route_len - 1]);
        printf("\n");
}

void _print_agents(const agent_type *ags, const int ags_n)
{
    for(int i = 0; i < ags_n; i++) {
        printf("agent #%d:\n", i);
        _print_agent(ags[i]);
    }
}



void dealloc_agents(agent_type *ags, const int ags_n)
{
    int zero_uid = 0;
    for(int i = 0; i < ags_n; i++) {
        if(ags[i].params->uid == 0)
            zero_uid = i;

        free(ags[i].params->route);
        ags[i].params->route = NULL;
    }
    free(ags[zero_uid].params);
    ags[zero_uid].params = NULL;
    free(ags);
    ags = NULL;
}

int find_on_edge(const int prev, const int next, const agent_type *ags, const int ags_n) {
    agent_type t;
    t.prev = prev;
    t.next = next;
    int l = 0, r = ags_n-1, m, cmp;
    while(l <= r) {
        m = (l+r)/2;
        cmp = agents_edge_cmp(ags[m], t);
        if(cmp < 0)      l = m+1;
        else if(cmp > 0) r = m-1;
        else return m;
    }
    return -1;
}

int first_on_next_edge(const int i, const agent_type *ags, const int ags_n) {
    int j;
    for(j = find_on_edge(ags[i].next, ags[i].params->route[ags[i].route_pos], ags, ags_n);
        j > 0 && !agents_edge_cmp(ags[j], ags[j+1]); j--);
    return j;
}

double idm_accel(const agent_type ag, double x_ahead, double v_ahead) {
    double ss = ag.params->s0 + ag.v*(ag.v-v_ahead) / (2*sqrt(ag.params->a*ag.params->b));
    return ag.params->a*(1 - pow(ag.v/ag.params->v0, 4) - pow(ss/(x_ahead-ag.x), 2));
}

void agent_sim(const int i, agent_type *ags, const agent_type *ags_prev, const int ags_n,
    double **weights, double t_step) {
    if(ags_prev[i].next < 0)
        return; // agent is inactive

    double x_ahead, v_ahead;
    if(i+1 < ags_n && ags_prev[i+1].next == ags_prev[i].next && 
        ags_prev[i+1].prev == ags_prev[i+1].prev) {
        // there is an agent ahead on the same edge
        x_ahead = ags_prev[i+1].x;
        v_ahead = ags_prev[i+1].v;
    }
    else if(ags_prev[i].route_pos == ags_prev[i].params->route_len) {
        // agent is approaching its destination with no agents ahead
        x_ahead = DBL_MAX;
        v_ahead = 0.0;
    }
    else if(true) { // for now, all intersections are open to everyone
        // there is a open intersection ahead of an agent
        int j = first_on_next_edge(i, ags_prev, ags_n);
        if(j != -1) {
            // there is an agent on the next edge
            x_ahead = weights[ags_prev[i].prev][ags_prev[i].next]+ags_prev[j].x;
            v_ahead = ags_prev[j].v;
        }
        else {
            x_ahead = DBL_MAX;
            v_ahead = 0;
        }
    }
    else {
        // there is a closed intersection ahead of an agent
        x_ahead = weights[ags_prev[i].prev][ags_prev[i].next];
        v_ahead = 0;
    }

    ags[i].x += t_step*ags[i].v;
    ags[i].v += t_step*idm_accel(ags[i], x_ahead, v_ahead);
    
    if(ags[i].x > weights[ags_prev[i].prev][ags_prev[i].next]) {
        ags[i].x -= weights[ags_prev[i].prev][ags_prev[i].next];
        if(ags[i].route_pos == ags[i].params->route_len+1)
            ags[i].next = -1;
        else {
            ags[i].prev = ags[i].next;
            ags[i].next = ags[i].params->route[ags[i].route_pos];
            ags[i].route_pos++;
        }
    }
}


void sim_cpu(const double t_step, const double t_end,
    const agent_type *ags_ic, double **weights, const int ags_n) {
    
    agent_type *ags = malloc(ags_n * sizeof(*ags));
    agent_type *ags_prev = malloc(ags_n * sizeof(*ags_prev));
    memcpy(ags, ags_ic, ags_n * sizeof(*ags_ic));
    sort_agents(ags, ags_n);

    for(double t = 0.0; t < t_end; t += t_step) {
        _print_agents(ags, ags_n);
        memcpy(ags_prev, ags, ags_n * sizeof(*ags));
        for(int i = 0; i < ags_n; i++)
            agent_sim(i, ags, ags_prev, ags_n, weights, t_step);
        sort_agents(ags, ags_n);
    }

    free(ags);
    ags = NULL;
    free(ags_prev);
    ags_prev = NULL;
}


int main(void)
{
    int nodes_n, ags_n;
    double **weights;
    agent_type* ags_ic;
    ic_fromfile("net.bin", &weights, &ags_ic, &nodes_n, &ags_n);
    sim_cpu(0.2, 30, ags_ic, weights, ags_n);
    dealloc_agents(ags_ic, ags_n);
    return 0;
}