#include "vault.h"
#include "crypto.h"
#include <sodium.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static char *sodium_strdup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *copy = (char *)sodium_malloc(len + 1);
    if (!copy) return NULL;
    memcpy(copy, s, len);
    copy[len] = '\0';
    return copy;
}

// Écrit un entier 16-bit en Big-Endian dans le buffer
static void write_uint16(unsigned char *buf, uint16_t val) {
    buf[0] = (val >> 8) & 0xFF;
    buf[1] = val & 0xFF;
}
// Écrit un entier 32-bit en Big-Endian dans le buffer
static void write_uint32(unsigned char *buf, uint32_t val) {
    buf[0] = (val >> 24) & 0xFF;
    buf[1] = (val >> 16) & 0xFF;
    buf[2] = (val >> 8) & 0xFF;
    buf[3] = val & 0xFF;
}
// Lit un entier 16-bit Big-Endian depuis le buffer
static uint16_t read_uint16(const unsigned char *buf) {
    return (uint16_t)((buf[0] << 8) | buf[1]);
}
// Lit un entier 32-bit Big-Endian depuis le buffer
static uint32_t read_uint32(const unsigned char *buf) {
    return (uint32_t)((buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]);
}

Vault *vault_create(void) {
    /*
     * TODO : Étape 3a
     * 1. Allouer la structure Vault (avec sodium_malloc de préférence).
     * 2. Initialiser count = 0 et capacity = 4 (ou autre valeur initiale).
     * 3. Allouer le tableau entries avec sodium_allocarray(capacity, sizeof(Entry)).
     * 4. Initialiser à zéro (avec memset) le tableau d'entrées.
     * 5. Retourner le pointeur Vault.
     */
    Vault *vault = (Vault *)sodium_malloc(sizeof(Vault));
    if (!vault) {
        fprintf(stderr, "Error: Failed to allocate memory for Vault.\n");
        return NULL;
    }
    vault->count = 0;
    vault->capacity = 4;
    vault->entries = (Entry *)sodium_allocarray(vault->capacity, sizeof(Entry));
    if (!vault->entries) {
        fprintf(stderr, "Error: Failed to allocate memory for Vault entries.\n");
        sodium_free(vault);
        return NULL;
    }
    memset(vault->entries, 0, vault->capacity * sizeof(Entry));

    return vault;
}

void vault_free(Vault *vault) {
    /*
     * TODO : Étape 3b
     * 1. Vérifier si vault est NULL.
     * 2. Boucler sur toutes les entrées i de 0 à vault->count - 1 :
     *    a. Libérer le titre et le type de l'entrée.
     *    b. Boucler sur tous les champs j de 0 à entry.count - 1 :
     *       - Si le champ ou la valeur est sensible (ou par précaution pour tous les champs) :
     *         effacer activement la mémoire de la valeur avec sodium_memzero(field.value, strlen(field.value))
     *         avant de la libérer.
     *       - Libérer field.name et field.value.
     *    c. Libérer le tableau entry.fields (avec sodium_free).
     * 3. Libérer le tableau d'entrées vault->entries.
     * 4. Libérer la structure vault.
     */
    if (!vault) {
        return;
    }
    while (vault->count > 0) {
        Entry *entry = &vault->entries[vault->count - 1];
        sodium_free(entry->title);
        sodium_free(entry->type);
        while (entry->count > 0) {
            Field *field = &entry->fields[entry->count - 1];
            if (field->is_sensitive && field->value) {
                sodium_memzero(field->value, strlen(field->value));
            }
                sodium_free(field->name);
                sodium_free(field->value);
            entry->count--;
        }
        sodium_free(entry->fields);
        vault->count--; 
    }
    sodium_free(vault->entries);
    sodium_free(vault);
}

Entry *vault_add_entry(Vault *vault, const char *title, const char *type) {
    /*
     * TODO : Étape 3c
     * 1. Vérifier si une entrée avec le même titre existe déjà via vault_find_entry().
     *    Si oui, renvoyer NULL (les doublons de titre ne sont pas autorisés).
     * 2. Si count >= capacity :
     *    - Doubler la capacité (capacity = capacity * 2).
     *    - Allouer un nouveau tableau d'entrées de taille capacity.
     *    - Copier les anciennes entrées.
     *    - Libérer l'ancien tableau d'entrées (attention à ne pas libérer les sous-pointeurs !).
     *    - Remplacer vault->entries par le nouveau tableau.
     * 3. Récupérer un pointeur vers la nouvelle entrée vide : Entry *entry = &vault->entries[vault->count].
     * 4. Allouer et copier entry->title = strdup(title) (ou sodium_malloc).
     * 5. Allouer et copier entry->type = strdup(type).
     * 6. Initialiser entry->count = 0, entry->capacity = 4.
     * 7. Allouer entry->fields = sodium_allocarray(entry->capacity, sizeof(Field)).
     * 8. Initialiser le tableau de champs à zéro.
     * 9. Incrémenter vault->count.
     * 10. Retourner le pointeur entry.
     */
     if (vault_find_entry(vault, title) != NULL) {
        fprintf(stderr, "Error: Entry with title '%s' already exists.\n", title);
        return NULL;
    }
    if (vault->count >= vault->capacity) {
        size_t new_capacity = vault->capacity * 2;
        Entry *new_entries = (Entry *)sodium_allocarray(new_capacity, sizeof(Entry));
        if (!new_entries) {
            fprintf(stderr, "Error: Failed to allocate memory for Vault entries.\n");
            return NULL;
        }
        memcpy(new_entries, vault->entries, vault->count * sizeof(Entry));
        sodium_free(vault->entries);
        vault->entries = new_entries;
        vault->capacity = new_capacity;
    }
    Entry *entry = &vault->entries[vault->count];
    entry->title = sodium_strdup(title);
    entry->type = sodium_strdup(type);
    if (!entry->title || !entry->type) {
        fprintf(stderr, "Error: Failed to allocate memory for Entry title or type.\n");
        sodium_free(entry->title);
        sodium_free(entry->type);
        return NULL;
    }
    entry->count = 0;
    entry->capacity = 4;
    entry->fields = (Field *)sodium_allocarray(entry->capacity, sizeof(Field));
    if (!entry->fields) {
        fprintf(stderr, "Error: Failed to allocate memory for Entry fields.\n");
        sodium_free(entry->title);
        sodium_free(entry->type);
        return NULL;
    }
    memset(entry->fields, 0, entry->capacity * sizeof(Field));
    vault->count++;
    return entry;
}

int entry_set_field(Entry *entry, const char *name, const char *value, int is_sensitive) {
    /*
     * TODO : Étape 3d
     * 1. Rechercher si un champ avec le même 'name' existe déjà via entry_find_field().
     *    - Si oui :
     *      - Effacer l'ancienne valeur avec sodium_memzero.
     *      - Libérer l'ancienne valeur.
     *      - Allouer et copier la nouvelle valeur dans field->value.
     *      - Mettre à jour field->is_sensitive = is_sensitive.
     *      - Retourner 0.
     * 2. Si le champ n'existe pas encore :
     *    - Vérifier si entry->count >= entry->capacity.
     *    - Si oui, augmenter la capacité (capacity * 2) et réallouer entry->fields.
     *    - Récupérer Field *field = &entry->fields[entry->count].
     *    - Allouer et copier field->name = strdup(name).
     *    - Allouer et copier field->value = strdup(value) (ou utiliser sodium_malloc pour les valeurs sensibles).
     *    - Configurer field->is_sensitive = is_sensitive.
     *    - Incrémenter entry->count.
     *    - Retourner 0.
     */
    Field *field = entry_find_field(entry, name);
    if (field != NULL) {
        char *new_value = sodium_strdup(value);
        if (!new_value) {
            fprintf(stderr, "Error: Failed to allocate memory for Field value.\n");
            return -1;
        }
        if (field->is_sensitive && field->value) {
            sodium_memzero(field->value, strlen(field->value));
        }
        sodium_free(field->value);
        field->value = new_value;
        field->is_sensitive = is_sensitive;
        return 0;
    }

    if (entry->count >= entry->capacity) {
        size_t new_capacity = entry->capacity * 2;
        Field *new_fields = (Field *)sodium_allocarray(new_capacity, sizeof(Field));
        if (!new_fields) {
            fprintf(stderr, "Error: Failed to allocate memory for Entry fields.\n");
            return -1;
        }
        memcpy(new_fields, entry->fields, entry->count * sizeof(Field));
        sodium_free(entry->fields);
        entry->fields = new_fields;
        entry->capacity = new_capacity;
    }
    field = &entry->fields[entry->count];
    field->name = sodium_strdup(name);
    field->value = sodium_strdup(value);
    if (!field->name || !field->value) {
        fprintf(stderr, "Error: Failed to allocate memory for Field name or value.\n");
        sodium_free(field->name);
        sodium_free(field->value);
        return -1;
    }
    field->is_sensitive = is_sensitive;
    entry->count++;
    return 0;
}

Entry *vault_find_entry(Vault *vault, const char *title) {
    /*
     * TODO : Étape 3e
     * 1. Parcourir les entrées de 0 à vault->count - 1.
     * 2. Si strcmp(entry.title, title) == 0, retourner le pointeur vers cette entrée.
     * 3. Retourner NULL si non trouvé.
     */
    for (size_t i = 0; i < vault->count; i++) {
        if (strcmp(vault->entries[i].title, title) == 0) {
            return &vault->entries[i];
        }
    }
    return NULL;
}

Field *entry_find_field(Entry *entry, const char *name) {
    /*
     * TODO : Étape 3f
     * 1. Parcourir les champs de l'entrée de 0 à entry->count - 1.
     * 2. Si strcmp(field.name, name) == 0, retourner le pointeur vers ce champ.
     * 3. Retourner NULL si non trouvé.
     */
    for (size_t i = 0; i < entry->count; i++) {
        if (strcmp(entry->fields[i].name, name) == 0) {
            return &entry->fields[i];
        }
    }
    return NULL;
}

int vault_delete_entry(Vault *vault, const char *title) {
    /*
     * TODO : Étape 3g
     * 1. Trouver l'index de l'entrée correspondant au titre.
     * 2. Si non trouvée, retourner -1.
     * 3. Libérer la mémoire de cette entrée (en effaçant les valeurs sensibles avec sodium_memzero d'abord).
     * 4. Décaler les entrées suivantes vers la gauche pour combler le vide.
     * 5. Décrémenter vault->count.
     * 6. Retourner 0.
     */

    Entry *entry = vault_find_entry(vault, title);
    
    if (entry == NULL) {
        return -1;
    }
    size_t index = entry - vault->entries;

    for (size_t j = 0; j < entry->count; j++) {
        Field *field = &entry->fields[j];
        if (field->is_sensitive && field->value) {
            sodium_memzero(field->value, strlen(field->value));
        }
        sodium_free(field->name);
        sodium_free(field->value);
    }
    sodium_free(entry->fields);
    sodium_free(entry->title);
    sodium_free(entry->type);
    for (size_t i = index; i < vault->count - 1; i++) {
        vault->entries[i] = vault->entries[i + 1];
    }
    vault->count--;
    memset(&vault->entries[vault->count], 0, sizeof(Entry));
    return 0;
}

int vault_serialize(const Vault *vault, unsigned char **out_buf, size_t *out_len) {
    /*
     * TODO : Étape 4a (Sérialisation récursive)
     * 1. Calculer la taille totale requise pour le buffer.
     *    Taille = 1o (version) + 4o (nb d'entrées)
     *    Pour chaque entrée :
     *      + 2o (titre len) + titre len
     *      + 2o (type len) + type len
     *      + 2o (nb de champs)
     *      Pour chaque champ de l'entrée :
     *        + 2o (nom len) + nom len
     *        + 4o (valeur len) + valeur len
     *        + 1o (is_sensitive flag)
     * 2. Allouer out_buf avec la taille calculée (avec sodium_malloc).
     * 3. Écrire la version (VAULT_VERSION) et vault->count dans le buffer.
     * 4. Boucler pour copier chaque entrée et ses champs dans le buffer plat, 
     *    en écrivant d'abord la longueur de chaque chaîne puis la chaîne elle-même.
     *    (Utiliser des fonctions comme htons/htonl ou écrire octet par octet pour être indépendant de l'endianness).
     * 5. Remplir out_len avec la taille totale écrite.
     * 6. Retourner 0 en cas de succès, -1 en cas d'erreur.
     */

    size_t total_size = 5; // Version + Nb d'entrées
    for (size_t i = 0; i < vault->count; i++) {
        Entry *entry = &vault->entries[i];
        total_size += 2 + strlen(entry->title); // Titre len + titre
        total_size += 2 + strlen(entry->type);  // Type len + type
        total_size += 2; // Nb de champs
        for (size_t j = 0; j < entry->count; j++) {
            Field *field = &entry->fields[j];
            total_size += 2 + strlen(field->name);  // Nom len + nom
            total_size += 4 + strlen(field->value); // Valeur len + valeur
            total_size += 1; // IsSensitive flag
        }
    }

    *out_buf = (unsigned char *)sodium_malloc(total_size); // Version + Nb d'entrées
    if (*out_buf == NULL) {
        return -1;
    }
    unsigned char *buf = *out_buf;
    *out_len = total_size;

    size_t offset = 0;
    buf[offset++] = VAULT_VERSION;
    write_uint32(buf + offset, (uint32_t)vault->count);
    offset += 4;
    for (size_t i = 0; i < vault->count; i++) {
        Entry *entry = &vault->entries[i];
        write_uint16(buf + offset, (uint16_t)strlen(entry->title));
        offset += 2;
        memcpy(buf + offset, entry->title, strlen(entry->title));
        offset += strlen(entry->title);

        write_uint16(buf + offset, (uint16_t)strlen(entry->type));
        offset += 2;
        memcpy(buf + offset, entry->type, strlen(entry->type));
        offset += strlen(entry->type);

        write_uint16(buf + offset, (uint16_t)entry->count);
        offset += 2;

        for (size_t j = 0; j < entry->count; j++) {
            Field *field = &entry->fields[j];
            write_uint16(buf + offset, (uint16_t)strlen(field->name));
            offset += 2;
            memcpy(buf + offset, field->name, strlen(field->name));
            offset += strlen(field->name);

            write_uint32(buf + offset, (uint32_t)strlen(field->value));
            offset += 4;
            memcpy(buf + offset, field->value, strlen(field->value));
            offset += strlen(field->value);

            buf[offset++] = field->is_sensitive ? 1 : 0;
        }
    }
    if (offset != total_size) {
        fprintf(stderr, "Error: Serialization size mismatch.\n");
        sodium_free(buf);
        *out_buf = NULL;
        *out_len = 0;
        return -1;
    }
    *out_len = total_size;
    return 0;
}

Vault *vault_deserialize(const unsigned char *buf, size_t len) {
    if (buf == NULL || len < 5) {
        return NULL;
    }
    if (buf[0] != VAULT_VERSION) {
        fprintf(stderr, "Error: Unsupported vault version %d.\n", buf[0]);
        return NULL;
    }

    Vault *vault = vault_create();
    if (vault == NULL) {
        return NULL;
    }

    size_t offset = 1;
    uint32_t entries_count = read_uint32(buf + offset);
    offset += 4;

    for (size_t i = 0; i < entries_count; i++) {
        if (offset + 6 > len) {
            goto error;
        }

        uint16_t title_len = read_uint16(buf + offset);
        offset += 2;
        if (offset + title_len > len) {
            goto error;
        }
        char *title = (char *)sodium_malloc(title_len + 1);
        if (title == NULL) {
            goto error;
        }
        memcpy(title, buf + offset, title_len);
        title[title_len] = '\0';
        offset += title_len;

        uint16_t type_len = read_uint16(buf + offset);
        offset += 2;
        if (offset + type_len > len) {
            sodium_free(title);
            goto error;
        }
        char *type = (char *)sodium_malloc(type_len + 1);
        if (type == NULL) {
            sodium_free(title);
            goto error;
        }
        memcpy(type, buf + offset, type_len);
        type[type_len] = '\0';
        offset += type_len;

        Entry *entry = vault_add_entry(vault, title, type);
        sodium_free(title);
        sodium_free(type);
        if (entry == NULL) {
            goto error;
        }

        uint16_t fields_count = read_uint16(buf + offset);
        offset += 2;

        for (size_t j = 0; j < fields_count; j++) {
            if (offset + 7 > len) {
                goto error;
            }

            uint16_t name_len = read_uint16(buf + offset);
            offset += 2;
            if (offset + name_len > len) {
                goto error;
            }
            char *name = (char *)sodium_malloc(name_len + 1);
            if (name == NULL) {
                goto error;
            }
            memcpy(name, buf + offset, name_len);
            name[name_len] = '\0';
            offset += name_len;

            uint32_t val_len = read_uint32(buf + offset);
            offset += 4;
            if (offset + val_len > len) {
                sodium_free(name);
                goto error;
            }
            char *value = (char *)sodium_malloc(val_len + 1);
            if (value == NULL) {
                sodium_free(name);
                goto error;
            }
            memcpy(value, buf + offset, val_len);
            value[val_len] = '\0';
            offset += val_len;

            if (offset + 1 > len) {
                sodium_free(name);
                sodium_free(value);
                goto error;
            }
            int is_sensitive = buf[offset++];

            int ret = entry_set_field(entry, name, value, is_sensitive);
            if (is_sensitive) {
                sodium_memzero(value, val_len);
            }
            sodium_free(name);
            sodium_free(value);
            if (ret != 0) {
                goto error;
            }
        }
    }

    return vault;

error:
    fprintf(stderr, "Error: Failed to deserialize vault (corrupted data or memory error).\n");
    if (vault) {
        vault_free(vault);
    }
    return NULL;
}

int vault_save(const Vault *vault, const char *filepath, const unsigned char *key, const unsigned char *salt) {
    /*
     * TODO : Étape 5c
     * Identique à l'implémentation précédente :
     * 1. Sérialiser (vault_serialize).
     * 2. Chiffrer (encrypt_data).
     * 3. Écrire dans filepath.tmp : Salt (16o) + Nonce (24o) + Ciphertext.
     * 4. Remplacer le fichier original par rename().
     */
    unsigned char *serialized_buf = NULL;
    size_t serialized_len = 0;

    if (vault_serialize(vault, &serialized_buf, &serialized_len) != 0) {
        fprintf(stderr, "Error: Failed to serialize vault.\n");
        return -1;
    }

    size_t ciphertext_len = serialized_len + crypto_secretbox_MACBYTES;
    unsigned char *ciphertext = (unsigned char *)sodium_malloc(ciphertext_len);
    if (ciphertext == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for ciphertext.\n");
        sodium_free(serialized_buf);
        return -1;
    }

    unsigned char nonce[NONCE_SIZE];
    if (encrypt_data(serialized_buf, serialized_len, key, ciphertext, nonce) != 0) {
        fprintf(stderr, "Error: Failed to encrypt vault data.\n");
        sodium_free(serialized_buf);
        sodium_free(ciphertext);
        return -1;
    }

    char tmp_filepath[1024];
    snprintf(tmp_filepath, sizeof(tmp_filepath), "%s.tmp", filepath);
    FILE *fp = fopen(tmp_filepath, "wb");
    if (!fp) {
        fprintf(stderr, "Error: Failed to open temporary file for writing.\n");
        sodium_free(serialized_buf);
        sodium_free(ciphertext);
        return -1;
    }

    if (fwrite(salt, 1, SALT_SIZE, fp) != SALT_SIZE ||
        fwrite(nonce, 1, NONCE_SIZE, fp) != NONCE_SIZE ||
        fwrite(ciphertext, 1, ciphertext_len, fp) != ciphertext_len) {
        fprintf(stderr, "Error: Failed to write to temporary file.\n");
        fclose(fp);
        remove(tmp_filepath);
        sodium_free(serialized_buf);
        sodium_free(ciphertext);
        return -1;
    }

    fclose(fp);

    if (rename(tmp_filepath, filepath) != 0) {
        fprintf(stderr, "Error: Failed to rename temporary file to target file.\n");
        remove(tmp_filepath);
        sodium_free(serialized_buf);
        sodium_free(ciphertext);
        return -1;
    }

    sodium_memzero(serialized_buf, serialized_len);
    sodium_free(serialized_buf);
    sodium_free(ciphertext);
    return 0;
}

Vault *vault_load(const char *filepath, const unsigned char *key, unsigned char *out_salt) {
    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        // C'est normal si le fichier n'existe pas encore (première exécution)
        return NULL;
    }

    // 1. Calcul de la taille totale du fichier
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Taille minimale requise : Salt (16o) + Nonce (24o) + Tag MAC (16o) = 56o
    if (file_size < (long)(SALT_SIZE + NONCE_SIZE + crypto_secretbox_MACBYTES)) {
        fprintf(stderr, "Error: Vault file is too small or corrupted.\n");
        fclose(fp);
        return NULL;
    }

    // 2. Lecture du Salt public
    if (fread(out_salt, 1, SALT_SIZE, fp) != SALT_SIZE) {
        fprintf(stderr, "Error: Failed to read salt.\n");
        fclose(fp);
        return NULL;
    }

    // 3. Lecture du Nonce
    unsigned char nonce[NONCE_SIZE];
    if (fread(nonce, 1, NONCE_SIZE, fp) != NONCE_SIZE) {
        fprintf(stderr, "Error: Failed to read nonce.\n");
        fclose(fp);
        return NULL;
    }

    // 4. Lecture du Ciphertext restant
    size_t ciphertext_len = file_size - SALT_SIZE - NONCE_SIZE;
    unsigned char *ciphertext = (unsigned char *)sodium_malloc(ciphertext_len);
    if (!ciphertext) {
        fprintf(stderr, "Error: Failed to allocate memory for ciphertext.\n");
        fclose(fp);
        return NULL;
    }

    if (fread(ciphertext, 1, ciphertext_len, fp) != ciphertext_len) {
        fprintf(stderr, "Error: Failed to read ciphertext.\n");
        sodium_free(ciphertext);
        fclose(fp);
        return NULL;
    }

    fclose(fp); // On peut fermer le fichier car tout a été chargé en RAM

    // 5. Déchiffrement
    size_t plaintext_len = ciphertext_len - crypto_secretbox_MACBYTES;
    unsigned char *plaintext = (unsigned char *)sodium_malloc(plaintext_len);
    if (!plaintext) {
        fprintf(stderr, "Error: Failed to allocate memory for plaintext.\n");
        sodium_free(ciphertext);
        return NULL;
    }

    if (decrypt_data(ciphertext, ciphertext_len, nonce, key, plaintext) != 0) {
        fprintf(stderr, "Error: Decryption failed (wrong master password or file corruption).\n");
        sodium_free(ciphertext);
        sodium_free(plaintext);
        return NULL;
    }

    sodium_free(ciphertext); // Plus besoin du chiffré

    // 6. Désérialisation pour reconstruire le coffre en mémoire
    Vault *vault = vault_deserialize(plaintext, plaintext_len);

    // Sécurité : effacer le tampon en clair
    sodium_memzero(plaintext, plaintext_len);
    sodium_free(plaintext);

    return vault;
}