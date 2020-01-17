#include "sim_gpu.h"

#include <stdio.h>

void copy_to_gpu(net_type *net, agents_type *ags,
    const net_type *h_net, const agents_type *h_ags) 
{
    cudaMalloc(&net->weights, h_net->nodes_n * sizeof(*net->weights));
    cudaMalloc(&net->inters, h_net->nodes_n * sizeof(*net->inters));
    cudaMalloc(&net->inters_params, h_net->inters_params_len * sizeof(*net->inters_params));
    cudaMalloc(&ags->states, h_ags->count * sizeof(*ags->states));
    cudaMalloc(&ags->params, h_ags->count * sizeof(*ags->params));
    cudaMalloc(&ags->routes, h_ags->routes_len * sizeof(*ags->routes));

    cudaMemcpy(net->weights, h_net->weights,
        h_net->nodes_n * sizeof(*net->weights), cudaMemcpyHostToDevice);
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

void sim_gpu(const char* out_filename, const double t_step, const double t_final,
    const net_type *h_net, const agents_type *h_ags)
{
    net_type net;
    agents_type ags;
    copy_to_gpu(&net, &ags, h_net, h_ags);
    agent_state_type *states, *states_prev;
    cudaMalloc(&states, h_ags->count * sizeof(*states));
    cudaMalloc(&states_prev, h_ags->count * sizeof(*states_prev));
    cudaMemcpy(states, ags.states, h_ags->count * sizeof(*states), cudaMemcpyDeviceToDevice);
    dealloc_gpu(&net, &ags);
    cudaFree(states);
    cudaFree(states_prev);
}