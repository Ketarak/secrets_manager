# Makefile - Secrets Manager (passmgr)

CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -O2 -g $(shell pkg-config --cflags libsodium 2>/dev/null || echo "")
LDFLAGS = $(shell pkg-config --libs libsodium 2>/dev/null || echo "-lsodium")

# Dossiers du projet
SRC_DIR = src
OBJ_DIR = obj
TARGET = passmgr

# Variables pour Native Messaging (Firefox)
HOST_NAME = passmgr
MANIFEST_SRC = passmgr.json.template
MANIFEST_DIR = $(HOME)/.mozilla/native-messaging-hosts
MANIFEST_DEST = $(MANIFEST_DIR)/$(HOST_NAME).json
BINARY_ABS_PATH = $(abspath $(TARGET))

# Liste des sources et objets
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

.PHONY: all clean debug sanitize install-native

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

# Installer le manifest Native Messaging pour Firefox
install-native: $(TARGET)
	@mkdir -p $(MANIFEST_DIR)
	@sed "s|TARGET_PATH|$(BINARY_ABS_PATH)|g" $(MANIFEST_SRC) > $(MANIFEST_DEST)
	@echo "[+] Manifest native messaging installe dans : $(MANIFEST_DEST)"
	@echo "    Chemin absolu vers l'executable : $(BINARY_ABS_PATH)"

# Nettoyage des objets et de l'exécutable
clean:
	rm -rf $(OBJ_DIR) $(TARGET)

# Aide à la compilation et aux commandes
help:
	@echo "Commandes disponibles :"
	@echo "  make          : Compile le projet"
	@echo "  make sanitize : Compile avec AddressSanitizer"
	@echo "  make clean    : Supprime les fichiers objets et l'exécutable"
	@echo "  make install-native : Installe le manifest native messaging pour Firefox"