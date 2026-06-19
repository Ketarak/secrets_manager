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
    return NULL;
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
    return -1;
}

Entry *vault_find_entry(Vault *vault, const char *title) {
    /*
     * TODO : Étape 3e
     * 1. Parcourir les entrées de 0 à vault->count - 1.
     * 2. Si strcmp(entry.title, title) == 0, retourner le pointeur vers cette entrée.
     * 3. Retourner NULL si non trouvé.
     */
    return NULL;
}

Field *entry_find_field(Entry *entry, const char *name) {
    /*
     * TODO : Étape 3f
     * 1. Parcourir les champs de l'entrée de 0 à entry->count - 1.
     * 2. Si strcmp(field.name, name) == 0, retourner le pointeur vers ce champ.
     * 3. Retourner NULL si non trouvé.
     */
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
    return -1;
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
    return -1;
}

Vault *vault_deserialize(const unsigned char *buf, size_t len) {
    /*
     * TODO : Étape 4b (Désérialisation récursive)
     * 1. Vérifier la taille minimale et la version (buf[0] == VAULT_VERSION).
     * 2. Lire le nombre d'entrées (uint32_t à partir de buf[1]).
     * 3. Créer un coffre avec vault_create().
     * 4. Pour chaque entrée :
     *    - Lire la longueur du titre, copier le titre.
     *    - Lire la longueur du type, copier le type.
     *    - Ajouter l'entrée au Vault avec vault_add_entry().
     *    - Lire le nombre de champs (uint16_t).
     *    - Pour chaque champ :
     *      - Lire la longueur du nom, copier le nom.
     *      - Lire la longueur de la valeur, copier la valeur.
     *      - Lire le flag is_sensitive (1o).
     *      - Ajouter le champ à l'entrée avec entry_set_field().
     * 5. Vérifier tout au long de la lecture qu'on ne dépasse pas 'len' pour éviter les dépassements de buffer.
     * 6. Retourner le Vault reconstitué.
     */
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
    return -1;
}

Vault *vault_load(const char *filepath, const unsigned char *key, unsigned char *out_salt) {
    /*
     * TODO : Étape 5d
     * Identique à l'implémentation précédente :
     * 1. Lire filepath : extraire Salt, Nonce, Ciphertext.
     * 2. Déchiffrer (decrypt_data).
     * 3. Désérialiser (vault_deserialize).
     * 4. Nettoyer la RAM temporaire.
     */
    return NULL;
}
