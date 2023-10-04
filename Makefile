CC = gcc
src = src/onm_main.c src/commands.c src/cli.c src/onm_yang.c
LIB_PATH = -L/usr/local/lib/

main: $(src)
	$(CC) -g $(src) -o onm_cli $(LIB_PATH) -lcli -lyang  --vtv-debug

run: main
	LD_LIBRARY_PATH=/usr/local/lib/ ./vtyshd

clean:
	rm -f onm_cli

install: libcli_install libyang_install

libcli_install:
	@echo "installing libcli..."
	cd lib/libcli && make && make install

libcli_install:
	@echo "Building libyang..."
	cd lib/libyang && mkdir -p build && cd build && cmake .. && make && make install

