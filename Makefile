CC = gcc
CFLAGS = -Wall -Wextra -lm 
LDFLAGS = -lmingw32 -lSDL2main -lSDL2
SRC = main.c include/graphic.c include/map.c include/render.c
OUT = build/raycast

windows:
	$(CC) $(SRC) -mwindows -o $(OUT) $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(OUT)
