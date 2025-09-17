# Make files are the bane of my existance
CC=g++
CCFLAGS=-g  -Wall -Wextra -Wpedantic # -fsanitize=undefined,address
CCSTD= -std=c++23
DEPFLAGS= -MP -MD
OPT= -O1

CCCMD=$(CC) $(CCFLAGS) $(OPT) $(DEPFLAGS) $(CCSTD)
PKG_CONFIG=pkg-config
PKG_LIST=ncurses 
PKG_CFLAGS = `$(PKG_CONFIG) --cflags $(PKG_LIST)`
PKG_LDLIBS = `$(PKG_CONFIG) --libs $(PKG_LIST)`

BIN=mem-anal
INCDIR=include src
SRCDIR=src
BLDDIR=build
INTERIMBLD=$(BLDDIR)/interrim
TESTDIR=tests
TESTBUILDDIR=$(BLDDIR)/tests

INCFILES=$(wildcard $(INCDIR)/*.hpp)
CFILES=$(foreach D, $(SRCDIR), $(wildcard $(D)/*.cpp))
TESTFILES=$(wildcard $(TESTDIR)/*.cpp)

OFILES=$(patsubst $(SRCDIR)/%.cpp, $(INTERIMBLD)/%.o, $(CFILES))
DFILES=$(patsubst $(SRCDIR)/%.cpp, $(INTERIMBLD)/%.d, $(CFILES))
TESTBUILDFILES=$(patsubst $(TESTDIR)/%.cpp, $(TESTBUILDDIR)/%, $(TESTFILES))
TESTDFILES=$(patsubst $(TESTDIR)/%.cpp, $(TESTBUILDDIR)/%.d, $(TESTFILES))


INCFLAGS=$(foreach D, $(INCDIR), -I$(D))

main: $(BLDDIR) $(BIN) $(TESTBUILDFILES)

$(BIN): $(OFILES)
	@echo -e "----- Linking '$<' into '$@' -----"
	$(CCCMD) $(INCFLAGS) $(PKG_LDLIBS) -o $@ $^
	@echo

$(INTERIMBLD)/%.o: $(SRCDIR)/%.cpp | $(INTERIMBLD)
	@echo -e "----- Compiling '$<' into '$@' -----"
	$(CCCMD) $(INCFLAGS) $(PKG_CFLAGS) -c -o $@ $<
	@echo

$(INTERIMBLD):
	@mkdir -p $(INTERIMBLD)

tests: $(TESTBUILDFILES)

run_tests: $(filter-out %sampleProcess, $(TESTBUILDFILES)) | $(TESTBUILDFILES)
	@echo "Running all tests..." && \
	echo "Initiating the sample process" && \
	./build/tests/sampleProcess & \
	pid=$$(pidof ./build/tests/sampleProcess) ; \
	echo "Started sampleProcess with PID: $$pid" ; \
	export SAMPLE_PROCESS_PID="$$pid" ; \
	echo "SAMPLE_PROCESS_PID is set to: $$SAMPLE_PROCESS_PID" ; \
	for test in $^; do \
		echo "--- Running $$test ---"; \
		./$$test; \
		echo "--- Finished $$test ---"; \
		echo ""; \
	done && \
	echo "Killing $$pid" && \
	kill $$pid && \
	echo "Unsetting SAMPLE_PROCESS_PID" && \
	export -n SAMPLE_PROCESS_PID && \
	echo "All tests finished."

$(TESTBUILDDIR)/%: $(TESTDIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo -e "----- Compiling test '$<' into '$@' -----"
	$(CCCMD) $(INCFLAGS) $(PKG_LDLIBS) $(PKG_CFLAGS) $< -o $@
	@echo

files:
	@echo -e "INCFILES: \n\t" 			$(INCFILES) "\n"
	@echo -e "CFILES: \n\t" 				$(CFILES)"\n"
	@echo -e "OFILES: \n\t" 				$(OFILES)"\n"
	@echo -e "DFILES: \n\t" 				$(DFILES)"\n"
	@echo -e "TESTFILES: \n\t" 			$(TESTFILES)"\n"
	@echo -e "TESTBUILDFILES: \n\t" $(TESTBUILDFILES)"\n"
	@echo -e "INCFLAGS: \n\t" 			$(INCFLAGS)"\n"

clean:
	@rm -rf $(BLDDIR)

$(BLDDIR):
	@mkdir -p $(BLDDIR) $(TESTBUILDDIR)

-include $(DFILES) $(TESTDFILES)
