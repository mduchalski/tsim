#include "sim_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SQR(x) x*x

void ic_fromfile(const char* filename, net_type *net, agent_type **ags, int *ags_n)
{
    FILE *f = fopen(filename, "rb");

    fread(&net->nodes_n, sizeof(int), 1, f);

    net->weights = malloc(net->nodes_n * sizeof(*net->weights));
    net->weights[0] = malloc(SQR(net->nodes_n) * sizeof(**net->weights));
    for(int i = 1; i < net->nodes_n; i++)
        net->weights[i] = net->weights[0] + net->nodes_n*i;
    fread(net->weights[0], sizeof(**net->weights), SQR(net->nodes_n), f);

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

    net->inters = malloc(net->nodes_n * sizeof(*net->inters));
    fread(net->inters, sizeof(*net->inters), net->nodes_n, f);
    int params_bytes;
    for(int i = 0; i < net->nodes_n; i++) {
        switch(net->inters[i].type_id) {
            case SIMPLE:
                params_bytes = 2;
                break;
            default:
                params_bytes = 0;
                break;
        }
        params_bytes *= sizeof(net->inters[0].params[0]);
        net->inters[i].params = malloc(params_bytes);
        fread(net->inters[i].params, params_bytes, 1, f);
    }

    fclose(f);
    f = NULL;
}

int main(int argc, char *argv[])
{
    char *in_filename = "ic.bin", *out_filename = "res.bin";
    double t_step = 0.1, t_final = 10.0;
    char opt;
    while((opt = getopt(argc, argv, "i:o:s:f:h")) != -1) {
        switch(opt) {
            case 's':
                t_step = atof(optarg);
                break;
            case 'f':
                t_final = atof(optarg);
                break;
            case 'i':
                in_filename = strdup(optarg);
                break;
            case 'o':
                out_filename = strdup(optarg); // offset
                break;
            case 'h':
                printf("Usage: %s [-h] [-i IN] [-o OUT] [-s STEP] [-f FINAL]\n\n"
                    "Optional parameters:\n"
                    "-h:        displays this message\n"
                    "-i IN:     initial conditions filename (default: ic.bin)\n"
                    "-o OUT:    simulation output filename (default: res.bin)\n"
                    "-s STEP:   simulation step in seconds (default: 0.1)\n"
                    "-f FINAL:  simulation stop time in seconds (default: 10.0)\n", argv[0]);
                return 0;
        }
    }

    net_type net;
    int ags_n;
    agent_type* ags_ic;

    ic_fromfile(in_filename, &net, &ags_ic, &ags_n);
    sim_cpu(out_filename, t_step, t_final, &net, ags_ic, ags_n);
    
    //ic_fromfile(in_filename, &weights, &ags_ic, &inters_types, &nodes_n, &ags_n);
    //sim_cpu(t_step, t_final, ags_ic, weights, inters_types, nodes_n, ags_n, out_filename);

    dealloc_agents(ags_ic, ags_n);
    dealloc_net(&net);

    return 0;
}
