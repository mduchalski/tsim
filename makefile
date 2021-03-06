CC=nvcc
LIBS=-lm
OBJ_DIR=obj
SRC_DIR=src
GPU_SRC=sim_gpu.cu
BIN_NAME=tsim

all: exec
no-cuda: CC=g++
no-cuda: GPU_SRC=sim_gpu_dummy.cpp
no-cuda: exec
debug: CCFLAGS=-g
debug: exec

exec: $(OBJ_DIR)/sim_cpu.o $(OBJ_DIR)/sim_gpu.o $(OBJ_DIR)/tsim.o $(OBJ_DIR)/types.o
	$(CC) $(CCFLAGS) $(OBJ_DIR)/sim_cpu.o $(OBJ_DIR)/sim_gpu.o $(OBJ_DIR)/tsim.o $(OBJ_DIR)/types.o -o $(BIN_NAME) $(LIBS)

$(OBJ_DIR)/sim_cpu.o: $(SRC_DIR)/sim_cpu.cpp $(SRC_DIR)/sim_cpu.h
	$(CC) $(CCFLAGS) $(SRC_DIR)/sim_cpu.cpp -o $(OBJ_DIR)/sim_cpu.o -c

$(OBJ_DIR)/sim_gpu.o: $(SRC_DIR)/$(GPU_SRC) $(SRC_DIR)/sim_gpu.h
	$(CC) $(CCFLAGS) $(SRC_DIR)/$(GPU_SRC) -o $(OBJ_DIR)/sim_gpu.o -c

$(OBJ_DIR)/tsim.o: $(SRC_DIR)/tsim.cpp $(SRC_DIR)/sim_cpu.h
	$(CC) $(CCFLAGS) $(SRC_DIR)/tsim.cpp -o $(OBJ_DIR)/tsim.o -c

$(OBJ_DIR)/types.o: $(SRC_DIR)/types.cpp $(SRC_DIR)/types.h
	$(CC) $(CCFLAGS) $(SRC_DIR)/types.cpp -o $(OBJ_DIR)/types.o -c

clean:
	rm -f $(OBJ_DIR)/*
	rm -f tsim
