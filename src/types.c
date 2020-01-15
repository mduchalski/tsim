#include "types.h"

#include <stdlib.h>

void dealloc_net(net_type *net)
{
    for(int i = 0; i < net->nodes_n; i++) {
        free(net->inters[i].params);
        net->inters[i].params = NULL;
    }
    free(net->inters);
    net->inters = NULL;
    free(*net->weights);
    *net->weights = NULL;
    free(net->weights);
    net->weights = NULL;
}

void dealloc_agents(agent_type *ags, const int ags_n)
{
    int zero_uid = 0;
    for(int i = 0; i < ags_n; i++) {
        if(ags[i].uid == 0)
            zero_uid = i;

        free(ags[i].params->route);
        ags[i].params->route = NULL;
    }
    free(ags[zero_uid].params);
    ags[zero_uid].params = NULL;
    free(ags);
    ags = NULL;
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
