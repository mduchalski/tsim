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

int main(void)
{
    FILE *f = fopen("net.bin", "rb");
    
    int n, m;
    double *weights;
    agent_type* agents;
    agent_params_type* agents_params;

    fread(&n, sizeof(int), 1, f);
    weights = malloc(n*n * sizeof(*weights));
    fread(weights, sizeof(*weights), n*n, f);
    fread(&m, sizeof(int), 1, f);
    agents = malloc(m * sizeof(*agents));
    fread(agents, sizeof(*agents), m, f);
    agents_params = malloc(m * sizeof(*agents_params));
    for(int i = 0; i < m; i++) {
        fread(&agents_params[i],
            sizeof(*agents_params)-sizeof((*agents_params).route), 1, f);
        agents_params[i].route = malloc(
            agents_params[i].route_len * sizeof(*agents_params->route));
        fread(agents_params[i].route, 
            agents_params[i].route_len * sizeof(*agents_params->route), 1, f);
    }

    for(int i = 0; i < m; i++) {
        printf("agent #%d:\n", i);
        printf("\tx = %.1f, v = %.1f, prev = %d, next = %d\n", 
            agents[i].x, agents[i].v, agents[i].prev, agents[i].next);
        printf("\tv0 = %.1f, s0 = %.1f, T = %3.1f, a = %.1f, b= %.1f\n\troute = ", 
            agents_params[i].v0, agents_params[i].s0, agents_params[i].T,
            agents_params[i].a, agents_params[i].b);
        for(int j = 0; j < agents_params[i].route_len-1; j++)
            printf("%d, ", agents_params[i].route[j]);
        printf("%d\n", agents_params[i].route[agents_params[i].route_len-1]);
    }

    free(weights);
    weights = NULL;
    free(agents);
    agents = NULL;
    for(int i = 0; i < m; i++) {
        free(agents_params[i].route);
        agents_params[i].route = NULL;
    }
    fclose(f);
    f = NULL;
    return 0;
}