OBJECTS :=
CC := clang
CFLAGS += -Wall -lSDL2 -lm

game: $(OBJECTS) game.c
	$(CC) $(CFLAGS) -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<

.PHONY: clean
clean:
	$(RM) game
