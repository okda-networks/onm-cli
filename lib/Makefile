install_lib: libcli_install

libcli_install:
	@echo "installing libcli..."
	cd lib/libcli && make && make install

#libyang_install:
#	@echo "Building libyang..."
#	git clone https://github.com/CESNET/libyang.git lib/libyang
#	cd lib/libyang && mkdir -p build && cd build && cmake .. && make && make install