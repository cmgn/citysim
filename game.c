#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include <SDL2/SDL.h>

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 768

#define GRID_WIDTH 32
#define GRID_HEIGHT 32

#define CELL_WIDTH ((WINDOW_WIDTH)/(GRID_WIDTH))
#define CELL_HEIGHT ((WINDOW_HEIGHT)/(GRID_HEIGHT))

enum tile_type {
	TILE_GRASS,
	TILE_WATER,
	TILE_ROAD,
	TILE_TYPE_COUNT,
};

struct tile {
	enum tile_type type;
};

static SDL_Window *window = 0;
static SDL_Renderer *renderer = 0;
static struct tile grid[GRID_HEIGHT][GRID_WIDTH] = { 0 };

// Rendering data.
static const char *tile_texture_paths[TILE_TYPE_COUNT] = {
		/* TILE_GRASS */ "assets/grass.bmp",
		/* TILE_WATER */ "assets/water.bmp",
		/* TILE_ROAD  */ "assets/road.bmp",
};
static SDL_Texture *tile_textures[TILE_TYPE_COUNT] = { 0 };

static int init_rendering()
{
	for (int i = 0; i < TILE_TYPE_COUNT; i++) {
		const char *path = tile_texture_paths[i];
		SDL_Surface *surface = SDL_LoadBMP(path);
		if (!surface) {
			SDL_Log("Failed to load %s: %s", path, SDL_GetError());
			return -1;
		}
		SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer,
								    surface);
		if (!texture) {
			SDL_Log("Failed to build texture for %s: %s", path,
				SDL_GetError());
			return -1;
		}
		SDL_FreeSurface(surface);
		tile_textures[i] = texture;
	}
	return 0;
}

static void render()
{
	for (int y = 0; y < GRID_HEIGHT; y++) {
		for (int x = 0; x < GRID_WIDTH; x++) {
			SDL_Rect rect = { x * CELL_WIDTH, y * CELL_HEIGHT,
					  CELL_WIDTH, CELL_HEIGHT };
			SDL_Texture *texture = tile_textures[grid[y][x].type];
			SDL_RenderCopy(renderer, texture, 0, &rect);
		}
	}
}

static float random_float()
{
	return (float)rand() / (float)RAND_MAX;
}

static int in_grid(int x, int y)
{
	return x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT;
}

static void place_random_lake_step(int x, int y, float probability)
{
	if (random_float() > probability) {
		return;
	}
	if (!in_grid(x, y) || grid[y][x].type == TILE_WATER) {
		return;
	}
	grid[y][x].type = TILE_WATER;
	place_random_lake_step(x - 1, y, probability * 0.875);
	place_random_lake_step(x + 1, y, probability * 0.875);
	place_random_lake_step(x, y - 1, probability * 0.875);
	place_random_lake_step(x, y + 1, probability * 0.875);
}

static void place_random_lake()
{
	int x = rand() % GRID_WIDTH;
	int y = rand() % GRID_HEIGHT;
	place_random_lake_step(x, y, 1.0f);
}

static float euclidean_distance(int x1, int y1, int x2, int y2)
{
	return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}

static void swap(void *a, void *b, int size)
{
	unsigned char buf[size];
	memcpy(buf, a, size);
	memcpy(a, b, size);
	memcpy(b, buf, size);
}

static void draw_road(int x1, int y1, int x2, int y2)
{
	if (x1 > x2) {
		swap(&x1, &x2, sizeof(x1));
	}
	if (y1 > y2) {
		swap(&y1, &y2, sizeof(y1));
	}
	for (int x = x1; x <= x2; x++) {
		grid[y1][x].type = TILE_ROAD;
	}
	for (int y = y1; y <= y2; y++) {
		grid[y][x2].type = TILE_ROAD;
	}
}

static void place_random_road()
{
	int x1 = rand() % GRID_WIDTH;
	int y1 = rand() % GRID_HEIGHT;
	int x2;
	int y2;
	do {
		x2 = rand() % GRID_WIDTH;
		y2 = rand() % GRID_HEIGHT;
	} while (euclidean_distance(x1, y1, x2, y2) < 8);
	draw_road(x1, y1, x2, y2);
}

int main(int argc, char **argv)
{
	srand(time(NULL));
	window = SDL_CreateWindow(argv[0], SDL_WINDOWPOS_UNDEFINED,
				  SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH,
				  WINDOW_HEIGHT, 0);
	if (!window) {
		SDL_Log("Failed to create window: %s", SDL_GetError());
		exit(1);
	}
	renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_ACCELERATED);
	if (!renderer) {
		SDL_Log("Failed to create renderer: %s", SDL_GetError());
		exit(1);
	}
	if (init_rendering() < 0) {
		goto quit;
	}
	place_random_lake();
	place_random_road();
	for (;;) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				goto quit;
			}
		}
		render();
		SDL_RenderPresent(renderer);
	}
quit:
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
