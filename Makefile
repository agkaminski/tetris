tetris: tetris.c
	gcc "$<" -o "$@"
	
.PHONY: clean

clean:
	rm -f tetris