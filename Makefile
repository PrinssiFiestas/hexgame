# Targets
.PHONY: all       # Build the game
.PHONY: run       # Build and run the game
.PHONY: install   # Build and install the game
.PHONY: uninstall # Remove all files from system
.PHONY: clean     # Remove build artifacts, including the binary
.PHONY: debug     # Build with debug symbols and sanitizers

ifeq ($(OS), Windows_NT)
EXE_EXT = .exe
else
SANITIZERS = -fsanitize=address -fsanitize=undefined -fsanitize=leak
endif

all: hexgame$(EXE_EXT)
./hexgame$(EXE_EXT): ./hexgame.c
	cc -o $@ -Oz $< -lgpc

debug: hexgamed$(EXE_EXT)
./hexgamed$(EXE_EXT): ./hexgame.c
	cc -o $@ -ggdb3 -gdwarf -Wall -Wextra $< -lgpcd $(SANITIZERS)

run: all
	./hexgame$(EXE_EXT)

install: all
	cp ./hexgame$(EXE_EXT) /usr/local/bin/

uninstall:
	rm -rf /usr/local/bin/hexgame$(EXE_EXT) /etc/hexgame/

clean:
	rm -rf ./hexgame$(EXE_EXT) ./hexgamed$(EXE_EXT)
