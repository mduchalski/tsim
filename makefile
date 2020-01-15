CC=gcc
LIBS=-lm
OBJ_DIR=obj
SRC_DIR=src

all: $(OBJ_DIR)/sim.o $(OBJ_DIR)/sim_cpu.o $(OBJ_DIR)/tsim.o $(OBJ_DIR)/types.o
	$(CC) $(OBJ_DIR)/sim.o $(OBJ_DIR)/sim_cpu.o $(OBJ_DIR)/tsim.o $(OBJ_DIR)/types.o -o tsim $(LIBS)

$(OBJ_DIR)/sim.o: $(SRC_DIR)/sim.c $(SRC_DIR)/sim.h $(SRC_DIR)/sim_cpu.h
	$(CC) $(SRC_DIR)/sim.c -o $(OBJ_DIR)/sim.o -c

$(OBJ_DIR)/sim_cpu.o: $(SRC_DIR)/sim_cpu.c $(SRC_DIR)/sim_cpu.h
	$(CC) $(SRC_DIR)/sim_cpu.c -o $(OBJ_DIR)/sim_cpu.o -c

$(OBJ_DIR)/tsim.o: $(SRC_DIR)/tsim.c $(SRC_DIR)/sim_cpu.h
	$(CC) $(SRC_DIR)/tsim.c -o $(OBJ_DIR)/tsim.o -c

$(OBJ_DIR)/types.o: $(SRC_DIR)/types.c $(SRC_DIR)/types.h
	$(CC) $(SRC_DIR)/types.c -o $(OBJ_DIR)/types.o -c

clean:
	rm $(OBJ_DIR)/*
