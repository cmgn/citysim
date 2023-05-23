#ifndef _GAME_H
#define _GAME_H

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 768

#define GRID_WIDTH 32
#define GRID_HEIGHT 32

#define CELL_WIDTH ((WINDOW_WIDTH)/(GRID_WIDTH))
#define CELL_HEIGHT ((WINDOW_HEIGHT)/(GRID_HEIGHT))

#define OVERLAY_TEXT_SIZE 3

enum tile_type {
	TILE_GRASS,
	TILE_WATER,
	TILE_ROAD,
	TILE_HOUSE,
	TILE_TYPE_COUNT,
};

struct tile {
	enum tile_type type;
};

extern SDL_Window *window;
extern SDL_Renderer *renderer;
extern struct tile grid[GRID_HEIGHT][GRID_WIDTH];

#endif
