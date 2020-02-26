# tsim
This is a simple traffic microsimulation and visualization tool with CUDA support. The program can simulate movement of vechicles in a arbitrary network of two-way streets with simple signalled or always open intersections (individually set with possibly different parameters for each node). Agents in the network can follow arbitrary paths. Initial condition generator script currently allows generation of simple grid networks where agents follow shortest paths between two randomly selected loacations (uniform 2D distribution). Agents' and intersections' parameters are also randomized. A number of simulation parameters can be adjusted. Helper scripts for result visualization, comparison etc. are also provided. A video screencast of result visualization is seen below.

[![Video thumbnail](https://img.youtube.com/vi/KCtCjPpy82A/0.jpg)](https://www.youtube.com/watch?v=KCtCjPpy82A)

## Prerequisites
Python 3.x, NumPy, Matplotlib, `g++`, `make`, `nvcc` (only for CUDA support)

## Getting started
Please note that all executables and scripts in this project accept command-line parameters, so to get more information just use `[name] -h`. In order to perform a simple simulation from scratch:
1. Download or clone this repository and compile the simulation program:
   
   ```
   mkdir obj
   make no-cuda
   ```
   
   Note that other compilation targets (i.e. `all`, `debug` and `prof`) assume CUDA support and require `nvcc`.
2. Generate initial conditions for the simulation using `scripts/ic_gen.py`. Parameters are specified in a separate JSON file - refer to `config.json`, field names should be self-explainatory. You can specify output filenames and use graphical mode to get a preview of the network (see help message). To use default parameters, simply type:
   
   ```
   python3 scripts/ic_gen.py
   ```
   
   This will use the sample config and output `ic.bin` and `net.npz` files.
3. Run the simulation. You can specify a variety of variables here (see help message). To use defaults just type:
   
   ```
   ./tsim
   ```
   
4. Visualize the results by using `scripts/result_vis.py`. If you used defaults up to this point, simply type:
   
   ```
   python3 scripts/result_vis.py
   ```
   
   You'll be presented with a simple Matplotlib UI with "playback" controls.
   
   

## Code structure
* `src` - directory containing C++ source and header files for the actual simulation program
* `scripts` - directory containing various helper Python scripts and helper libraries
  - `ic_gen.py` - script for generating initial conditions for the simulation with optional visualization
  - `result_vis.py` - script for result visualization
  - `result_comp.py`- script for result comparison, outputs result summaries and further comparison between a pair of results (can be used to compare C++ and CUDA simulation outputs as well as results with different time step/number of samples but the same initial conditions)
  - `network.py` - network (graph) class
  - `_common.py` - miscallanious shared types and functions
* `config.json` - example configuration file for `scripts/ic_gen.py`
* `makefile.am`

## TODOs
* write/extend code documentation
* rewrite CPU-side C++ code to make use of C++ features (was originally developed in C)
* change graph representation to allow for larger networks (currently dense adjacency matrix)
* performance improvements (especially further parrarelization on the CUDA side)
* ...

## References
1. https://en.wikipedia.org/wiki/Intelligent_driver_model
