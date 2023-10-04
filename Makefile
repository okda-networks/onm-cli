CC = gcc
src = src/onm_main.c src/commands.c src/cli.c src/onm_yang.c
LIB_PATH = -L/usr/local/lib/

include lib/Makefile

main: $(src)
	$(CC) -g $(src) -o onm_cli $(LIB_PATH) -lcli -lyang  --vtv-debug

run: main
	LD_LIBRARY_PATH=/usr/local/lib/ ./onm_cli

clean:
	rm -f onm_cli


