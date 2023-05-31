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
	int dynamic;
	SDL_Texture *texture;
	struct menu *menu;
};

struct compiled_graph {
	SDL_Texture *texture;
	struct graph *graph;
};

#define RGBA(R, G, B, A) ((A) << 24 | (R) << 16 | (G) << 8 | (B))

#define RGRID_WIDTH 4
#define RGRID_HEIGHT 4

#define RCELL_WIDTH ((GRID_WIDTH)/(RGRID_HEIGHT))
#define RCELL_HEIGHT ((GRID_HEIGHT)/(RGRID_HEIGHT))

#define MAX_GRAPHS 32

#define GRAPH_LINE_WIDTH 2
#define GRAPH_PADDING 3

#define GRAPH_BG_COLOR RGBA(255, 255, 255, 255)
#define GRAPH_FG_COLOR RGBA(  0,   0,   0, 255)
#define GRAPH_LN_COLOR RGBA(255,   0,   0, 255)

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

static struct compiled_graph graphs[MAX_GRAPHS] = { 0 };
static int num_graphs = 0;

static unsigned int sdl_color_to_uint32(const SDL_Color *c)
{
	unsigned int ret = 0;
	ret |= (unsigned int)(c->a) << 24;
	ret |= (unsigned int)(c->r) << 16;
	ret |= (unsigned int)(c->g) <<  8;
	ret |= (unsigned int)(c->b) <<  0;
	return ret;
}

// TODO(cmgn): provided color must already be converted?
static void write_text(SDL_Surface *surface, int x, int y, const char *text,
		       int size, SDL_Color *color)
{
	unsigned int cint = sdl_color_to_uint32(color);
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

static void compile_menu(struct compiled_menu *cm, struct menu *m)
{
	cm->menu = m;
	cm->h = m->num_entries * 8 * m->font_size;
	cm->h += m->num_entries * 2 * m->padding;
	cm->h += (m->num_entries + 1) * m->border_size;
	cm->w = 0;
	for (int i = 0; i < m->num_entries; i++) {
		struct menu_entry *e = &m->entries[i];
		// TODO(cmgn): Only re-draw when at least one of the callbacks
		// change the text.
		if (e->callback) {
			e->text = e->callback();
			cm->dynamic = 1;
		}
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


static void render_menus()
{
	for (int i = 0; i < num_menus; i++) {
		struct compiled_menu *cm = &menus[i];
		if (cm->dynamic) {
			SDL_DestroyTexture(cm->texture);
			compile_menu(cm, cm->menu);
		}
		SDL_Rect rect = { cm->menu->x, cm->menu->y, cm->w, cm->h };
		SDL_RenderCopy(renderer, cm->texture, 0, &rect);
	}
}

static void draw_line_gradual(SDL_Surface *surface, int x0, int y0, int x1, int y1,
			      unsigned int color, int width)
{
	int dx = x1 - x0;
	int dy = y1 - y0;
	int yi = 1;
	if (dy < 0) {
		yi = -1;
		dy = -dy;
	}
	int d = 2*dy - dx;
	int y = y0;
	for (int x = x0; x <= x1; x++) {
		SDL_Rect r = { x, y, width, width };
		SDL_FillRect(surface, &r, color);
		if (d > 0) {
			y += yi;
			d += 2 * (dy - dx);
		} else {
			d += 2 * dy;
		}
	}
}

static void draw_line_steep(SDL_Surface *surface, int x0, int y0, int x1, int y1,
			    unsigned int color, int width)
{
	int dx = x1 - x0;
	int dy = y1 - y0;
	int xi = 1;
	if (dx < 0) {
		xi = -1;
		dx = -dx;
	}
	int d = 2*dx - dy;
	int x = x0;
	for (int y = y0; y <= y1; y++) {
		SDL_Rect r = { x, y, width, width };
		SDL_FillRect(surface, &r, color);
		if (d > 0) {
			x += xi;
			d += 2 * (dx - dy);
		} else {
			d += 2 * dx;
		}
	}
}

static void draw_line(SDL_Surface *surface, int x0, int y0, int x1, int y1,
		      unsigned int color, int width)
{
	if (abs(y1 - y0) < abs(x1 - x0)) {
		if (x0 > x1) {
			draw_line_gradual(surface, x1, y1, x0, y0, color, width);
		} else {
			draw_line_gradual(surface, x0, y0, x1, y1, color, width);
		}
	} else {
		if (y0 > y1) {
			draw_line_steep(surface, x1, y1, x0, y0, color, width);
		} else {
			draw_line_steep(surface, x0, y0, x1, y1, color, width);
		}
	}
}

static void compile_graph(struct compiled_graph *cg, struct graph *g)
{
	SDL_Surface *surface = SDL_CreateRGBSurface(0, g->w, g->h,
						    32, 0, 0, 0, 0);
	if (!surface) {
		SDL_Log("Failed to create graph surface: %s", SDL_GetError());
		exit(1);
	}
	SDL_FillRect(surface, 0, GRAPH_BG_COLOR);
	SDL_Rect x_axis = { GRAPH_PADDING, GRAPH_PADDING, GRAPH_PADDING,
			    g->h - 2 * GRAPH_PADDING };
	SDL_FillRect(surface, &x_axis, GRAPH_FG_COLOR);
	SDL_Rect y_axis= { GRAPH_PADDING, g->h - 2 * GRAPH_PADDING,
			   g->w - 2 * GRAPH_PADDING, GRAPH_PADDING };
	SDL_FillRect(surface, &y_axis, GRAPH_FG_COLOR);
	float min_y = g->values[0];
	float max_y = g->values[0];
	for (int i = 1; i < g->num_values; i++) {
		if (g->values[i] < min_y) {
			min_y = g->values[i];
		}
		if (g->values[i] > max_y) {
			max_y = g->values[i];
		}
	}
	int w = g->w - 4 * GRAPH_PADDING;
	int h = g->h - 4 * GRAPH_PADDING;
	int x_step = w/(g->num_values - 1);
	int x_offset = 3 * GRAPH_PADDING;
	int prev_x = x_offset;
	int prev_y = h * ((g->values[0] - min_y) / fabs(max_y - min_y));
	prev_y = g->h - prev_y - 3 * GRAPH_PADDING;
	for (int i = 1; i < g->num_values; i++) {
		int x = x_offset + x_step * i;
		int y = h * ((g->values[i] - min_y) / fabs(max_y - min_y));
		y = g->h - y - 3 * GRAPH_PADDING;
		draw_line(surface, prev_x, prev_y, x, y, GRAPH_LN_COLOR,
			  GRAPH_LINE_WIDTH);
		prev_x = x;
		prev_y = y;
	}
	SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
	if (!texture) {
		SDL_Log("Failed to create graph texture: %s", SDL_GetError());
		exit(1);
	}
	SDL_FreeSurface(surface);
	cg->graph = g;
	cg->texture = texture;
}

static void render_graphs()
{
	for (int i = 0; i < num_graphs; i++) {
		struct compiled_graph *cg = &graphs[i];
		struct graph *g = cg->graph;
		SDL_Rect r = { g->x, g->y, g->w, g->h };
		SDL_RenderCopy(renderer, cg->texture, 0, &r);
	}
}

void render()
{
	update_rendering_grid();
	render_grid();
	render_menus();
	render_graphs();
}


void render_mark_tile(int x, int y)
{
	x /= RCELL_WIDTH;
	y /= RCELL_HEIGHT;
	rendering_grid[y][x].needs_update = 1;
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

void render_push_graph(struct graph *g)
{
	compile_graph(&graphs[num_graphs++], g);
}

void render_pop_graph()
{
	num_graphs--;
	SDL_DestroyTexture(graphs[num_graphs].texture);
}
