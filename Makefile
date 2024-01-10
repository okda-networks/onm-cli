CC := gcc
CFLAGS :=
LIB_PATH := -L/usr/local/lib/
INCLUDE_PATH := -I$(CURDIR)
LIBS := -lyang -lsysrepo -lcrypt

# Directories
SRC_DIR := src
YCORE_DIR := $(SRC_DIR)/commands/yang_core
LIBCLI_DIR := lib/libcli

# libcli source files
LIBCLI_SRC := $(wildcard $(LIBCLI_DIR)/*.c)

# Source files
YCORE_SRC := $(wildcard $(YCORE_DIR)/*.c)
COMMANDS_SRC := $(SRC_DIR)/commands/default_cmd.c $(SRC_DIR)/commands/sysrepo_cmd.c $(YCORE_SRC) $(LIBCLI_SRC)
UTILS_SRC := $(SRC_DIR)/onm_main.c $(SRC_DIR)/onm_cli.c $(SRC_DIR)/onm_sysrepo.c $(SRC_DIR)/utils.c $(SRC_DIR)/onm_logger.c

# Object files
OBJ := $(COMMANDS_SRC:.c=.o) $(UTILS_SRC:.c=.o)

# Executable
EXEC := onmcli

.PHONY: all clean run debug

all: $(EXEC)

debug: CFLAGS += -g --vtv-debug
debug: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(INCLUDE_PATH) $(LIB_PATH) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDE_PATH)

run: all
	./$(EXEC)

clean:
	rm -f $(OBJ) $(EXEC) onmcli.log