#ifndef _MENU_H
#define _MENU_H

#define MAX_MENUS 32

#include <SDL2/SDL.h>

typedef const char *(*text_callback)();

struct menu_entry {
	const char *text;
	text_callback callback;
};

struct menu {
	int x;
	int y;
	struct menu_entry *entries;
	int num_entries;
	int font_size;
	int border_size;
	int padding;
	SDL_Color background;
	SDL_Color foreground;
	SDL_Color border;
};

extern void init_menu();
extern void push_menu(struct menu *m);

#endif
