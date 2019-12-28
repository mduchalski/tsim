#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

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

void ic_fromfile(const char* name, double **weights, agent_type **ags, int *nodes_n, int *ags_n)
{
    FILE *f = fopen("net.bin", "rb");

    fread(nodes_n, sizeof(int), 1, f);
    *weights = malloc((*nodes_n)*(*nodes_n) * sizeof(**weights));
    fread(*weights, sizeof(*weights), (*nodes_n)*(*nodes_n), f);
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


void sort(agent_type *ags, const int ags_n) {
    int j;
    agent_type tmp;

    for(int i = 2; i < ags_n; i++) {
        tmp = ags[i];
        for(j = i-1; j >= 0 && agents_cmp(tmp, ags[j]); j--)
            ags[j+1] = ags[j];
        ags[j+1] = tmp;
    }
}

void _print_agents(const agent_type *ags, const int ags_n)
{
    for(int i = 0; i < ags_n; i++) {
        printf("agent #%d:\n", i);
        printf("\tx = %.1f, v = %.1f, prev = %d, next = %d\n", 
            ags[i].x, ags[i].v, ags[i].prev, ags[i].next);
        printf("\tv0 = %.1f, s0 = %.1f, T = %.1f, a = %.1f, b = %.1f, route_len = %d\n\troute = ", 
            ags[i].params->v0, ags[i].params->s0, ags[i].params->T,
            ags[i].params->a, ags[i].params->b, ags[i].params->route_len);
        for(int j = 0; j < ags[i].params->route_len - 1; j++)
            printf("%d, ", ags[i].params->route[j]);
        if(ags[i].params->route_len)
            printf("%d", ags[i].params->route[ags[i].params->route_len - 1]);
        printf("\n");
    }
}

int main(void)
{
    int nodes_n, ags_n;
    double *weights;
    agent_type* ags;

    ic_fromfile("net.bin", &weights, &ags, &nodes_n, &ags_n);

    _print_agents(ags, ags_n);
    //sort(ags, ags_n);
    //printf("-----\n");
    //_print_agents(ags, ags_pars, ags_n);

    free(weights);
    weights = NULL;
    for(int i = 0; i < ags_n; i++) {
        free(ags[i].params->route);
        ags[i].params->route = NULL;
    }
    free(ags[0].params);
    ags[0].params = NULL;
    free(ags);
    ags = NULL;
    return 0;
}