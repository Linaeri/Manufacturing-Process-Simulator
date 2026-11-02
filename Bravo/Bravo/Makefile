CC = clang
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic
SRC = main.c panel_state.c factory_view.c view3d.c sawmill_state.c sawmill_panel.c sawmill_view.c ui.c
HDR = panel_state.h factory_view.h view3d.h sawmill_state.h sawmill_panel.h sawmill_view.h ui.h
.PHONY: all run balance clean
all: metalworks
metalworks: $(SRC) $(HDR)
	$(CC) $(CFLAGS) $(shell pkg-config --cflags raylib) $(SRC) $(shell pkg-config --libs raylib) -o $@
run: metalworks
	./metalworks
# 그래픽 없이 밸런스를 검사합니다. / Balance check without graphics.
balance: balance_test.c panel_state.c sawmill_state.c panel_state.h sawmill_state.h
	$(CC) $(CFLAGS) -O2 balance_test.c panel_state.c sawmill_state.c -lm -o balance_test
	./balance_test 4 metal
	./balance_test 4 saw
clean:
	rm -f metalworks balance_test
