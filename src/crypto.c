#include "crypto.h"
#include <sodium.h>
#include <stdio.h>
#include <string.h>

int crypto_init(void) {
    /*
     * TODO : Étape 1
     * 1. Appeler sodium_init()
     * 2. Si sodium_init() renvoie -1, afficher une erreur sur stderr et retourner -1.
     * 3. Si sodium_init() réussit (retourne >= 0, ou 1 si déjà initialisé), retourner 0.
     */
    return -1;
}

int derive_key(const char *password, const unsigned char *salt, unsigned char *out_key) {
    /*
     * TODO : Étape 2
     * 1. Valider les entrées (password != NULL, salt != NULL, out_key != NULL).
     * 2. Appeler crypto_pwhash() :
     *    - out : out_key
     *    - outlen : KEY_SIZE
     *    - passwd : password
     *    - passwdlen : strlen(password)
     *    - salt : salt
     *    - opslimit : crypto_pwhash_OPSLIMIT_INTERACTIVE
     *    - memlimit : crypto_pwhash_MEMLIMIT_INTERACTIVE
     *    - alg : crypto_pwhash_ALG_DEFAULT
     * 3. Vérifier le retour de crypto_pwhash(). Si != 0, retourner -1 (erreur).
     * 4. Sinon, retourner 0.
     */
    return -1;
}

int encrypt_data(const unsigned char *plaintext, size_t plaintext_len,
                 const unsigned char *key,
                 unsigned char *out_ciphertext, unsigned char *out_nonce) {
    /*
     * TODO : Étape 5a
     * 1. Générer un nonce aléatoire avec randombytes_buf(out_nonce, NONCE_SIZE).
     * 2. Appeler crypto_secretbox_easy() :
     *    - c : out_ciphertext
     *    - m : plaintext
     *    - mlen : plaintext_len
     *    - n : out_nonce
     *    - k : key
     * 3. Si l'appel échoue (retourne != 0), retourner -1.
     * 4. Sinon, retourner 0.
     */
    return -1;
}

int decrypt_data(const unsigned char *ciphertext, size_t ciphertext_len,
                 const unsigned char *nonce, const unsigned char *key,
                 unsigned char *out_plaintext) {
    /*
     * TODO : Étape 5b
     * 1. Appeler crypto_secretbox_open_easy() :
     *    - m : out_plaintext
     *    - c : ciphertext
     *    - clen : ciphertext_len
     *    - n : nonce
     *    - k : key
     * 2. Si l'appel échoue (retourne != 0, ce qui signifie que le tag est invalide,
     *    donc soit le mot de passe est faux, soit le fichier a été modifié/corrompu),
     *    retourner -1.
     * 3. Sinon, retourner 0.
     */
    return -1;
}
