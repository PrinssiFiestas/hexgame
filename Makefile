# -----------------------------------------------------------------------------
# Public Targets

.PHONY: all       # Build the game (default)
.PHONY: run       # Build and run the game
.PHONY: install   # Build and install the game (may require sudo)
.PHONY: uninstall # Remove all hexgame files for all users (may require sudo)

# -----------------------------------------------------------------------------
# Dev Targets

.PHONY: debug     # Build with debug symbols and sanitizers
.PHONY: clean     # Remove binaries from current directory

# -----------------------------------------------------------------------------

ifeq ($(OS), Windows_NT)
EXE_EXT = .exe
else
SANITIZERS = -fsanitize=address -fsanitize=undefined -fsanitize=leak
endif

all: hexgame$(EXE_EXT)
./hexgame$(EXE_EXT): ./hexgame.c
	cc -o $@ -Os $< -lgpc

debug: hexgamed$(EXE_EXT)
./hexgamed$(EXE_EXT): ./hexgame.c
	cc -o $@ -ggdb3 -gdwarf -Wall -Wextra $< -lgpcd $(SANITIZERS)

run: all
	./hexgame$(EXE_EXT)

install: all
	cp ./hexgame$(EXE_EXT) /usr/local/bin/

uninstall:
	rm -rf /usr/local/bin/hexgame$(EXE_EXT) /home/*/.hexgame

clean:
	rm -rf ./hexgame$(EXE_EXT) ./hexgamed$(EXE_EXT)
