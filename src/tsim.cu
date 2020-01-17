#include "types.h"
#include "sim_cpu.h"
#include "sim_gpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SQR(x) x*x

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
    sim_gpu(out_filename, t_step, t_final, &net, &ags);

    dealloc_agents(&ags);
    dealloc_net(&net);

    return 0;
}
