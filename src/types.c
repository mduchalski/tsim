#include "types.h"

#include <stdlib.h>
#include <stdio.h>

#define SQR(x) x*x

void dealloc_net(net_type *net)
{
    free(net->inters);
    net->inters = NULL;
    free(net->inters_params);
    net->inters_params = NULL;
    free(net->weights);
    net->weights = NULL;
}

void dealloc_agents(agents_type *ags)
{
    free(ags->states);
    ags->states = NULL;
    free(ags->params);
    ags->params = NULL;
    free(ags->routes);
    ags->routes = NULL;
}

bool agents_cmp(const agent_state_type a, const agent_state_type b)
{   
    // comparison function for lexicographical sorting of agent_state_type struct arrays
    // with order (prev, next, x)
    if(a.prev != b.prev)
        return a.prev < b.prev;
    else if(a.next != b.next)
        return a.next < b.next;
    else return a.x < b.x;
}

bool agents_edge_cmp(const agent_state_type a, const agent_state_type b)
{
    if(a.prev != b.prev)
        return a.prev < b.prev;
    else if(a.next != b.next)
        return a.next < b.next;
    else return 0;
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

void ic_fromfile(const char* filename, net_type *net, agents_type *ags)
{
    FILE *f = fopen(filename, "rb");

    fread(&net->nodes_n, sizeof(net->nodes_n), 1, f);
    net->weights = (double*) malloc(SQR(net->nodes_n) * sizeof(*net->weights));
    fread(net->weights, sizeof(*net->weights), SQR(net->nodes_n), f);

    fread(&ags->count, sizeof(ags->count), 1, f);
    ags->states = (agent_state_type*) malloc(ags->count * sizeof(*ags->states));
    fread(ags->states, sizeof(*ags->states), ags->count, f);
    
    ags->params = (agent_params_type*) malloc(ags->count * sizeof(agent_params_type));
    fread(ags->params, sizeof(*ags->params), ags->count, f);
    
    fread(&ags->routes_len, sizeof(ags->routes_len), 1, f);
    ags->routes = (int*) malloc(ags->routes_len * sizeof(*ags->routes));
    fread(ags->routes, sizeof(*ags->routes), ags->routes_len, f);

    net->inters = (inter_type*) malloc(net->nodes_n * sizeof(*net->inters));
    fread(net->inters, sizeof(*net->inters), net->nodes_n, f);

    fread(&net->inters_params_len, sizeof(net->inters_params_len), 1, f);
    net->inters_params = (double*) malloc(net->inters_params_len * sizeof(*net->inters_params));
    fread(net->inters_params, sizeof(*net->inters_params), net->inters_params_len, f);

    fclose(f);
    f = NULL;
}