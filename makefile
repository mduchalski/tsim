CC=nvcc
LIBS=-lm
OBJ_DIR=obj
SRC_DIR=src
BIN_NAME=tsim

all: exec
debug: CCFLAGS=-g
debug: exec
profile: CCFLAGS=-pg
profile: exec

exec: $(OBJ_DIR)/sim.o $(OBJ_DIR)/sim_cpu.o $(OBJ_DIR)/sim_gpu.o $(OBJ_DIR)/tsim.o $(OBJ_DIR)/types.o
	$(CC) $(CCFLAGS) $(OBJ_DIR)/sim.o $(OBJ_DIR)/sim_cpu.o $(OBJ_DIR)/sim_gpu.o $(OBJ_DIR)/tsim.o $(OBJ_DIR)/types.o -o $(BIN_NAME) $(LIBS)

$(OBJ_DIR)/sim.o: $(SRC_DIR)/sim.c $(SRC_DIR)/sim.h $(SRC_DIR)/sim_gpu.h
	$(CC) $(CCFLAGS) $(SRC_DIR)/sim.c -o $(OBJ_DIR)/sim.o -c

$(OBJ_DIR)/sim_cpu.o: $(SRC_DIR)/sim_cpu.c $(SRC_DIR)/sim_cpu.h
	$(CC) $(CCFLAGS) $(SRC_DIR)/sim_cpu.c -o $(OBJ_DIR)/sim_cpu.o -c

$(OBJ_DIR)/sim_gpu.o: $(SRC_DIR)/sim_gpu.cu $(SRC_DIR)/sim_gpu.h
	$(CC) $(CCFLAGS) $(SRC_DIR)/sim_gpu.cu -o $(OBJ_DIR)/sim_gpu.o -c

$(OBJ_DIR)/tsim.o: $(SRC_DIR)/tsim.cu $(SRC_DIR)/sim_cpu.h
	$(CC) $(CCFLAGS) $(SRC_DIR)/tsim.cu -o $(OBJ_DIR)/tsim.o -c

$(OBJ_DIR)/types.o: $(SRC_DIR)/types.c $(SRC_DIR)/types.h
	$(CC) $(CCFLAGS) $(SRC_DIR)/types.c -o $(OBJ_DIR)/types.o -c

clean:
	rm -f $(OBJ_DIR)/*
	rm -f tsim
