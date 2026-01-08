NumProcessors = `getconf NPROCESSORS_CONF`

.PHONY: all build configure clean test rebuild debug _debug

builder = # -G Ninja -DCMAKE_MAKE_PROGRAM=`which ninja`

all: configure build

debug: _debug build


configure:
	cmake -B build -S . $(builder) -DBuildTests=OFF

build:
	cmake --build build -j $(NumProcessors)

test:
	cmake $(builder) -DDisableOPT=ON -DBuildTests=ON -S . -B build
	cmake --build build -j $(NumProcessors)
	ctest --test-dir build

clean:
	rm -rf build

rebuild: clean all

_debug:
	cmake $(builder) -DDisableOPT=ON -S . -B build
