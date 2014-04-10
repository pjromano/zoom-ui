# Philip Romano
# Makefile for statemachine experiment

SRCDIR = ../src
INCDIR = ../include
BINDIR = ../bin
TESTDIR = ../tests

CC = g++
CFLAGS = -I$(INCDIR) -g
LDFLAGS = -lSDL2 -lGL -lLeap

SOURCES = $(SRCDIR)/gesturestategraph.cpp $(SRCDIR)/gesturenode.cpp
HEADERS = $(INCDIR)/gesturestategraph.h $(INCDIR)/gesturenode.h

TESTS = static update simplegraph swipe

.PHONY: all dirs clean


all: dirs tests

dirs: $(BINDIR)/
$(BINDIR)/:
	mkdir -p $(BINDIR)


tests: $(foreach test,$(TESTS),$(BINDIR)/$(test).x)

define TEST_TEMPLATE
$$(BINDIR)/$(1).x: $$(TESTDIR)/$(1).cpp $$(SOURCES) $$(HEADERS)
	$$(CC) $$(CFLAGS) $$(TESTDIR)/$(1).cpp $$(SOURCES) $$(LDFLAGS) -o $$@
endef

$(foreach test,$(TESTS),$(eval $(call TEST_TEMPLATE,$(test))))


clean:
	rm -rf $(BINDIR)

