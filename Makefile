# Main program variables
FILENAME = irc-bot
OUTDIR   = bin
SRCDIR   = src
INCLDIR  = include
CFLAGS   = -Wall -Wextra -std=c99

# Needed for some network calls
CPPFLAGS = -D_POSIX_SOURCE

# Test & coverage variables
SRCDIR-TEST  = test
LDFLAGS-TEST = --coverage
LIBS-TEST    = -lcheck

############################# Do not edit below this line #############################

# Prepend output directory and add object extension on main program source files
SRCFILES := $(shell ls $(SRCDIR))
TMPFILES = $(SRCFILES:.c=.o)
OBJFILES = $(addprefix $(OUTDIR)/, $(TMPFILES))

# Do the same for the test program but don't link main.c or .check files
# Check framework provides it's own main() function.
SRCFILES-TEST := $(shell ls $(SRCDIR-TEST))
TMPFILES-TEST = $(SRCFILES-TEST:.c=.o)
OBJFILES-TEST = $(addprefix $(OUTDIR)/, $(TMPFILES-TEST))
OBJFILES-TEST += $(OBJFILES)
OBJFILES-TEST := $(filter-out %/main.o %.check %.txt, $(OBJFILES-TEST))

# If test rule is selected, build main source files with the extra flags as well
ifeq "$(MAKECMDGOALS)" "test"
	CFLAGS += -g --coverage
endif

# Disable assertions, enable gcc optimizations and strip binary for "release" rule
ifeq "$(MAKECMDGOALS)" "release"
	CPPFLAGS += -DNDEBUG
	CFLAGS += -O2
	LDFLAGS = -s
endif

# Build main program
$(OUTDIR)/$(FILENAME): $(OBJFILES)
	gcc $(LDFLAGS) $^ -o $@

# Generic rule to build all source files needed for main
$(OUTDIR)/%.o: $(SRCDIR)/%.c
	gcc -c $(CPPFLAGS) $(CFLAGS) -I$(INCLDIR) $< -o $@

# Run test program and produce coverage stats in html
test: $(OUTDIR)/$(FILENAME)-test
	./$<
	lcov --capture --directory $(OUTDIR)/ --output-file $(OUTDIR)/coverage.info >/dev/null
	genhtml $(OUTDIR)/coverage.info --output-directory $(OUTDIR)/lcov >/dev/null

# Build test program
$(OUTDIR)/$(FILENAME)-test: $(OBJFILES-TEST)
	gcc $(LDFLAGS-TEST) $^ -o $@ $(LIBS-TEST)

# Generic rule to build all source files needed for test
$(OUTDIR)/%.o: $(SRCDIR-TEST)/%.c
	gcc -c $(CFLAGS) -I$(INCLDIR) $< -o $@

# Generate .c files from the easier to write .check tests
$(SRCDIR-TEST)/%.c: $(SRCDIR-TEST)/%.check
	~/checkmk $< >$@

release: outdir $(OUTDIR)/$(FILENAME)

# Create output directory
outdir:
	mkdir -p $(OUTDIR)

clean:
	rm -r $(OUTDIR)/*
	echo bin folder cleaned

# Make sure any files in the project folder with a same name as the ones listed below, do not interfere with our rules
.PHONY: clean test release