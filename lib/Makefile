install: libyang_install libcli_install

libcli_install:
	@echo "installing libcli..."
	git clone https://github.com/dparrish/libcli.git lib/libcli
	cd lib/libcli && make && make install
	cp /usr/local/lib/libcli.so.* /usr/lib/
	cp /usr/local/lib/libcli.a /usr/lib/

libyang_install:
	@echo "Building libyang..."
	git clone https://github.com/CESNET/libyang.git lib/libyang
	cd lib/libyang && mkdir -p build && cd build && cmake .. && make && make install