#include "sim_gpu.h"

#include <stdio.h>
#include <float.h>

#define MAX_PREDECESSORS 4
#define DECELL_MAX 10.0

__device__ int find_pred(int *pred, const int node, const net_type net)
{
    int i = 0;
    for(int j = 0; j < net.nodes_n; j++)
        if(net.weights[j*net.nodes_n + node])
            pred[i++] = j;
    return i;
}

__device__ bool simple_inter(const double t, const int from, const int through,
    const double timeout, const double offset, const net_type net)
{
    int pred[MAX_PREDECESSORS];
    int pred_n = find_pred(pred, through, net);
    return pred[(int)(fmod(t+offset, (double)pred_n*timeout) / timeout)] == from;
}

__device__ bool inter_open(const double t, const int from, const int through,
    const int to, const net_type net) 
{
    if(net.inters[through].type_id == SIMPLE)
        return simple_inter(t, from, through, 
            net.inters_params[net.inters[through].params_start],           // offset
            net.inters_params[net.inters[through].params_start + 1], net); // timeout
    
    return true; // ALWAYS_OPEN and invalid entries   
}

__device__ bool agents_edge_cmp_gpu(const agent_state_type a, const agent_state_type b)
{
    if(a.prev != b.prev)
        return a.prev < b.prev;
    else if(a.next != b.next)
        return a.next < b.next;
    else return 0;
}

__device__ int first_on_next_edge(const int i, const agent_state_type *states, 
    const int *routes, const int ags_n) {
    agent_state_type t;
    t.prev = states[i].next;
    t.next = routes[states[i].route_pos];
    int l = 0, r = ags_n, m;
    while (l < r) {
        m = (l+r)/2;
        if(agents_edge_cmp_gpu(states[m], t))
            l = m+1;
        else r = m;
    }
    
    if(states[l].prev == t.prev && states[l].next == t.next)
        return l;
    return -1;
}

__device__ double idm_accel(const agent_state_type state, const agent_params_type params,
    const double x_ahead, const double v_ahead) {
    double ss = params.s0 + state.v*params.T + state.v*(state.v-v_ahead)/(2*sqrt(params.a*params.b));
    return params.a*(1 - pow(state.v/params.v0, 4) - pow(ss/(x_ahead-state.x), 2));
}

__global__ void agent_sim(agent_state_type *states, const double t,
    const double t_step, const net_type net, const agent_state_type *states_prev,
    const agent_params_type *params, const int *routes, const int ags_count)
{
    int i = blockDim.x*blockIdx.x + threadIdx.x;
    if (i >= ags_count)
        return;
    
    if(states_prev[i].next < 0)
        return; // agent is inactive
    
    double x_ahead, v_ahead;
    
    if(i+1 < ags_count && states_prev[i+1].next == states_prev[i].next && 
        states_prev[i].prev == states_prev[i+1].prev) {
        // there is an agent ahead on the same edge
        x_ahead = states_prev[i+1].x;
        v_ahead = states_prev[i+1].v;
    }
    else if(states_prev[i].route_pos == params[states_prev[i].uid].route_end) {
        // agent is approaching its destination with no agents ahead
        x_ahead = DBL_MAX;
        v_ahead = 0.0;
    }
    else if(inter_open(t, states_prev[i].prev, states_prev[i].next, -1, net)) {
        // there is a open intersection ahead of an agent
        int j = first_on_next_edge(i, states_prev, routes, ags_count);
        if(j != -1) {
            // there is an agent on the next edge
            x_ahead = net.weights[states_prev[i].prev*net.nodes_n + states_prev[i].next]+states_prev[j].x;
            v_ahead = states_prev[j].v;
        }
        else {
            x_ahead = DBL_MAX;
            v_ahead = 0;
        }
    }
    else {
        // there is a closed intersection ahead of an agent
        x_ahead = net.weights[states_prev[i].prev*net.nodes_n + states_prev[i].next];
        v_ahead = 0;
    }

    states[i].x += t_step*states_prev[i].v;
    double accel = idm_accel(states_prev[i], params[states_prev[i].uid], x_ahead, v_ahead);
    if(accel < -DECELL_MAX)
        accel = -DECELL_MAX;
    states[i].v += t_step*accel;
    if(states[i].v < 0.0)
        states[i].v = 0.0;


    if(states[i].x > net.weights[states_prev[i].prev*net.nodes_n + states_prev[i].next]) {
        if(states_prev[i].route_pos == params[states_prev[i].uid].route_end) {
            states[i].next = -1;

        }
        else {
            states[i].x -= net.weights[states_prev[i].prev*net.nodes_n + states_prev[i].next];
            states[i].prev = states[i].next;
            states[i].next = routes[states[i].route_pos];
            states[i].route_pos++;
        }
    }
}

#define SQR(x) x*x

void copy_to_gpu(net_type *net, agents_type *ags,
    const net_type *h_net, const agents_type *h_ags) 
{
    cudaMalloc(&net->weights, SQR(h_net->nodes_n) * sizeof(*net->weights));
    cudaMalloc(&net->inters, h_net->nodes_n * sizeof(*net->inters));
    cudaMalloc(&net->inters_params, h_net->inters_params_len * sizeof(*net->inters_params));
    cudaMalloc(&ags->states, h_ags->count * sizeof(*ags->states));
    cudaMalloc(&ags->params, h_ags->count * sizeof(*ags->params));
    cudaMalloc(&ags->routes, h_ags->routes_len * sizeof(*ags->routes));

    cudaMemcpy(net->weights, h_net->weights,
        SQR(h_net->nodes_n) * sizeof(*net->weights), cudaMemcpyHostToDevice);
    cudaMemcpy(net->inters, h_net->inters,
        h_net->nodes_n * sizeof(*net->inters), cudaMemcpyHostToDevice);
    cudaMemcpy(net->inters_params, h_net->inters_params,
        h_net->inters_params_len * sizeof(*net->inters_params), cudaMemcpyHostToDevice);
    cudaMemcpy(ags->states, h_ags->states,
        h_ags->count * sizeof(*ags->states), cudaMemcpyHostToDevice);
    cudaMemcpy(ags->params, h_ags->params,
        h_ags->count * sizeof(*ags->params), cudaMemcpyHostToDevice);
    cudaMemcpy(ags->routes, h_ags->routes,
        h_ags->routes_len * sizeof(*ags->routes), cudaMemcpyHostToDevice);
    net->nodes_n = h_net->nodes_n;
}

void dealloc_gpu(net_type *net, agents_type *ags)
{
    cudaFree(net->weights);
    cudaFree(net->inters);
    cudaFree(net->inters_params);
    cudaFree(ags->states);
    cudaFree(ags->params);
    cudaFree(ags->routes);
}

#define PER_BLOCK 1024

void sim_gpu(const char* out_filename, const double t_step, const double t_final,
    const net_type *h_net, const agents_type *h_ags)
{
    net_type net;
    agents_type ags;
    copy_to_gpu(&net, &ags, h_net, h_ags);
    agent_state_type *states, *states_prev;
    agent_state_type *states_host = (agent_state_type*)malloc(h_ags->count * sizeof(*states_host));
    cudaMalloc(&states, h_ags->count * sizeof(*states));
    cudaMalloc(&states_prev, h_ags->count * sizeof(*states_prev));
    cudaMemcpy(states, ags.states, h_ags->count * sizeof(*states), cudaMemcpyDeviceToDevice);
    FILE *f = fopen(out_filename, "wb");

    int steps = (int)floor(t_final / t_step);
    fwrite(&t_step, sizeof(t_step), 1, f);
    fwrite(&steps, sizeof(steps), 1, f);
    fwrite(&h_ags->count, sizeof(h_ags->count), 1, f);
    for(int i = 0; i < steps; i++) {
        cudaMemcpy(states_host, states, h_ags->count * sizeof(*states), cudaMemcpyDeviceToHost);
        sort_agents(states_host, h_ags->count);
        fwrite(states_host, h_ags->count * sizeof(*states), 1, f);
        cudaMemcpy(states, states_host, h_ags->count * sizeof(*states), cudaMemcpyHostToDevice);
        cudaMemcpy(states_prev, states, h_ags->count * sizeof(*states), cudaMemcpyDeviceToDevice);
        agent_sim<<<1 + h_ags->count/PER_BLOCK, PER_BLOCK>>>(states, (double)i*t_step,
            t_step, net, states_prev, ags.params, ags.routes, h_ags->count);
        cudaDeviceSynchronize();
    }

    fclose(f);
    f = NULL;
    free(states_host);
    states_host = NULL;
    dealloc_gpu(&net, &ags);
    cudaFree(states);
    cudaFree(states_prev);
}