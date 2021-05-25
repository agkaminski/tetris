/*
Purely tty tetris

Author: Aleksander Kaminski

Copyright 2021
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

#define FIELD_XSZ     10
#define FIELD_YSZ     24
#define TIME_STEP     1000
#define TICKS_TO_STEP 1000

typedef struct {
	char x;
	char y;
	char matrix[4][4];
} tetromino_t;


typedef struct {
	char field[FIELD_YSZ][FIELD_XSZ];
	tetromino_t curr;
	int score;
} tetris_t;


typedef enum { keyLeft, keyRight, keyRot, keyPause, keyNone } keySpec_t;


static void newGame(tetris_t *tetris)
{
	memset(tetris->field, 0, sizeof(tetris->field));
	tetris->score = 0;
}


static int eatFullLines(tetris_t *tetris)
{
	int x, y, i, ret = 0;
	
	for (y = 0; y < FIELD_YSZ; ) {
		for (x = 0; x < FIELD_XSZ; ++x)
			if (!tetris->field[y][x])
				break;

		if (x == FIELD_XSZ) {
			++ret;

			for (i = y; i < FIELD_YSZ - 1; ++i)
				memcpy(&tetris->field[i], &tetris->field[i + 1], sizeof(tetris->field[i]));

			memset(&tetris->field[i], 0, sizeof(tetris->field[i]));
		}
		else {
			++y;
		}
	}

	return ret;
}


static void removeTetromino(tetris_t *tetris)
{
	int i, j;
	
	for (i = 0; i < 4; ++i)
		for (j = 0; j < 4; ++j)
			if (tetris->curr.matrix[j][i])
				tetris->field[j + tetris->curr.y][i + tetris->curr.x] = 0;
}


static int _addTetromino(tetris_t *tetris, int dry)
{
	int i, j;
	
	for (i = 0; i < 4; ++i) {
		for (j = 0; j < 4; ++j) {
			if (tetris->curr.matrix[j][i]) {
				if (tetris->field[j + tetris->curr.y][i + tetris->curr.x])
					return -1;
				
				if (!dry)
					tetris->field[j + tetris->curr.y][i + tetris->curr.x] = 1;
			}
		}
	}
	
	return 0;
}


static int addTetromino(tetris_t *tetris)
{
	if (_addTetromino(tetris, 1))
		return -1;
	
	return _addTetromino(tetris, 0);
}


static int spawnTetromino(tetris_t *tetris)
{
	int i;
	char *point;
	static const tetromino_t spawns[] = {
		{ 4, 23, { { 0, 1, 0, 0 },
		           { 0, 1, 0, 0 },
		           { 0, 1, 0, 0 },
		           { 0, 1, 0, 0 } } },

		{ 4, 23, { { 0, 0, 0, 0 },
		           { 0, 1, 0, 0 },
		           { 1, 1, 1, 0 },
		           { 0, 0, 0, 0 } } },

		{ 4, 23, { { 1, 1, 0, 0 },
		           { 0, 1, 0, 0 },
		           { 0, 1, 0, 0 },
		           { 0, 0, 0, 0 } } },

		{ 4, 23, { { 0, 1, 1, 0 },
		           { 0, 1, 0, 0 },
		           { 0, 1, 0, 0 },
		           { 0, 0, 0, 0 } } },

		{ 4, 23, { { 0, 0, 0, 0 },
		           { 1, 1, 0, 0 },
		           { 0, 1, 1, 0 },
		           { 0, 0, 0, 0 } } },

		{ 4, 23, { { 0, 0, 0, 0 },
		           { 0, 1, 1, 0 },
		           { 1, 1, 0, 0 },
		           { 0, 0, 0, 0 } } },

		{ 4, 23, { { 0, 0, 0, 0 },
		           { 0, 1, 1, 0 },
		           { 0, 1, 1, 0 },
	               { 0, 0, 0, 0 } } }};


	tetris->curr = spawns[rand() % (sizeof(spawns) / sizeof(spawns[0]))];

	if (addTetromino(tetris))
		return -1;

	return 0;
}


static int move(tetris_t *tetris, int xdir, int ydir)
{
	removeTetromino(tetris);

	tetris->curr.x += xdir;
	tetris->curr.y += ydir;

	if (addTetromino(tetris)) {
		tetris->curr.x -= xdir;
		tetris->curr.y -= ydir;
		addTetromino(tetris);

		return -1;
	}

	return 0;
}


static int rotate(tetris_t *tetris)
{
	int i, j;
	tetromino_t old = tetris->curr;

	removeTetromino(tetris);

	for (i = 0; i < 4; ++i)
		for (j = 0; j < 4; ++j)
			tetris->curr.matrix[j][i] = old.matrix[i][j];

	if (addTetromino(tetris)) {
		tetris->curr = old;
		addTetromino(tetris);

		return -1;
	}

	return 0;
}


static void draw(tetris_t *tetris)
{
	int i, j;

	/* clear the term */
	printf("\033[H");

	/* top border */
	for (i = 0; i < FIELD_XSZ + 4; ++i)
		printf("-");
	printf("\n");

	/* margin */
	printf("|");
	for (i = 0; i < FIELD_XSZ + 2; ++i)
		printf(" ");
	printf("|\n");

	/* game field */
	for (j = 0; j < FIELD_YSZ; ++j) {
		printf("| ");
	
		for (i = 0; i < FIELD_XSZ; ++i)
			printf("%c", tetris->field[j][i] ? 'o' : ' ');

		printf(" |\n");
	}

	/* margin */
	printf("|");
	for (i = 0; i < FIELD_XSZ + 2; ++i)
		printf(" ");
	printf("|\n");

	/* bottom border */
	for (i = 0; i < FIELD_XSZ + 4; ++i)
		printf("-");
	printf("\n");
}


static keySpec_t getKey(void)
{
	char c = getchar();

	if (c == 'p' || c == 'P')
		return keyPause;

	if (c == 'r' || c == 'R')
		return keyRot;

	if (c == 0x1B && getchar() == 0x5B) {
		c = getchar();

		if (c == 'C')
			return keyRight;

		if (c == 'D')
			return keyLeft;
	}

	return keyNone;
}


int main(int argc, char *argv[])
{
	tetris_t tetris;
	int level = 1000, ticks = 0, gotLines;
	keySpec_t key;
	struct termios oldt, newt;
	int ch;
	int oldf;

	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

	srand(time(NULL));

	newGame(&tetris);
	spawnTetromino(&tetris);
	printf("Get ready!\n");
	
	sleep(1);

	do {
		draw(&tetris);
		usleep(TIME_STEP);

		switch (key = getKey()) {
		case keyLeft:
			move(&tetris, -1, 0);
			break;

		case keyRight:
			move(&tetris, 1, 0);
			break;

		case keyRot:
			rotate(&tetris);
			break;

		case keyPause:
			do {
				usleep(TIME_STEP);
				key = getKey();
			} while (key != keyPause);
		}

		if (++ticks < TICKS_TO_STEP)
			continue;

		ticks = 0;

		if (!move(&tetris, 0, -1))
			continue;

		gotLines = eatFullLines(&tetris);
		tetris.score += gotLines * gotLines;
	} while (!spawnTetromino(&tetris));

	printf("Game over! Your score: %d\n", tetris.score);

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);
}
