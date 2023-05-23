#include "menu.h"
#include "render.h"
#include "simulate.h"

static struct menu *menus[MAX_MENUS] = { 0 };
static int num_menus = 0;

static const char *population_callback()
{
	static char buffer[128] = { 0 };
	snprintf(buffer, 128, "Population: %06d", population);
	return buffer;
}

static struct menu *build_simple_menu()
{
	static struct menu_entry entries[] = {
		{ 0,          population_callback },
		{ "Option B",                   0 },
		{ "Option C",                   0 },
	};
	static struct menu m = {
		.x = 10,
		.y = 10,
		.entries = entries,
		.num_entries = 3,
		.font_size = 2,
		.border_size = 2,
		.padding = 2,
		.background = { 255, 255, 255, 255 },
		.foreground = { 255,   0,   0, 255 },
		.border =     {   0,   0,   0, 255 },
	};

	return &m;
}

void init_menu()
{	
	push_menu(build_simple_menu());
}

void push_menu(struct menu *m)
{
	menus[num_menus++] = m;
	render_push_menu(m);
}
