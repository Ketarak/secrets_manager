#include "../src/crypto.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sodium.h>

void test_crypto_init() {
    printf("Running test_crypto_init...\n");
    int res = crypto_init();
    assert(res == 0);
    printf("test_crypto_init passed.\n\n");
}

void test_key_derivation() {
    printf("Running test_key_derivation...\n");
    
    char password[] = "SuperSafePassword123!";
    unsigned char salt[SALT_SIZE];
    unsigned char key1[KEY_SIZE];
    unsigned char key2[KEY_SIZE];
    unsigned char key_diff[KEY_SIZE];
    
    // Generate a random salt
    randombytes_buf(salt, sizeof(salt));
    
    // Derive key 1
    int res1 = derive_key(password, salt, key1);
    assert(res1 == 0);
    
    // Derive key 2 with same password and salt (should be identical)
    int res2 = derive_key(password, salt, key2);
    assert(res2 == 0);
    assert(memcmp(key1, key2, KEY_SIZE) == 0);
    
    // Derive key with different password
    int res_diff = derive_key("AnotherPassword", salt, key_diff);
    assert(res_diff == 0);
    assert(memcmp(key1, key_diff, KEY_SIZE) != 0);
    
    printf("test_key_derivation passed.\n\n");
}

void test_encryption_roundtrip() {
    printf("Running test_encryption_roundtrip...\n");
    
    unsigned char key[KEY_SIZE];
    unsigned char salt[SALT_SIZE];
    char password[] = "SessionPassword";
    
    randombytes_buf(salt, sizeof(salt));
    int res_k = derive_key(password, salt, key);
    assert(res_k == 0);
    
    const char *plaintext = "This is a confidential secret entry text!";
    size_t plain_len = strlen(plaintext);
    
    // Ciphertext must be plain_len + 16 (MAC tag)
    size_t cipher_len = plain_len + 16;
    unsigned char *ciphertext = malloc(cipher_len);
    unsigned char nonce[NONCE_SIZE];
    assert(ciphertext != NULL);
    
    // Encrypt
    int res_enc = encrypt_data((const unsigned char *)plaintext, plain_len, key, ciphertext, nonce);
    assert(res_enc == 0);
    
    // Decrypt
    unsigned char *decrypted = malloc(plain_len + 1);
    assert(decrypted != NULL);
    
    int res_dec = decrypt_data(ciphertext, cipher_len, nonce, key, decrypted);
    assert(res_dec == 0);
    decrypted[plain_len] = '\0';
    
    assert(strcmp(plaintext, (char *)decrypted) == 0);
    
    // Test decryption failure with wrong key
    unsigned char wrong_key[KEY_SIZE];
    randombytes_buf(wrong_key, sizeof(wrong_key));
    unsigned char *decrypted_wrong = malloc(plain_len + 1);
    assert(decrypted_wrong != NULL);
    
    int res_dec_wrong = decrypt_data(ciphertext, cipher_len, nonce, wrong_key, decrypted_wrong);
    assert(res_dec_wrong == -1); // Must fail
    
    // Test decryption failure with tampered ciphertext
    ciphertext[0] ^= 0xFF; // Flip first byte
    int res_dec_tampered = decrypt_data(ciphertext, cipher_len, nonce, key, decrypted_wrong);
    assert(res_dec_tampered == -1); // Must fail
    
    free(ciphertext);
    free(decrypted);
    free(decrypted_wrong);
    
    printf("test_encryption_roundtrip passed.\n\n");
}

int main() {
    printf("=== START CRYPTO UNIT TESTS ===\n");
    
    test_crypto_init();
    test_key_derivation();
    test_encryption_roundtrip();
    
    printf("=== ALL CRYPTO UNIT TESTS PASSED ===\n");
    return 0;
}
