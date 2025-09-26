all:
	cc -o hexgame -ggdb3 -gdwarf -Wall -Wextra hexgame.c -lgpcd -fsanitize=address -fsanitize=undefined

run: all
	./hexgame
