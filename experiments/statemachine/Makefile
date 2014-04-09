CC = g++
CFLAGS = -Iinclude -g -fno-inline
LDFLAGS = -lSDL2 -lLeap

SOURCES = src/gesturestategraph.cpp
HEADERS = include/gesturestategraph.h

.PHONY: all dirs clean


all: dirs tests

dirs: bin/
bin/:
	mkdir -p bin


TESTS = simple update

tests: $(foreach test,$(TESTS),bin/$(test).x)

define TEST_TEMPLATE
bin/$(1).x: tests/$(1).cpp $$(SOURCES) $$(HEADERS)
	$$(CC) $$(CFLAGS) tests/$(1).cpp $$(SOURCES) $$(LDFLAGS) -o $$@
endef

$(foreach test,$(TESTS),$(eval $(call TEST_TEMPLATE,$(test))))

#bin/simple.x: tests/simple.cpp $(SOURCES)
#	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@


clean:
	rm -rf bin

