# --- 1. VARIABLES DE CONFIGURACIÓN ---
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -IC:/msys64/ucrt64/include/SDL2
SRC_DIR = src
BUILD_DIR = build
TARGET = chip8_emu

# --- 2. DETECCIÓN DE SISTEMA OPERATIVO ---
# Si detecta Windows, usa las banderas específicas de MinGW y añade ".exe"
ifeq ($(OS),Windows_NT)
    LDFLAGS = -lmingw32 -lSDL2main -lSDL2
    TARGET_EXEC = $(TARGET).exe
else
# Si es Linux (o macOS), usa solo la bandera estándar de SDL2
    LDFLAGS = -lSDL2
    TARGET_EXEC = $(TARGET)
endif

# --- 3. BÚSQUEDA DE ARCHIVOS ---
# $(wildcard ...) busca todos los archivos que terminen en .c dentro de src/
SRCS = $(wildcard $(SRC_DIR)/*.c)

# $(patsubst ...) toma la lista de SRCS y cambia el texto "src/archivo.c" por "build/archivo.o"
# Los archivos .o (objetos) son el código compilado antes de empaquetarlo en el .exe
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# --- 4. REGLAS PRINCIPALES ---
# .PHONY avisa a Make de que "all" y "clean" son comandos, no archivos reales
.PHONY: all clean

# Cuando escribes "make", ejecuta esta regla por defecto.
# Sus requisitos son que exista la carpeta build y el archivo ejecutable.
all: $(BUILD_DIR) $(TARGET_EXEC)

# Si la carpeta build no existe, la crea
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# --- 5. REGLA DE ENLAZADO (LINKING) ---
# Une todos los archivos .o para crear el .exe final.
# $@ significa "el nombre de esta regla" (el ejecutable)
# $^ significa "todos los requisitos" (todos los .o)
$(TARGET_EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# --- 6. REGLA DE COMPILACIÓN ---
# Enseña a Make cómo transformar cualquier archivo src/X.c en build/X.o
# $< significa "el primer requisito" (el archivo .c)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# --- 7. LIMPIEZA ---
# Se ejecuta escribiendo "make clean" en la terminal. Borra los archivos generados.
clean:
	rm -rf $(BUILD_DIR) $(TARGET_EXEC)
