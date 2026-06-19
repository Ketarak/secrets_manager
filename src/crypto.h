#ifndef CRYPTO_H
#define CRYPTO_H

#include <stddef.h>

/* Constants matching libsodium recommendations */
#define KEY_SIZE 32        /* crypto_secretbox_KEYBYTES */
#define NONCE_SIZE 24      /* crypto_secretbox_NONCEBYTES */
#define SALT_SIZE 16       /* crypto_pwhash_SALTBYTES */

int crypto_init(void);

int derive_key(const char *password, const unsigned char *salt, unsigned char *out_key);

/*
 * Étape 5a : Chiffrement symétrique authentifié.
 * Chiffre le plaintext sérialisé avec la clé dérivée et écrit le résultat dans out_ciphertext.
 * out_ciphertext doit avoir une taille de (plaintext_len + 16) pour stocker le tag Poly1305.
 * out_nonce recevra le nonce de 24 octets généré aléatoirement.
 * Retourne 0 en cas de succès, -1 en cas d'erreur.
 * Indice :
 *   - Générer le nonce avec randombytes_buf().
 *   - Chiffrer avec crypto_secretbox_easy().
 */
int encrypt_data(const unsigned char *plaintext, size_t plaintext_len,
                 const unsigned char *key,
                 unsigned char *out_ciphertext, unsigned char *out_nonce);

/*
 * Étape 5b : Déchiffrement symétrique authentifié.
 * Déchiffre le ciphertext à l'aide de la clé et du nonce, et écrit le résultat dans out_plaintext.
 * out_plaintext doit avoir une taille de (ciphertext_len - 16).
 * Retourne 0 en cas de succès, -1 en cas d'erreur (ex: tag Poly1305 invalide, mauvais mot de passe).
 * Indice : utiliser crypto_secretbox_open_easy().
 */
int decrypt_data(const unsigned char *ciphertext, size_t ciphertext_len,
                 const unsigned char *nonce, const unsigned char *key,
                 unsigned char *out_plaintext);

#endif /* CRYPTO_H */
