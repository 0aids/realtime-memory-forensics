# Make files are the bane of my existance
CXX=g++
ASAN = -fsanitize=undefined,address
# Building with asan causes tests to fail due to asan memory limits.
# Maybe i'm doing something wrong.
# It seems to move the shit around.
CXXFLAGS=-g  -Wall -Wextra -Wpedantic # $(ASAN)
CCSTD= -std=c++23
DEPFLAGS= -MP -MD
OPT= -O1

PKGS = ncurses

INCLUDES = -Iinclude `pkg-config --cflags $(PKGS)`
LIBFLAGS = `pkg-config --libs $(PKGS)`

CPPFILES = $(shell find src -type f -name "*.cpp" -not -path '*/.*' )
OFILES = $(patsubst src/%.cpp, build/src/%.o, $(CPPFILES))

# maybe I should learn cmake. or meson, or an actually good build system.
# I love make it's so easyyyyy :) :):)(:
OFILES_FOR_TEST = $(filter-out %main.o, $(OFILES))
TESTFILES = $(shell find tests -type f -name "*.cpp" -not -path '*/.*')
TESTOFILES = $(patsubst tests/%.cpp, build/tests/%.o, $(TESTFILES))
TESTDFILES  = $(patsubst tests/%.cpp, build/tests/%.d, $(TESTFILES))
TESTEXEFILES = $(patsubst tests/%.cpp, build/tests/%, $(TESTFILES))
HFILES = $(shell find include -type f -name "*.hpp" -not -path '*/.*' )
DFILES = $(patsubst %.hpp, build/%.d, $(HFILES)) $(TESTDFILES)

files:
	@echo "CPPFILES: $(CPPFILES)"
	@echo "OFILES: $(OFILES)"
	@echo "TESTFILES: $(TESTFILES)"
	@echo "TESTEXEFILES: $(TESTEXEFILES)"
	@echo "HFILES: $(HFILES)"
	@echo "DFILES: $(DFILES)"
	@echo "OFILES_FOR_TEST: $(OFILES_FOR_TEST)"

mem-anal: $(OFILES)
	$(CXX) $(INCLUDES) $(OPT) $(DEPFLAGS) $(CCSTD) $(LIBFLAGS) $(CXXFLAGS) -o $@ $^

tests:    $(TESTEXEFILES) $(OFILES_FOR_TEST)


build/tests/% : build/tests/%.o $(OFILES_FOR_TEST)
	@mkdir -p build/tests
	$(CXX) $(INCLUDES) $(OPT) $(DEPFLAGS) $(CCSTD) $(LIBFLAGS) $(CXXFLAGS) -o $@ $^

build/tests/%.o: tests/%.cpp
	@mkdir -p build/tests
	$(CXX) $(INCLUDES) $(OPT) $(DEPFLAGS) $(CCSTD) $(LIBFLAGS) $(CXXFLAGS) -c -o $@ $<


build/src/%.o : src/%.cpp
	@mkdir -p build/`dirname $<`
	$(CXX) $(INCLUDES) $(OPT) $(DEPFLAGS) $(CCSTD) $(CXXFLAGS) -c -o $@ $< 
	

clean:
	rm -rf build

-include $(DFILES)
