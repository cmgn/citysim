OBJECTS := render.o
CC := clang
CFLAGS += -Wall

game: $(OBJECTS) game.c
	$(CC) $(CFLAGS) -lSDL2 -lm -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) game $(OBJECTS)
