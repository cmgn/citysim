OBJECTS := render.o simulate.o menu.o
CC := clang
LINKFLAGS := -lSDL2 -lm
CFLAGS += -Wall

# TODO(cmgn): Is there a better way to do this?
ifeq ($(shell uname -s),Darwin)
	CFLAGS += -I/opt/homebrew/include
	LINKFLAGS += -L/opt/homebrew/lib
endif

game: $(OBJECTS) game.c
	$(CC) $(CFLAGS) $(LINKFLAGS) -o $@ $^

render.o: menu.o

%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) game $(OBJECTS)
