#include "crypto.h"
#include <sodium.h>
#include <stdio.h>
#include <string.h>

int crypto_init(void) {
    if (sodium_init() >= 0) {
        return 0;
    }
    else {
        fprintf(stderr, "Error: Failed to initialize the libsodium library.\n");
        return -1;
    }
}

int derive_key(const char *password, const unsigned char *salt, unsigned char *out_key) {
    if (!password || !salt || !out_key) {
        fprintf(stderr, "Error: Invalid input to derive_key.\n");
        return -1;
    }
    if (crypto_pwhash(out_key, KEY_SIZE, password, strlen(password), salt,
                      crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE,
                      crypto_pwhash_ALG_DEFAULT) != 0) {
        fprintf(stderr, "Error: Key derivation failed.\n");
        return -1;
    }
    return 0;
}

int encrypt_data(const unsigned char *plaintext, size_t plaintext_len,
                 const unsigned char *key,
                 unsigned char *out_ciphertext, unsigned char *out_nonce) {
    /* Encrypt data using XSalsa20-Poly1305 with a random nonce */
    if (!plaintext || !key || !out_ciphertext || !out_nonce) {
        fprintf(stderr, "Error: Invalid input to encrypt_data.\n");
        return -1;
    }
    randombytes_buf(out_nonce, NONCE_SIZE);
    if (crypto_secretbox_easy(out_ciphertext, plaintext, plaintext_len, out_nonce, key) != 0) {
        fprintf(stderr, "Error: Encryption failed.\n");
        return -1;
    }
    return 0;
}

int decrypt_data(const unsigned char *ciphertext, size_t ciphertext_len,
                 const unsigned char *nonce, const unsigned char *key,
                 unsigned char *out_plaintext) {
    /* Decrypt data and verify its integrity tag */
    if (!ciphertext || !nonce || !key || !out_plaintext) {
        fprintf(stderr, "Error: Invalid input to decrypt_data.\n");
        return -1;
    }
    if (crypto_secretbox_open_easy(out_plaintext, ciphertext, ciphertext_len, nonce, key) != 0) {
        fprintf(stderr, "Error: Decryption failed.\n");
        return -1;
    }
    return 0;
}
