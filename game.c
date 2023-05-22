#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include <SDL2/SDL.h>

#include "font8x8_basic.h"

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
	TILE_TYPE_COUNT,
};

struct tile {
	enum tile_type type;
};

static const char *overlay_text = "CITYSIM";
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
static SDL_Texture *overlay_text_texture = 0;

static unsigned int sdl_color_to_uint32(const SDL_Color *c)
{
	unsigned int ret = 0;
	ret |= (unsigned int)(c->a) << 24;
	ret |= (unsigned int)(c->r) << 16;
	ret |= (unsigned int)(c->g) <<  8;
	ret |= (unsigned int)(c->b) <<  0;
	return ret;
}

static void write_text(SDL_Surface *surface, int x, int y, const char *text,
		       int size)
{
	const SDL_Color foreground = { 255, 255, 255, 255 };
	const SDL_Color background = {   0,   0,   0,   0 };
	const unsigned int fgc = sdl_color_to_uint32(&foreground);
	const unsigned int bgc = sdl_color_to_uint32(&background);
	for (int k = 0; text[k]; k++) {
		int c = text[k];
		for (int j = 0; j < 8; j++) {
			for (int i = 0; i < 8; i++) {
				SDL_Rect rect = { x + (i + k * 8) * size,
						  y + j * size, size, size };
				int bit = font8x8_basic[c][j] & (1 << i);
				if (bit) {
					SDL_FillRect(surface, &rect, fgc);
				} else {
					SDL_FillRect(surface, &rect, bgc);
				}
			}
		}
	}
}

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
		tile_textures[i] = texture;
	}
	SDL_Surface *overlay_text_surface = SDL_CreateRGBSurface(
		0, strlen(overlay_text) * 8 * OVERLAY_TEXT_SIZE,
		8 * OVERLAY_TEXT_SIZE, 32, 0, 0, 0, 0
	);
	if (!overlay_text_surface) {
		SDL_Log("Failed to create overlay text surface: %s",
			SDL_GetError());
		return -1;
	}
	write_text(overlay_text_surface, 0, 0, overlay_text,
		   OVERLAY_TEXT_SIZE);
	overlay_text_texture = SDL_CreateTextureFromSurface(
		renderer, overlay_text_surface);
	SDL_FreeSurface(overlay_text_surface);
	return 0;
}

static void render_overlay_text()
{
	SDL_Rect rect = { 10, 10, strlen(overlay_text) * OVERLAY_TEXT_SIZE * 8,
			  OVERLAY_TEXT_SIZE * 8 };
	SDL_RenderCopy(renderer, overlay_text_texture, 0, &rect);
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
	render_overlay_text();
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
