# Makefile - Secrets Manager (passmgr)

CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -O2 -g $(shell pkg-config --cflags libsodium 2>/dev/null || echo "")
LDFLAGS = $(shell pkg-config --libs libsodium 2>/dev/null || echo "-lsodium")

# Dossiers du projet
SRC_DIR = src
OBJ_DIR = obj
TARGET = passmgr

# Liste des sources et objets
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

.PHONY: all clean debug sanitize

all: $(TARGET)

# Liaison de l'exécutable principal
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Compilation des fichiers objets
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Cible avec AddressSanitizer et UndefinedBehaviorSanitizer activés (Étape 8)
sanitize: CFLAGS += -fsanitize=address,undefined -fno-omit-frame-pointer
sanitize: LDFLAGS += -fsanitize=address,undefined
sanitize: clean $(TARGET)

# Nettoyage des objets et de l'exécutable
clean:
	rm -rf $(OBJ_DIR) $(TARGET)

# Aide à la compilation et aux commandes
help:
	@echo "Commandes disponibles :"
	@echo "  make          : Compile le projet"
	@echo "  make sanitize : Compile avec AddressSanitizer"
	@echo "  make clean    : Supprime les fichiers objets et l'exécutable"
