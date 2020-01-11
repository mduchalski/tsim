#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <unistd.h>

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
    FILE *f = fopen(name, "rb");

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

const int MAX_PREDECESSORS = 4;

int find_pred(int *pred, const int node, double** weights, const int nodes_n)
{
    int i = 0;
    for(int j = 0; j < nodes_n; j++)
        if(weights[j][node])
            pred[i++] = j;
    return i;
}

bool simple_inter(const double t, const int from, const int through,
    const int to, double** weights, const int nodes_n)
{
    int pred[MAX_PREDECESSORS];
    int pred_n = find_pred(pred, through, weights, nodes_n);
    double offset = 0.0, timeout = 10.0; // debug only!
    return (int)( fmod(t+offset, (double)pred_n*timeout) / timeout ) == from;
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

bool agents_edge_cmp(const agent_type a, const agent_type b)
{
    if(a.prev != b.prev)
        return a.prev < b.prev;
    else if(a.next != b.next)
        return a.next < b.next;
    else return 0;
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

const double DECELL_MAX = 10.0;

void agent_sim(const double t, const int i, agent_type *ags, const agent_type *ags_prev,
    const int ags_n, double **weights, const int nodes_n, const double t_step) {
    if(ags_prev[i].next < 0)
        return; // agent is inactive

    double x_ahead, v_ahead;
    if(i+1 < ags_n && ags_prev[i+1].next == ags_prev[i].next && 
        ags_prev[i].prev == ags_prev[i+1].prev) {
        // there is an agent ahead on the same edge
        x_ahead = ags_prev[i+1].x;
        v_ahead = ags_prev[i+1].v;
    }
    else if(ags_prev[i].route_pos == ags_prev[i].params->route_len) {
        // agent is approaching its destination with no agents ahead
        x_ahead = DBL_MAX;
        v_ahead = 0.0;
    }
    else if (true) {
    //else if(simple_inter(t, ags_prev[i].prev, ags_prev[i].next, -1, weights, nodes_n)) { // for now, all intersections are open to everyone
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

    ags[i].x += t_step*ags_prev[i].v;
    double accel = idm_accel(ags_prev[i], x_ahead, v_ahead);
    if(accel < -DECELL_MAX)
        accel = -DECELL_MAX;
    ags[i].v += t_step*accel;
    if(ags[i].v < 0.0)
        ags[i].v = 0.0;

    if(ags[i].x > weights[ags_prev[i].prev][ags_prev[i].next]) {
        if(ags_prev[i].params->route_len == 0 || 
            ags_prev[i].route_pos == ags_prev[i].params->route_len)
            ags[i].next = -1;
        else {
            ags[i].x -= weights[ags_prev[i].prev][ags_prev[i].next];
            ags[i].prev = ags[i].next;
            ags[i].next = ags[i].params->route[ags[i].route_pos];
            ags[i].route_pos++;
        }
    }
}


void sim_cpu(const double t_step, const double t_end, const agent_type *ags_ic,
    double **weights, const int nodes_n, const int ags_n, const char* out_filename) {
    agent_type *ags = malloc(ags_n * sizeof(*ags));
    agent_type *ags_prev = malloc(ags_n * sizeof(*ags_prev));
    memcpy(ags, ags_ic, ags_n * sizeof(*ags_ic));
    FILE *f = fopen(out_filename, "wb");

    int steps = (int)floor(t_end / t_step);
    fwrite(&steps, sizeof(steps), 1, f);
    fwrite(&ags_n, sizeof(ags_n), 1, f);
    for(int i = 0; i < steps; i++) {
        sort_agents(ags, ags_n);
        fwrite(ags, ags_n * sizeof(*ags), 1, f);
        memcpy(ags_prev, ags, ags_n * sizeof(*ags));
        for(int j = 0; j < ags_n; j++)
            agent_sim((double)i*t_step, j, ags, ags_prev, ags_n, weights, nodes_n, t_step);
    }

    fclose(f);
    f = NULL;
    free(ags);
    ags = NULL;
    free(ags_prev);
    ags_prev = NULL;
}


int main(int argc, char *argv[])
{
    char *in_filename = "ic.bin", *out_filename = "res.bin";
    double t_step = 0.1, t_final = 10.0;
    char opt;
    while((opt = getopt(argc, argv, "i:o:s:f:h")) != -1) {
        switch(opt) {
            case 's':
                t_step = atof(optarg);
                break;
            case 'f':
                t_final = atof(optarg);
                break;
            case 'i':
                in_filename = strdup(optarg);
                break;
            case 'o':
                out_filename = strdup(optarg);
                break;
            case 'h':
                printf("Usage: %s [-h] [-i IN] [-o OUT] [-s STEP] [-f FINAL]\n\n"
                    "Optional parameters:\n"
                    "-h:        displays this message\n"
                    "-i IN:     initial conditions filename      (default: ic.bin)\n"
                    "-o OUT:    simulation output filename       (default: res.bin)\n"
                    "-s STEP:   simulation step in seconds       (default: 0.1)\n"
                    "-f FINAL:  simulation stop time in seconds  (default: 10.0)\n", argv[0]);
                return 0;
        }
    }

    int nodes_n, ags_n;
    double **weights;
    agent_type* ags_ic;
    ic_fromfile(in_filename, &weights, &ags_ic, &nodes_n, &ags_n);
    sim_cpu(t_step, t_final, ags_ic, weights, nodes_n, ags_n, out_filename);
    
    dealloc_agents(ags_ic, ags_n);
    return 0;
}