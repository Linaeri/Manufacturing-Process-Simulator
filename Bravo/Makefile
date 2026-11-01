CC = clang
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic
.PHONY: all run clean
all: metalworks
metalworks: main.c panel_state.c panel_state.h
	$(CC) $(CFLAGS) $(shell pkg-config --cflags raylib) main.c panel_state.c $(shell pkg-config --libs raylib) -o $@
run: metalworks
	./metalworks
clean:
	rm -f metalworks
