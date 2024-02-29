# SPDX-License-Identifier: AGPL-3.0-or-later
# Authors:     Ali Aqrabawi, <aaqrabaw@okdanetworks.com>
#
#              This program is free software; you can redistribute it and/or
#              modify it under the terms of the GNU Affero General Public
#              License Version 3.0 as published by the Free Software Foundation;
#              either version 3.0 of the License, or (at your option) any later
#              version.
#
# Copyright (C) 2024 Okda Networks, <aaqrabaw@okdanetworks.com>
#

CC ?= gcc
CFLAGS := -Wall
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
	rm -f $(OBJ) $(EXEC) onmcli.log valgrind-out.txt
