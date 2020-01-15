#include "sim_cpu.h"
#include "sim.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>

#define DECELL_MAX 10.0

void agent_sim(agent_type *ags, const int i, const double t, const double t_step,
    const net_type *net, const agent_type *ags_prev, const int ags_n)
{
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
    //else if (true) {
    else if(inter_open(t, ags_prev[i].prev, ags_prev[i].next, -1, net)) {
        // there is a open intersection ahead of an agent
        int j = first_on_next_edge(i, ags_prev, ags_n);
        if(j != -1) {
            // there is an agent on the next edge
            x_ahead = net->weights[ags_prev[i].prev][ags_prev[i].next]+ags_prev[j].x;
            v_ahead = ags_prev[j].v;
        }
        else {
            x_ahead = DBL_MAX;
            v_ahead = 0;
        }
    }
    else {
        // there is a closed intersection ahead of an agent
        x_ahead = net->weights[ags_prev[i].prev][ags_prev[i].next];
        v_ahead = 0;
    }

    ags[i].x += t_step*ags_prev[i].v;
    double accel = idm_accel(ags_prev[i], x_ahead, v_ahead);
    if(accel < -DECELL_MAX)
        accel = -DECELL_MAX;
    ags[i].v += t_step*accel;
    if(ags[i].v < 0.0)
        ags[i].v = 0.0;

    if(ags[i].x > net->weights[ags_prev[i].prev][ags_prev[i].next]) {
        if(ags_prev[i].params->route_len == 0 || 
            ags_prev[i].route_pos == ags_prev[i].params->route_len)
            ags[i].next = -1;
        else {
            ags[i].x -= net->weights[ags_prev[i].prev][ags_prev[i].next];
            ags[i].prev = ags[i].next;
            ags[i].next = ags[i].params->route[ags[i].route_pos];
            ags[i].route_pos++;
        }
    }
}

void sim_cpu(const char* out_filename, const double t_step, const double t_final,
    const net_type *net, const agent_type *ags_ic, const int ags_n)
{
    agent_type *ags = malloc(ags_n * sizeof(*ags));
    agent_type *ags_prev = malloc(ags_n * sizeof(*ags_prev));
    memcpy(ags, ags_ic, ags_n * sizeof(*ags_ic));
    FILE *f = fopen(out_filename, "wb");

    int steps = (int)floor(t_final / t_step);
    fwrite(&t_step, sizeof(t_step), 1, f);
    fwrite(&steps, sizeof(steps), 1, f);
    fwrite(&ags_n, sizeof(ags_n), 1, f);
    for(int i = 0; i < steps; i++) {
        sort_agents(ags, ags_n);
        fwrite(ags, ags_n * sizeof(*ags), 1, f);
        memcpy(ags_prev, ags, ags_n * sizeof(*ags));
        for(int j = 0; j < ags_n; j++)
            agent_sim(ags, j, (double)i*t_step, t_step, net, ags_prev, ags_n);
    }

    fclose(f);
    f = NULL;
    free(ags);
    ags = NULL;
    free(ags_prev);
    ags_prev = NULL;
}
