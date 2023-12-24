CC := gcc
CFLAGS := -g
LIB_PATH := -L/usr/local/lib/
INCLUDE_PATH := -I$(CURDIR)
LIBS := -lcli -lyang -lsysrepo

# Directories
SRC_DIR := src
YCORE_DIR := $(SRC_DIR)/commands/yang_core
LIB_DIR := lib

# Source files
YCORE_SRC := $(wildcard $(YCORE_DIR)/*.c)
COMMANDS_SRC := $(SRC_DIR)/commands/default_cmd.c $(SRC_DIR)/commands/sysrepo_cmd.c $(YCORE_SRC)
UTILS_SRC := $(SRC_DIR)/onm_main.c $(SRC_DIR)/onm_cli.c $(SRC_DIR)/onm_sysrepo.c $(SRC_DIR)/utils.c $(SRC_DIR)/onm_logger.c

# Object files
OBJ := $(COMMANDS_SRC:.c=.o) $(UTILS_SRC:.c=.o)

# Executable
EXEC := onmcli

.PHONY: all clean run

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(INCLUDE_PATH) $(LIB_PATH) $(LIBS) --vtv-debug

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDE_PATH)

run: all
	LD_LIBRARY_PATH=/usr/local/lib/ ./$(EXEC)

clean:
	rm -f $(OBJ) $(EXEC)

include $(LIB_DIR)/Makefile