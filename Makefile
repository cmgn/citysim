OBJECTS := render.o simulate.o menu.o
CC := clang
CFLAGS += -Wall

game: $(OBJECTS) game.c
	$(CC) $(CFLAGS) -lSDL2 -lm -o $@ $^

render.o: menu.o

%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) game $(OBJECTS)
