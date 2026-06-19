#include "vault.h"
#include "crypto.h"
#include <sodium.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Vault *vault_create(void) {
    /*
     * TODO : Étape 3a
     * 1. Allouer la structure Vault. Vous pouvez utiliser sodium_malloc(sizeof(Vault))
     *    pour plus de sécurité.
     * 2. Initialiser count = 0, capacity = 4 (ou une autre valeur initiale).
     * 3. Allouer le tableau entries avec sodium_allocarray(capacity, sizeof(Entry))
     *    ou sodium_malloc(capacity * sizeof(Entry)).
     * 4. Initialiser le tableau d'entrées à zéro.
     * 5. Retourner le pointeur Vault.
     */
    return NULL;
}

void vault_free(Vault *vault) {
    /*
     * TODO : Étape 3b
     * 1. Vérifier si vault est NULL.
     * 2. Boucler sur toutes les entrées de 0 à (vault->count - 1) :
     *    - Effacer le mot de passe de la mémoire en utilisant sodium_memzero()
     *      (taille = strlen(entry->password)) avant de le libérer.
     *    - Libérer title, login, password (avec sodium_free() si alloués avec sodium_malloc,
     *      ou free() standard selon votre choix d'allocation).
     * 3. Libérer le tableau d'entrées (vault->entries).
     * 4. Effacer le reste de la structure vault et la libérer.
     */
}

int vault_add_entry(Vault *vault, const char *title, const char *login, const char *password) {
    /*
     * TODO : Étape 3c
     * 1. Rechercher si une entrée avec le même titre existe déjà via vault_find_entry().
     *    - Si oui :
     *      - Effacer le mot de passe actuel avec sodium_memzero().
     *      - Libérer l'ancien login et l'ancien password.
     *      - Allouer et copier le nouveau login et nouveau password.
     *      - Retourner 0.
     * 2. Si l'entrée n'existe pas encore :
     *    - Vérifier si count >= capacity.
     *    - Si oui, augmenter la capacité (ex: multiplier par 2) et réallouer le tableau
     *      entries (avec realloc ou sodium_allocarray + copie manuelle sécurisée si vous voulez
     *      éviter les fuites de realloc).
     *    - Allouer et copier title, login, password pour la nouvelle entrée.
     *    - Incrémenter count.
     *    - Retourner 0.
     */
    return -1;
}

Entry *vault_find_entry(Vault *vault, const char *title) {
    /*
     * TODO : Étape 3d
     * 1. Boucler sur toutes les entrées de 0 à (vault->count - 1).
     * 2. Comparer entry->title avec le titre recherché (avec strcmp).
     * 3. Si trouvé, retourner le pointeur vers l'Entry.
     * 4. Sinon, retourner NULL.
     */
    return NULL;
}

int vault_delete_entry(Vault *vault, const char *title) {
    /*
     * TODO : Étape 3e
     * 1. Trouver l'index de l'entrée à supprimer.
     * 2. Si elle n'existe pas, retourner -1.
     * 3. Libérer proprement la mémoire de cette entrée (sodium_memzero sur password, puis free).
     * 4. Décaler toutes les entrées suivantes d'un index vers la gauche.
     * 5. Décrémenter vault->count.
     * 6. Optionnel : Réduire la capacité si count est très inférieur à capacity (ex: count < capacity/4).
     * 7. Retourner 0.
     */
    return -1;
}

int vault_serialize(const Vault *vault, unsigned char **out_buf, size_t *out_len) {
    /*
     * TODO : Étape 4a
     * 1. Calculer la taille totale nécessaire pour le buffer binaire.
     *    Taille = 1o (version) + 4o (count) + somme pour chaque entrée de :
     *      2o (titre len) + titre len + 2o (login len) + login len + 2o (pass len) + pass len.
     * 2. Allouer out_buf avec sodium_malloc(total_size).
     * 3. Écrire la version (VAULT_VERSION) et count (converti en uint32_t standard).
     * 4. Pour chaque entrée, écrire la longueur du titre (uint16_t), puis le titre lui-même,
     *    et faire de même pour login et password.
     *    (Attention à l'ordre des octets - vous pouvez utiliser htons ou écrire octet par octet).
     * 5. Mettre à jour out_len avec total_size.
     * 6. Retourner 0 en cas de succès, -1 en cas d'erreur.
     */
    return -1;
}

Vault *vault_deserialize(const unsigned char *buf, size_t len) {
    /*
     * TODO : Étape 4b
     * 1. Vérifier que la taille minimale est respectée (au moins 5 octets : version + count).
     * 2. Vérifier la version (buf[0]). Si != VAULT_VERSION, retourner NULL.
     * 3. Lire le nombre d'entrées (uint32_t à partir de buf[1]).
     * 4. Créer un Vault vide avec vault_create() et ajuster sa capacité.
     * 5. Parcourir le buffer pour extraire chaque entrée :
     *    - Lire la longueur de title (uint16_t). Vérifier qu'on ne dépasse pas les limites de 'len'.
     *    - Allouer title (longueur + 1 octet pour '\0'), copier la chaîne depuis le buffer et ajouter '\0'.
     *    - Répéter pour login et password.
     *    - Ajouter l'entrée au Vault (directement ou via vault_add_entry).
     * 6. Retourner le pointeur vers le nouveau Vault.
     */
    return NULL;
}

int vault_save(const Vault *vault, const char *filepath, const unsigned char *key, const unsigned char *salt) {
    /*
     * TODO : Étape 5c
     * 1. Sérialiser le Vault en mémoire avec vault_serialize().
     * 2. Allouer un buffer temporaire pour le ciphertext. Sa taille doit être (serialized_len + 16).
     *    (16 correspond à crypto_secretbox_MACBYTES).
     * 3. Déclarer un buffer pour le nonce (NONCE_SIZE octets).
     * 4. Chiffrer le buffer sérialisé avec encrypt_data() pour obtenir le ciphertext et le nonce.
     * 5. Créer un nom de fichier temporaire (ex: filepath + ".tmp").
     * 6. Ouvrir ce fichier en écriture binaire ("wb").
     * 7. Écrire dans le fichier :
     *    - Le salt (SALT_SIZE octets) : écrit en clair en tête de fichier.
     *    - Le nonce (NONCE_SIZE octets).
     *    - Le ciphertext (serialized_len + 16 octets).
     * 8. Fermer le fichier.
     * 9. Remplacer le fichier d'origine par le fichier temporaire avec rename().
     * 10. Libérer tous les buffers temporaires (et faire sodium_memzero sur la version sérialisée en clair).
     * 11. Retourner 0 en cas de succès, -1 en cas d'erreur.
     */
    return -1;
}

Vault *vault_load(const char *filepath, const unsigned char *key, unsigned char *out_salt) {
    /*
     * TODO : Étape 5d
     * 1. Ouvrir le fichier en lecture binaire ("rb"). Si NULL, retourner NULL.
     * 2. Obtenir la taille du fichier pour calculer la taille du ciphertext.
     *    Taille ciphertext = taille fichier - SALT_SIZE - NONCE_SIZE.
     * 3. Lire le salt (SALT_SIZE octets) et le copier dans out_salt (utile pour valider le mot de passe).
     * 4. Lire le nonce (NONCE_SIZE octets).
     * 5. Lire le ciphertext.
     * 6. Fermer le fichier.
     * 7. Allouer un buffer pour le plaintext déchiffré (taille ciphertext - 16).
     * 8. Déchiffrer avec decrypt_data(). Si le déchiffrement échoue, retourner NULL.
     * 9. Désérialiser le plaintext avec vault_deserialize() pour obtenir le Vault.
     * 10. Nettoyer les buffers temporaires (avec sodium_memzero sur le plaintext déchiffré).
     * 11. Retourner le Vault.
     */
    return NULL;
}
