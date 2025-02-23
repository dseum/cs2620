all: build
	@.build/exe serve

test: build
	@.build/exe_test

build:
	@if [ ! -d ".build" ]; then $(MAKE) install; fi
	@cmake --build .build

install: clean
	@cmake --preset=vcpkg

clean:
	@rm -rf .build

.PHONY: all test build install clean
