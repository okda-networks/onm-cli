CC = gcc
ycore_dir = src/commands/yang_core
yang_core_src = $(ycore_dir)/y_utils.c \
	$(ycore_dir)/cmd_list.c $(ycore_dir)/cmd_leaf.c\
	$(ycore_dir)/cmd_container.c $(ycore_dir)/cmd_choice.c\
	$(ycore_dir)/data_validators.c $(ycore_dir)/y_cmd_generator.c\
	$(ycore_dir)/data_factory.c

commands_src = src/commands/default_cmd.c src/commands/yang_loader_cmd.c $(yang_core_src)
src = src/onm_main.c src/onm_cli.c src/onm_yang.c src/onm_sysrepo.c src/utils.c $(commands_src)
LIB_PATH = -L/usr/local/lib/
# Get the current directory
CURRENT_DIR := $(CURDIR)

# Set the include path to the current directory
INCLUDE_PATH := -I$(CURRENT_DIR)




main: $(src)
	$(CC) -g $(src) -o onmcli $(INCLUDE_PATH) $(LIB_PATH) -lcli -lyang -lsysrepo  --vtv-debug

run: main
	LD_LIBRARY_PATH=/usr/local/lib/ ./onm_cli

static_link: $(src)
	$(CC) -g $(src) lib/libcli/libcli.c -o onm_cli -lcrypt -lyang  -lsysrepo --vtv-debug

clean:
	rm -f onmcli

include lib/Makefile
