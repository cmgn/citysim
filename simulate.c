#include <SDL2/SDL.h>

#include "game.h"
#include "simulate.h"

static void update_tile(int x, int y, enum tile_type type)
{
	grid[y][x].type = type;
	render_mark_tile(x, y);
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
	update_tile(x, y, TILE_WATER);
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
		update_tile(y1, x, TILE_ROAD);
	}
	for (int y = y1; y <= y2; y++) {
		update_tile(y, x2, TILE_ROAD);
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

static int has_neighbouring_road(int x, int y)
{
	struct { int dx, dy; } deltas[] = {
		{ -1, +0 }, { +1, +0 },
		{ +0, -1 }, { +0, +1 },
	};
	int num_deltas = sizeof(deltas)/sizeof(*deltas);
	for (int i = 0; i < num_deltas; i++) {
		int x1 = x + deltas[i].dx;
		int y1 = y + deltas[i].dy;
		if (in_grid(x1, y1) && grid[y1][x1].type == TILE_ROAD) {
			return 1;
		}
	}
	return 0;
}

static void place_houses_along_road()
{
	for (int y = 0; y < GRID_HEIGHT; y++) {
		for (int x = 0; x < GRID_WIDTH; x++) {
			if (grid[y][x].type == TILE_GRASS
			    && has_neighbouring_road(x, y)
			    && random_float() < 0.15) {
				update_tile(x, y, TILE_HOUSE);
			}
		}
	}
}

void init_simulate()
{
	place_random_lake();
	place_random_road();
	place_houses_along_road();
}

void simulate()
{
}