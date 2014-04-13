SRCDIR = ..
BINDIR = ../bin/linux
LDFLAGS = -lSDL2 -lSDL2main -lGL -lLeap


.PHONY: all dirs debug clean

all: dirs debug


dirs: $(BINDIR)/debug/ $(BINDIR)/release/
$(BINDIR)/debug/:
	mkdir -p $(BINDIR)/debug
$(BINDIR)/release/:
	mkdir -p $(BINDIR)/release


debug: $(BINDIR)/debug/swipe.x

$(BINDIR)/debug/swipe.x: $(SRCDIR)/main.cpp
	g++ $^ $(LDFLAGS) -o $@


clean:
	rm -rf $(BINDIR)

