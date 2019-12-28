#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

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

bool agents_cmp(const agent_type a, const agent_type b)
{   
    // comparison function for lexicographical sorting of agent_type struct arrays
    // with order (prev, next, x)
    if(a.prev != b.prev)
        return a.prev < b.prev;
    else if(a.next != b.next)
        return b.next < b.next;
    else return a.x < b.x;
}

int bst_closest(const agent_type to, const agent_type* ags, const int ags_n)
{
    int l = 0, r = ags_n-1, m;
    while(l <= r) {
        m = (l + r)/2;
        if( agents_cmp(ags[m], to) ) {
            if( !agents_cmp(ags[m+1], to) )
                return m; 
            l = m+1;
        }
        else {
            if( agents_cmp(ags[m-1], to) )
                return m; 
            r = m-1;
        }
    }
    return -1;
}

void fix_unsorted(agent_type* ags, const int ags_n, const int i)
{
    // restore order in an agent_type struct array where a single, specified
    // element is out of place
    agent_type tmp = ags[i];
    int ni = bst_closest(tmp, ags, ags_n);
    if(ni > i)
        memcpy(ags + i, ags + i+1, (ni-i)*sizeof(*ags));
    else
        memcpy(ags + ni+1, ags + ni, (i-ni)*sizeof(*ags));
    ags[ni] = tmp;
}

void _print_agents(const agent_type *ags, const agent_params_type *ags_pars,
    const int ags_n)
{
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
}

int main(void)
{
    int nodes_n, ags_n;
    double *weights;
    agent_type* ags;
    agent_params_type* ags_pars;

    ic_fromfile("net.bin", &weights, &ags, &ags_pars, &nodes_n, &ags_n);

    _print_agents(ags, ags_pars, ags_n);
    ags[0].prev = 3;
    ags[0].next = 2;
    fix_unsorted(ags, ags_n, 0);
    printf("---\n");
    _print_agents(ags, ags_pars, ags_n);

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