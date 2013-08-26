# Main program variables
PROGRAM  = irc-bot
OUTDIR   = bin
SRCDIR   = src
INCLDIR  = include
TESTDIR  = test
CC       = gcc
CFLAGS   = -g -Wall -Wextra -std=gnu99 -pedantic
LDLIBS   = -lcurl -lyajl
CPPFLAGS = -D_GNU_SOURCE
CFLAGS-test := $(CFLAGS)

# Disable assertions, enable compiler optimizations and strip binary for "release" rule
ifeq "$(MAKECMDGOALS)" "release"
	CPPFLAGS += -DNDEBUG
	CFLAGS   += -march=native -O2 -pipe
	CFLAGS   := $(filter-out -g, $(CFLAGS))
	LDFLAGS  = -s
endif

# If test rule is selected, add debugging symbols and test unit coverage
ifeq "$(MAKECMDGOALS)" "test"
	CFLAGS   += --coverage
	CPPFLAGS += -DTEST
	LDFLAGS   = --coverage
	LDLIBS   += -lcheck
endif

############################# Do not edit below this line #############################

# Prepend output directory and add object extension on main program source files
SRCFILES := $(shell ls $(SRCDIR))
TMPFILES  = $(SRCFILES:.c=.o)
OBJFILES  = $(addprefix $(OUTDIR)/, $(TMPFILES))

# Do the same for the test program but don't link main.c or .check files
# Check framework provides it's own main() function.
SRCFILES-TEST := $(shell ls $(TESTDIR))
TMPFILES-TEST  = $(SRCFILES-TEST:.c=.o)
OBJFILES-TEST  = $(addprefix $(OUTDIR)/, $(TMPFILES-TEST))
OBJFILES-TEST += $(OBJFILES)
OBJFILES-TEST := $(filter-out %/main.o %.check, $(OBJFILES-TEST))

all: $(OUTDIR)/$(PROGRAM) $(OUTDIR)/json_value

# Build main program
$(OUTDIR)/$(PROGRAM): $(OBJFILES)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

# Build a lookup table to quickly match string commands -> function pointers
$(SRCDIR)/gperf.c: $(INCLDIR)/gperf-input.txt
	gperf $< >$@

# Generic rule to build all source files needed for main
$(OUTDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -I$(INCLDIR) -c $< -o $@

$(OUTDIR)/json_value: scripts/json_value.c
	$(CC) $(LDFLAGS) $(CFLAGS) $< -o $@ -lyajl

# Run test program and produce coverage stats in html
test: $(OUTDIR)/$(PROGRAM)-test
	./$<
	# lcov --capture --directory $(OUTDIR)/ --output-file $(OUTDIR)/coverage.info >/dev/null
	# genhtml $(OUTDIR)/coverage.info --output-directory $(OUTDIR)/lcov >/dev/null

# Build test program
$(OUTDIR)/$(PROGRAM)-test: $(OBJFILES-TEST)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

# Generic rule to build all source files needed for test
$(OUTDIR)/%.o: $(TESTDIR)/%.c
	$(CC) $(CFLAGS-test) -I$(INCLDIR) -c $< -o $@

# Generate .c files from the easier to write .check tests
$(TESTDIR)/%.c: $(TESTDIR)/%.check
	~/checkmk $< >$@

release: outdir $(OUTDIR)/$(PROGRAM) $(OUTDIR)/json_value

# Create output directory
outdir:
	mkdir -p $(OUTDIR)

doc:
	doxygen Doxyfile
	rsync -avz --delete bin/doxygen/ freestyler@foss.tesyd.teimes.gr:public_html/irc-bot

clean:
	rm -r $(OUTDIR)/*

# Make sure any files in the project folder with a same name as the ones listed below, do not interfere with our rules
.PHONY: clean test release
