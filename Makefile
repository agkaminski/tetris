tetris: tetris.c
	gcc "$<" -o "$@"

all: tetris

.PHONY: clean

clean:
	rm -f tetris