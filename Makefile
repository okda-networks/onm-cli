CC = gcc
ycore_dir = src/commands/yang_core
yang_core_src = $(ycore_dir)/y_utils.c $(ycore_dir)/cmd_list.c $(ycore_dir)/cmd_leaf.c $(ycore_dir)/cmd_container.c
commands_src = src/commands/default_cmd.c src/commands/yang_cmd_loader.c $(yang_core_src)
src = src/onm_main.c src/cli.c src/onm_yang.c src/utils.c $(commands_src)
LIB_PATH = -L/usr/local/lib/



main: $(src)
	$(CC) -g $(src) -o onm_cli $(LIB_PATH) -lcli -lyang  --vtv-debug

run: main
	LD_LIBRARY_PATH=/usr/local/lib/ ./onm_cli

static_link: $(src)
	$(CC) -g $(src) lib/libcli/libcli.c -o onm_cli -lcrypt -lyang  --vtv-debug

clean:
	rm -f onm_cli

include lib/Makefile
