#ifndef VAULT_H
#define VAULT_H

#include <stddef.h>

#define VAULT_VERSION 2

/* 
 * Structure représentant un champ clé-valeur d'un secret.
 * Permet de stocker n'importe quelle métadonnée (login, mot de passe, clé SSH privée, etc.)
 */
typedef struct {
    char *name;       /* Nom du champ (ex: "username", "password", "private_key") */
    char *value;      /* Valeur associée (donnée potentiellement sensible) */
    int is_sensitive; /* Flag : 1 si la donnée est ultra-sensible (à masquer/effacer en priorité), 0 sinon */
} Field;

/* 
 * Structure représentant une entrée (un secret individuel)
 * Contient un tableau dynamique de champs (Fields)
 */
typedef struct {
    char *title;      /* Nom unique identifiant le secret (ex: google.com, id_ed25519) */
    char *type;       /* Type de secret (ex: "login", "ssh", "card", "note") */
    Field *fields;    /* Tableau dynamique de champs */
    size_t count;     /* Nombre de champs dans cette entrée */
    size_t capacity;  /* Capacité maximale allouée pour les champs */
} Entry;

/* 
 * Structure representing the entire vault in memory.
 * Contains a dynamic array of Entries.
 */
typedef struct {
    Entry *entries;   /* Dynamic array of entries */
    size_t count;     /* Current number of entries */
    size_t capacity;  /* Maximum capacity allocated for entries */
} Vault;

/*
 * Creates an empty dynamically allocated Vault.
 * Returns a pointer to the Vault, or NULL on memory allocation failure.
 */
Vault *vault_create(void);

/*
 * Securely frees the Vault, zeroing out sensitive fields before freeing.
 */
void vault_free(Vault *vault);

/*
 * Adds an empty Entry to the Vault with a unique title and a type.
 * Dynamically resizes the entries array if capacity is reached.
 * Returns a pointer to the Entry, or NULL if the title exists or on error.
 */
Entry *vault_add_entry(Vault *vault, const char *title, const char *type);

/*
 * Sets or updates a field (name, value, sensitivity flag) inside an Entry.
 * Returns 0 on success, -1 on failure.
 */
int entry_set_field(Entry *entry, const char *name, const char *value, int is_sensitive);

/*
 * Looks up an Entry by its unique title.
 * Returns a pointer to the Entry, or NULL if not found.
 */
Entry *vault_find_entry(Vault *vault, const char *title);

/*
 * Looks up a Field by its name in a specific Entry.
 * Returns a pointer to the Field, or NULL if not found.
 */
Field *entry_find_field(Entry *entry, const char *name);

/*
 * Deletes an Entry by its title, shifts subsequent entries, and saves.
 * Returns 0 on success, -1 if the entry was not found.
 */
int vault_delete_entry(Vault *vault, const char *title);

/*
 * Serializes the Vault into a binary TLV buffer.
 * out_buf is dynamically allocated and must be freed with sodium_free.
 * Returns 0 on success, -1 on failure.
 */
int vault_serialize(const Vault *vault, unsigned char **out_buf, size_t *out_len);

/*
 * Deserializes a Vault from a binary TLV buffer.
 * Returns a pointer to the loaded Vault, or NULL on corruption/failure.
 */
Vault *vault_deserialize(const unsigned char *buf, size_t len);

/*
 * Securely saves the Vault to disk using atomic replacement (rename).
 * Returns 0 on success, -1 on failure.
 */
int vault_save(const Vault *vault, const char *filepath, const unsigned char *key, const unsigned char *salt);

/*
 * Loads and decrypts the Vault from disk.
 * Returns a pointer to the decrypted Vault, or NULL on failure.
 */
Vault *vault_load(const char *filepath, const unsigned char *key, unsigned char *out_salt);

#endif /* VAULT_H */
