NumProcessors = `getconf NPROCESSORS_CONF`

.PHONY: all build configure clean test rebuild debug _debug

all: configure build

debug: _debug build


configure:
	cmake -S . -B build

build:
	cmake --build build -j $(NumProcessors)

test: clean _debug build
	ctest --test-dir build

clean:
	rm -rf build

rebuild: clean all

_debug:
	cmake -DDisableOPT=ON -S . -B build
