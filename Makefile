# Main program variables
PROGRAM  = irc-bot
VERSION  := $(shell echo "r$$(git describe --long | sed 's/-/.r/;s/-/./')")
CC       = gcc
OUTDIR   = bin
INCLDIR  = include
SRCDIR   = src
TESTDIR  = test
CFLAGS   = -ggdb3 -Wall -Wextra -std=c99 -pedantic
LDLIBS   = -pthread -lcurl -lcrypto -lyajl -lsqlite3
CPPFLAGS = -D_GNU_SOURCE -DVERSION='"$(VERSION)"'
CFLAGS-TEST := $(CFLAGS)

# Disable assertions, enable compiler optimizations and strip binary for "release" rule
ifeq "$(MAKECMDGOALS)" "release"
	CPPFLAGS += -DNDEBUG
	CFLAGS   += -O2 -pipe
	CFLAGS   := $(filter-out -ggdb3, $(CFLAGS))
	LDFLAGS  = -s
endif

# If test rule is selected, add debugging symbols and test unit coverage
ifeq "$(MAKECMDGOALS)" "test"
	CFLAGS   += --coverage
	CPPFLAGS += -DTEST
	LDFLAGS   = --coverage
	LDLIBS   += -lcheck
endif

# Prepend output directory and add object extension on main program source files
SRCFILES := $(shell ls $(SRCDIR))
OBJFILES := $(SRCFILES:.c=.o)
OBJFILES := $(addprefix $(OUTDIR)/, $(OBJFILES))

SRCFILES-TEST := $(shell ls $(TESTDIR))
OBJFILES-TEST := $(SRCFILES-TEST:.c=.o)
OBJFILES-TEST := $(addprefix $(OUTDIR)/, $(OBJFILES-TEST))
OBJFILES-TEST += $(OBJFILES)
OBJFILES-TEST := $(filter-out %/main.o %/test_main.h, $(OBJFILES-TEST))

all: $(OUTDIR)/$(PROGRAM)
release: all

# Build main program
$(OUTDIR)/$(PROGRAM): $(OBJFILES)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

# Build a lookup table to quickly match strings -> bot commands
$(SRCDIR)/gperf.c: $(INCLDIR)/gperf.txt
	gperf $< >$@

# Generic rule to build all source files needed for main
$(OUTDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -I$(INCLDIR) -c $< -o $@

# Run tests
test: $(OUTDIR)/$(PROGRAM)-test
	./$<

# Build test program
$(OUTDIR)/$(PROGRAM)-test: $(OBJFILES-TEST)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

# Generic rule to build all source files needed for test
$(OUTDIR)/%.o: $(TESTDIR)/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS-TEST) -I$(INCLDIR) -c $< -o $@

# Generate documentation & test coverage in html and upload them
doc:
	doxygen Doxyfile
	rsync -avz --delete bin/doxygen/ freestyler@foss.teiwest.gr:public_html/irc-bot/doc
	lcov --capture --directory $(OUTDIR)/ --output-file $(OUTDIR)/coverage.info >/dev/null
	genhtml $(OUTDIR)/coverage.info --output-directory $(OUTDIR)/lcov >/dev/null
	rsync -avz --delete bin/lcov/ freestyler@foss.teiwest.gr:public_html/irc-bot/coverage

clean:
	rm -rf $(OUTDIR)/*

# Make sure any files in the project folder with a same name as the ones listed below, do not interfere with our rules
.PHONY: clean test release
