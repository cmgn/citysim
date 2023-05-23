#include <SDL2/SDL.h>

#include "game.h"
#include "menu.h"
#include "render.h"
#include "font8x8_basic.h"

struct rendering_tile {
	SDL_Surface *surface;
	SDL_Texture *texture;
	int needs_update;
};

struct compiled_menu {
	int w;
	int h;
	SDL_Texture *texture;
	struct menu *menu;
};

#define RGRID_WIDTH 4
#define RGRID_HEIGHT 4

#define RCELL_WIDTH ((GRID_WIDTH)/(RGRID_HEIGHT))
#define RCELL_HEIGHT ((GRID_HEIGHT)/(RGRID_HEIGHT))

static struct rendering_tile rendering_grid[RGRID_HEIGHT][RGRID_WIDTH] = { 0 };

static const char *tile_bitmap_paths[TILE_TYPE_COUNT] = {
		/* TILE_GRASS */ "assets/grass.bmp",
		/* TILE_WATER */ "assets/water.bmp",
		/* TILE_ROAD  */ "assets/road.bmp",
		/* TILE_HOUSE */ "assets/house.bmp",
};
static SDL_Surface *tile_surfaces[TILE_TYPE_COUNT] = { 0 };

static struct compiled_menu menus[MAX_MENUS] = { 0 };
static int num_menus = 0;

static unsigned int sdl_color_to_uint32(const SDL_Color *c)
{
	unsigned int ret = 0;
	ret |= (unsigned int)(c->a) << 24;
	ret |= (unsigned int)(c->r) << 16;
	ret |= (unsigned int)(c->g) <<  8;
	ret |= (unsigned int)(c->b) <<  0;
	return ret;
}

// TODO(cmgn): provided colour must already be converted?
static void write_text(SDL_Surface *surface, int x, int y, const char *text,
		       int size, SDL_Color *colour)
{
	unsigned int cint = sdl_color_to_uint32(colour);
	for (int k = 0; text[k]; k++) {
		int c = text[k];
		for (int j = 0; j < 8; j++) {
			for (int i = 0; i < 8; i++) {
				SDL_Rect rect = { x + (i + k * 8) * size,
						  y + j * size, size, size };
				int bit = font8x8_basic[c][j] & (1 << i);
				if (bit) {
					SDL_FillRect(surface, &rect, cint);
				}
			}
		}
	}
}

static int init_rendering_tile(struct rendering_tile *rtile)
{
	rtile->surface = SDL_CreateRGBSurface(0, CELL_WIDTH * RCELL_WIDTH,
					      CELL_HEIGHT * RCELL_HEIGHT, 32,
					      0, 0, 0, 0);
	if (!rtile->surface) {
		return -1;
	}
	rtile->needs_update = 1;
	return 0;
}

static int init_rendering_grid()
{
	for (int y = 0; y < RGRID_HEIGHT; y++) {
		for (int x = 0; x < RGRID_WIDTH; x++) {
			if (init_rendering_tile(&rendering_grid[y][x]) < 0) {
				return -1;
			}
		}
	}
	return 0;
}

static int init_tile_surfaces()
{
	for (int i = 0; i < TILE_TYPE_COUNT; i++) {
		const char *path = tile_bitmap_paths[i];
		SDL_Surface *surface = SDL_LoadBMP(path);
		if (!surface) {
			SDL_Log("Failed to load %s: %s", path, SDL_GetError());
			return -1;
		}
		tile_surfaces[i] = surface;
	}
	return 0;
}

typedef int (*init_function)();

int init_render()
{
	init_function init_functions[] = {
		init_rendering_grid,
		init_tile_surfaces,
	};
	int num_init_functions = sizeof(init_functions)/sizeof(*init_functions);
	for (int i = 0; i < num_init_functions; i++) {
		if (init_functions[i]() < 0) {
			return -1;
		}
	}
	return 0;
}

static void update_rendering_tile(int x, int y)
{
	struct rendering_tile *rtile = &rendering_grid[y][x];
	if (!rtile->needs_update) {
		return;
	}
	if (rtile->texture) {
		SDL_DestroyTexture(rtile->texture);
	}
	for (int dy = 0; dy < RCELL_HEIGHT; dy++) {
		struct tile *grid_row = grid[y * RCELL_HEIGHT + dy];
		for (int dx = 0; dx < RCELL_WIDTH; dx++) {
			struct tile *tile = &grid_row[x * RCELL_WIDTH + dx];
			SDL_Surface *tsurface = tile_surfaces[tile->type];
			SDL_Rect rect = { dx * CELL_WIDTH, dy * CELL_HEIGHT,
					  CELL_WIDTH, CELL_HEIGHT };
			SDL_BlitScaled(tsurface, 0, rtile->surface, &rect);
		}
	}
	rtile->texture = SDL_CreateTextureFromSurface(renderer,
						      rtile->surface);
	rtile->needs_update = 0;
}

static void update_rendering_grid()
{
	for (int y = 0; y < RGRID_HEIGHT; y++) {
		for (int x = 0; x < RGRID_WIDTH; x++) {
			update_rendering_tile(x, y);
		}
	}
}

static void render_grid()
{
	for (int y = 0; y < RGRID_HEIGHT; y++) {
		for (int x = 0; x < RGRID_WIDTH; x++) {
			struct rendering_tile *rtile = &rendering_grid[y][x];
			SDL_Rect rect = { x * RCELL_WIDTH * CELL_WIDTH,
					  y * RCELL_HEIGHT * CELL_HEIGHT,
					  RCELL_WIDTH * CELL_WIDTH,
					  RCELL_HEIGHT * CELL_HEIGHT };
			SDL_RenderCopy(renderer, rtile->texture, 0, &rect);
		}
	}
}

static void render_menus()
{
	for (int i = 0; i < num_menus; i++) {
		struct compiled_menu *cm = &menus[i];
		SDL_Rect rect = { cm->menu->x, cm->menu->y, cm->w, cm->h };
		SDL_RenderCopy(renderer, cm->texture, 0, &rect);
	}
}

void render()
{
	update_rendering_grid();
	render_grid();
	render_menus();
}


void render_mark_tile(int x, int y)
{
	x /= RCELL_WIDTH;
	y /= RCELL_HEIGHT;
	rendering_grid[y][x].needs_update = 1;
}

static void compile_menu(struct compiled_menu *cm, struct menu *m)
{
	cm->menu = m;
	cm->h = m->num_entries * 8 * m->font_size;
	cm->h += m->num_entries * 2 * m->padding;
	cm->h += (m->num_entries + 1) * m->border_size;
	cm->w = 0;
	for (int i = 0; i < m->num_entries; i++) {
		struct menu_entry *e = &m->entries[i];
		int e_width = strlen(e->text) * 8 * m->font_size;
		if (e_width > cm->w) {
			cm->w = e_width;
		}
	}
	cm->w += m->border_size * 2;
	cm->w += m->padding * 2;
	SDL_Surface *surface = SDL_CreateRGBSurface(0, cm->w, cm->h, 32,
						    0, 0, 0, 0);
	if (!surface) {
		SDL_Log("Failed to create menu surface: %s", SDL_GetError());
		exit(1);
	}
	SDL_FillRect(surface, 0, sdl_color_to_uint32(&m->border));
	for (int i = 0; i < m->num_entries; i++) {
		struct menu_entry *e = &m->entries[i];
		int x_off = m->border_size;
		int y_off = i * m->font_size * 8;
		y_off += (i + 1) * m->border_size;
		y_off += m->padding * i * 2;
		SDL_Rect rect = { x_off, y_off, cm->w - 2 * m->border_size,
				  m->font_size * 8 + 2 * m->padding };
		SDL_FillRect(surface, &rect,
			     sdl_color_to_uint32(&m->background));
		write_text(surface, x_off + m->padding, y_off + m->padding,
			   e->text, m->font_size, &m->foreground);
	}
	SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
	if (!texture) {
		SDL_Log("Failed to create menu texture: %s", SDL_GetError());
		exit(1);
	}
	SDL_FreeSurface(surface);
	cm->texture = texture;
}

void render_push_menu(struct menu *m)
{
	compile_menu(&menus[num_menus++], m);
}

void render_pop_menu()
{
	num_menus--;
	SDL_DestroyTexture(menus[num_menus].texture);
}
