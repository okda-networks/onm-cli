CC = gcc
commands_src = src/cmds/commands.c src/cmds/yang_commands.c
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
