CC = clang
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic
RAYLIB_CFLAGS := $(shell pkg-config --cflags raylib)
RAYLIB_LIBS := $(shell pkg-config --libs raylib)

metal_stamping_simulator: main.c dashboard.c simulation.c dashboard.h simulation.h
	$(CC) $(CFLAGS) $(RAYLIB_CFLAGS) main.c dashboard.c simulation.c $(RAYLIB_LIBS) -o $@

.PHONY: run
run: metal_stamping_simulator
	./metal_stamping_simulator
