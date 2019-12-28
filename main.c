#include <stdio.h>
#include <stdlib.h>

typedef struct __attribute__((__packed__)) {
    double x;
    double v;
    int prev;
    int next;
    int route_pos;
} agent_type;

typedef struct __attribute__((__packed__)) {
    double v0;
    double s0;
    double T;
    double a;
    double b;
    int route_len;
    int *route;
} agent_params_type;


void ic_fromfile(const char* name, double **weights, agent_type **ags,
    agent_params_type **ags_pars, int *nodes_n, int *ags_n)
{   
    FILE *f = fopen("net.bin", "rb");

    fread(nodes_n, sizeof(int), 1, f);
    *weights = malloc((*nodes_n)*(*nodes_n) * sizeof(**weights));
    fread(*weights, sizeof(*weights), (*nodes_n)*(*nodes_n), f);
    fread(ags_n, sizeof(int), 1, f);
    *ags = malloc(*ags_n * sizeof(**ags));
    fread(*ags, sizeof(**ags), *ags_n, f);
    *ags_pars = malloc(*ags_n * sizeof(**ags_pars));
    fread(*ags_pars, sizeof(**ags_pars), *ags_n, f);
    for(int i = 0; i < *ags_n; i++) {
        (*ags_pars)[i].route = malloc((*ags_pars)[i].route_len * sizeof(*(*ags_pars)->route));
        fread((*ags_pars)[i].route, 
            (*ags_pars)[i].route_len * sizeof(*(*ags_pars)->route), 1, f);
    }

    fclose(f);
    f = NULL;
}

int main(void)
{
    int nodes_n, ags_n;
    double *weights;
    agent_type* ags;
    agent_params_type* ags_pars;

    ic_fromfile("net.bin", &weights, &ags, &ags_pars, &nodes_n, &ags_n);

    for(int i = 0; i < ags_n; i++) {
        printf("agent #%d:\n", i);
        printf("\tx = %.1f, v = %.1f, prev = %d, next = %d\n", 
            ags[i].x, ags[i].v, ags[i].prev, ags[i].next);
        printf("\tv0 = %.1f, s0 = %.1f, T = %.1f, a = %.1f, b = %.1f, route_len = %d\n\troute = ", 
            ags_pars[i].v0, ags_pars[i].s0, ags_pars[i].T,
            ags_pars[i].a, ags_pars[i].b, ags_pars[i].route_len);
        for(int j = 0; j < ags_pars[i].route_len-1; j++)
            printf("%d, ", ags_pars[i].route[j]);
        printf("%d\n", ags_pars[i].route[ags_pars[i].route_len-1]);
    }

    free(weights);
    weights = NULL;
    free(ags);
    ags = NULL;
    for(int i = 0; i < ags_n; i++) {
        free(ags_pars[i].route);
        ags_pars[i].route = NULL;
    }
    free(ags_pars);
    ags_pars = NULL;
    return 0;
}