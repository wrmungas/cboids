# important directories and names
OBJ_DIR = build
INC_DIR = include
SRC_DIR = src
BIN_DIR = bin

# all source files
SRCS = $(wildcard $(SRC_DIR)/*.c)
# all objects
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# the target executable
EXE_NAME = cboids
EXE = $(BIN_DIR)/$(EXE_NAME)

# compiler variables
CC = gcc
CFLAGS = -std=c99 -Wall -g -I$(INC_DIR)
LDFLAGS = -lSDL2 -lGL -lm

.PHONY:
all: $(EXE)

# compile all
# for some reason this doesn't work if I don't explicitly type the patterns
build/%.o: src/%.c
	$(CC) -c $(CFLAGS) $^ -o $@

$(EXE): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

.PHONY:
test:
	echo $(OBJS)

.PHONY:
clean:
	rm $(OBJ_DIR)/*.o
	rm $(EXE)
