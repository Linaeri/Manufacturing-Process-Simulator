CC = clang
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic
RAYLIB_CFLAGS := $(shell pkg-config --cflags raylib)
RAYLIB_LIBS := $(shell pkg-config --libs raylib)

.PHONY: all run
all: main metal_stamping_simulator

main: main.c dashboard.c simulation.c dashboard.h simulation.h
	$(CC) $(CFLAGS) $(RAYLIB_CFLAGS) main.c dashboard.c simulation.c $(RAYLIB_LIBS) -o $@

metal_stamping_simulator: main
	cp main metal_stamping_simulator

run: all
	./main
