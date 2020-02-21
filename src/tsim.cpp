#include "types.h"
#include "sim_cpu.h"
#include "sim_gpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef enum { CPU, CUDA } backend_type;

int main(int argc, char *argv[])
{
    char *in_filename = "ic.bin", *out_filename = "res.bin";
    double t_step = 0.1, t_final = 10.0;
    backend_type backend = CPU;
    bool verbose = false;
    char opt;
    while((opt = getopt(argc, argv, "hvb:i:o:s:f:")) != -1) {
        switch(opt) {
            case 'b':
                if(strcmp(optarg, "cuda") == 0)
                    backend = CUDA;
                break;
            case 'v':
                verbose = true;
                break;
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
                out_filename = strdup(optarg);
                break;
            case 'h':
                printf("Usage: %s [-h] [-v] [-b BACKEND] [-i IN] [-o OUT] [-s STEP] [-f FINAL]\n\n"
                    "Optional parameters:\n"
                    "-h:         displays this message\n"
                    "-v:         verbose mode"
                    "-b BACKEND: simulation backend (default: cpu)\n"
                    "-i IN:      initial conditions filename (default: ic.bin)\n"
                    "-o OUT:     simulation output filename (default: res.bin)\n"
                    "-s STEP:    simulation step in seconds (default: 0.1)\n"
                    "-f FINAL:   simulation stop time in seconds (default: 10.0)\n", argv[0]);
                return 0;
        }
    }

    net_type net;
    agents_type ags;

    ic_fromfile(in_filename, &net, &ags);

    switch(backend) {
        case CUDA:
            if(verbose)
                printf("Using CUDA backend\n");
            sim_gpu(out_filename, t_step, t_final, &net, &ags);
            break;
        default: // CPU
            if(verbose)
                printf("Using CPU backend\n");
            sim_cpu(out_filename, t_step, t_final, &net, &ags);
            break;
    }

    dealloc_agents(&ags);
    dealloc_net(&net);

    return 0;
}
