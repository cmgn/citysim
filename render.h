#ifndef _RENDER_H
#define _RENDER_H

struct menu;

struct graph {
	int x;
	int y;
	int w;
	int h;
	float *values;
	int num_values;
};

extern int init_render();
extern void render();

extern void render_mark_tile(int x, int y);

extern void render_push_menu(struct menu *m);
extern void render_pop_menu();

extern void render_push_graph(struct graph *g);
extern void render_pop_graph();

#endif
