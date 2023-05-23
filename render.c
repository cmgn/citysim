#include <SDL2/SDL.h>

#include "game.h"
#include "render.h"
#include "font8x8_basic.h"

static const char *overlay_text = "CITYSIM";

static const char *tile_texture_paths[TILE_TYPE_COUNT] = {
		/* TILE_GRASS */ "assets/grass.bmp",
		/* TILE_WATER */ "assets/water.bmp",
		/* TILE_ROAD  */ "assets/road.bmp",
		/* TILE_HOUSE */ "assets/house.bmp",
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

int init_render()
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

void render()
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
