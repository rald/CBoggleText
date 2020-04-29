game: game.c
	gcc game.c -o game -lm -lncurses -Wall

.PHONY: clean

clean:
	rm game
