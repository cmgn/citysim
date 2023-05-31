#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include <SDL2/SDL.h>

#include "game.h"
#include "render.h"
#include "menu.h"
#include "simulate.h"

SDL_Window *window = 0;
SDL_Renderer *renderer = 0;
struct tile grid[GRID_HEIGHT][GRID_WIDTH] = { 0 };

static struct graph *simple_graph()
{
	static float values[] = { -3.0, 2.0, -1.0, 5.0, 2.0, 7.0, -2.5, 1.5 };
	static int num_values = sizeof(values)/sizeof(*values);
	static struct graph g = { 0 };
	g = (struct graph){
		.x = 10,
		.y = 100,
		.w = 300,
		.h = 100,
		.values = values,
		.num_values = num_values,
	};
	return &g;
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
	if (init_render() < 0) {
		goto quit;
	}
	init_menu();
	init_simulate();
	render_push_graph(simple_graph());
	unsigned long long last_frame = SDL_GetTicks64() - 1000;
	for (;;) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				goto quit;
			}
		}
		unsigned long long this_frame = SDL_GetTicks64();
		if (this_frame - last_frame < 250) {
			continue;
		}
		last_frame = this_frame;
		simulate();
		SDL_Log("Population: %d; Net Migration: %d", population,
			-emigration);
		render();
		SDL_RenderPresent(renderer);
	}
quit:
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
