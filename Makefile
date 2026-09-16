# Made with AI as I dont know Makefile lol


#CC = gcc
CFLAGS = -Ithirdparty/sdl3/include -Wall -Wextra -O2 -flto -ffunction-sections -fdata-sections

# Route to compiled SDL
SDL_LIB = thirdparty/sdl3/lib/libSDL3.a

# Static linking flags
LDFLAGS = -static -flto -Wl,--gc-sections -s $(SDL_LIB) \
          -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 \
          -lversion -luuid -lsetupapi -ldxguid

TARGET = chip8_emu.exe
SRCS = src/main.c src/chip8.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(TARGET)

.PHONY: all clean
