#include "sim_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SQR(x) x*x

void ic_fromfile(const char* filename, net_type *net, agents_type *ags)
{
    FILE *f = fopen(filename, "rb");

    fread(&net->nodes_n, sizeof(net->nodes_n), 1, f);

    net->weights = malloc(net->nodes_n * sizeof(*net->weights));
    net->weights[0] = malloc(SQR(net->nodes_n) * sizeof(**net->weights));
    for(int i = 1; i < net->nodes_n; i++)
        net->weights[i] = net->weights[0] + net->nodes_n*i;
    fread(net->weights[0], sizeof(**net->weights), SQR(net->nodes_n), f);

    fread(&ags->count, sizeof(ags->count), 1, f);
    ags->states = malloc(ags->count * sizeof(*ags->states));
    fread(ags->states, sizeof(*ags->states), ags->count, f);
    
    ags->params = malloc(ags->count * sizeof(agent_params_type));
    fread(ags->params, sizeof(*ags->params), ags->count, f);
    
    int routes_len;
    fread(&routes_len, sizeof(routes_len), 1, f);
    ags->routes = malloc(routes_len * sizeof(*ags->routes));
    fread(ags->routes, sizeof(*ags->routes), routes_len, f);

    net->inters = malloc(net->nodes_n * sizeof(*net->inters));
    fread(net->inters, sizeof(*net->inters), net->nodes_n, f);

    int inters_params_len;
    fread(&inters_params_len, sizeof(inters_params_len), 1, f);
    net->inters_params = malloc(inters_params_len * sizeof(*net->inters_params));
    fread(net->inters_params, sizeof(*net->inters_params), inters_params_len, f);

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
    agents_type ags;

    ic_fromfile(in_filename, &net, &ags);
    sim_cpu(out_filename, t_step, t_final, &net, &ags);

    dealloc_agents(&ags);
    dealloc_net(&net);

    return 0;
}
