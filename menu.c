#include "menu.h"
#include "render.h"

static struct menu *menus[MAX_MENUS] = { 0 };
static int num_menus = 0;

static struct menu *build_simple_menu()
{
	static struct menu_entry entries[] = {
		{ "Option A" },
		{ "Option B" },
		{ "Option C" },
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
