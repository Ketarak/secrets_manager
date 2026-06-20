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
 * Authenticated symmetric encryption.
 * Encrypts plaintext using the derived key and writes the ciphertext to out_ciphertext.
 * out_ciphertext must be at least (plaintext_len + 16) bytes to store the Poly1305 MAC tag.
 * out_nonce receives the randomly generated 24-byte nonce.
 * Returns 0 on success, -1 on error.
 */
int encrypt_data(const unsigned char *plaintext, size_t plaintext_len,
                 const unsigned char *key,
                 unsigned char *out_ciphertext, unsigned char *out_nonce);

/*
 * Authenticated symmetric decryption.
 * Decrypts ciphertext using the key and nonce, writing the result to out_plaintext.
 * out_plaintext must be at least (ciphertext_len - 16) bytes.
 * Returns 0 on success, -1 on decryption failure (e.g. invalid MAC tag / wrong password).
 */
int decrypt_data(const unsigned char *ciphertext, size_t ciphertext_len,
                 const unsigned char *nonce, const unsigned char *key,
                 unsigned char *out_plaintext);

#endif /* CRYPTO_H */
