dyde-all: dyde-build
	@.build/exe serve

dyde-test: dyde-build
	@.build/exe_test

dyde-build:
	@if [ ! -d ".build" ]; then $(MAKE) install; fi
	@cmake -DPROTOCOL=dyde -B .build -S .
	@cmake --build .build

json-all: json-build
	@.build/exe serve

json-test: json-build
	@.build/exe_test

json-build:
	@if [ ! -d ".build" ]; then $(MAKE) install; fi
	@cmake -DPROTOCOL=json -B .build -S .
	@cmake --build .build

install: clean
	@cmake --preset=vcpkg

clean:
	@rm -rf .build

.PHONY: dyde-all dyde-test dyde-build json-all json-test json-build install clean
