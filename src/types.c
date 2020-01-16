#include "types.h"

#include <stdlib.h>

void dealloc_net(net_type *net)
{
    free(net->inters);
    net->inters = NULL;
    free(net->inters_params);
    net->inters_params = NULL;
    free(*net->weights);
    *net->weights = NULL;
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
