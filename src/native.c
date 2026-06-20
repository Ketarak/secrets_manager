#include "native.h"
#include "vault.h"
#include "crypto.h"
#include <sodium.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* Extract a string value for a key in a flat JSON structure, decoding escapes */
static int get_json_string_value(const char *json, const char *key, char *out_val, size_t max_len) {
    char key_pattern[256];
    snprintf(key_pattern, sizeof(key_pattern), "\"%s\"", key);
    
    const char *p = strstr(json, key_pattern);
    if (!p) return -1;
    
    p += strlen(key_pattern);
    p = strchr(p, ':');
    if (!p) return -1;
    p++;
    
    // Skip spaces
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    
    // Expect string starting quote
    if (*p != '"') return -1;
    p++;
    
    const char *start = p;
    size_t len = 0;
    
    // Find unescaped closing quote
    while (*p != '\0') {
        if (*p == '"') {
            int backslashes = 0;
            const char *b = p - 1;
            while (b >= start && *b == '\\') {
                backslashes++;
                b--;
            }
            if (backslashes % 2 == 0) {
                break;
            }
        }
        len++;
        p++;
    }
    
    if (*p != '"') return -1;
    
    // Copy and decode escape sequences
    size_t d = 0;
    for (size_t s = 0; s < len && d < max_len - 1; s++) {
        if (start[s] == '\\') {
            s++;
            if (s >= len) break;
            switch (start[s]) {
                case '"':  out_val[d++] = '"';  break;
                case '\\': out_val[d++] = '\\'; break;
                case '/':  out_val[d++] = '/';  break;
                case 'b':  out_val[d++] = '\b'; break;
                case 'f':  out_val[d++] = '\f'; break;
                case 'n':  out_val[d++] = '\n'; break;
                case 'r':  out_val[d++] = '\r'; break;
                case 't':  out_val[d++] = '\t'; break;
                default:   out_val[d++] = start[s]; break;
            }
        } else {
            out_val[d++] = start[s];
        }
    }
    out_val[d] = '\0';
    return 0;
}

/* Escape special characters of a string for standard JSON output */
static void escape_json_string(const char *src, char *dst, size_t max_len) {
    size_t d = 0;
    for (size_t s = 0; src[s] != '\0' && d < max_len - 4; s++) {
        switch (src[s]) {
            case '"':
                dst[d++] = '\\'; dst[d++] = '"';
                break;
            case '\\':
                dst[d++] = '\\'; dst[d++] = '\\';
                break;
            case '\n':
                dst[d++] = '\\'; dst[d++] = 'n';
                break;
            case '\r':
                dst[d++] = '\\'; dst[d++] = 'r';
                break;
            case '\t':
                dst[d++] = '\\'; dst[d++] = 't';
                break;
            default:
                if ((unsigned char)src[s] < 0x20) {
                    d += snprintf(dst + d, max_len - d, "\\u%04x", (unsigned char)src[s]);
                } else {
                    dst[d++] = src[s];
                }
                break;
        }
    }
    dst[d] = '\0';
}

int native_messaging_loop(const char *filepath) {
    Vault *vault = NULL;
    unsigned char salt[SALT_SIZE];
    unsigned char key[KEY_SIZE];
    int is_unlocked = 0;

    // Endless messaging loop
    while (1) {
        uint32_t length = 0;
        
        // 1. Read message length (4 bytes, native endianness)
        if (fread(&length, 1, 4, stdin) != 4) {
            break; // Connection closed by Firefox (normal EOF)
        }

        // Safety limit: reject abnormally large messages (e.g. > 1 MB)
        if (length > 1024 * 1024) {
            fprintf(stderr, "[Native] Error: Message too large (%u bytes).\n", length);
            break;
        }

        // 2. Allocate buffer for incoming JSON payload
        char *json_buf = (char *)sodium_malloc(length + 1);
        if (!json_buf) {
            fprintf(stderr, "[Native] Error: Memory allocation failure.\n");
            break;
        }

        if (fread(json_buf, 1, length, stdin) != length) {
            fprintf(stderr, "[Native] Error: Failed to read complete message body.\n");
            sodium_free(json_buf);
            break;
        }
        json_buf[length] = '\0';

        // 3. Extract the requested action
        char action[64] = "";
        get_json_string_value(json_buf, "action", action, sizeof(action));

        char response[8192] = "";

        // 4. Handle incoming requests
        if (strcmp(action, "unlock") == 0) {
            char password[256] = "";
            get_json_string_value(json_buf, "password", password, sizeof(password));

            if (is_unlocked) {
                snprintf(response, sizeof(response), "{\"status\":\"success\",\"message\":\"Already unlocked\"}");
            } else {
                FILE *f = fopen(filepath, "rb");
                if (!f) {
                    snprintf(response, sizeof(response), "{\"status\":\"error\",\"message\":\"Vault file not found\"}");
                } else {
                    if (fread(salt, 1, SALT_SIZE, f) != SALT_SIZE) {
                        snprintf(response, sizeof(response), "{\"status\":\"error\",\"message\":\"Failed to read salt\"}");
                        fclose(f);
                    } else {
                        fclose(f);
                        if (derive_key(password, salt, key) != 0) {
                            snprintf(response, sizeof(response), "{\"status\":\"error\",\"message\":\"Key derivation failed\"}");
                        } else {
                            unsigned char dummy_salt[SALT_SIZE];
                            vault = vault_load(filepath, key, dummy_salt);
                            if (!vault) {
                                snprintf(response, sizeof(response), "{\"status\":\"error\",\"message\":\"Incorrect password\"}");
                                sodium_memzero(key, sizeof(key));
                            } else {
                                is_unlocked = 1;
                                snprintf(response, sizeof(response), "{\"status\":\"success\",\"message\":\"Vault unlocked\"}");
                            }
                        }
                    }
                }
            }
            sodium_memzero(password, sizeof(password));

        } else if (strcmp(action, "lock") == 0) {
            if (is_unlocked) {
                vault_free(vault);
                vault = NULL;
                sodium_memzero(key, sizeof(key));
                is_unlocked = 0;
                snprintf(response, sizeof(response), "{\"status\":\"success\",\"message\":\"Vault locked\"}");
            } else {
                snprintf(response, sizeof(response), "{\"status\":\"error\",\"message\":\"Vault is already locked\"}");
            }

        } else if (strcmp(action, "status") == 0) {
            if (is_unlocked) {
                snprintf(response, sizeof(response), "{\"status\":\"unlocked\"}");
            } else {
                snprintf(response, sizeof(response), "{\"status\":\"locked\"}");
            }

        } else if (strcmp(action, "list") == 0) {
            if (!is_unlocked) {
                snprintf(response, sizeof(response), "{\"status\":\"error\",\"message\":\"Vault is locked\"}");
            } else {
                size_t resp_cap = 16384;
                char *dyn_response = (char *)sodium_malloc(resp_cap);
                if (!dyn_response) {
                    snprintf(response, sizeof(response), "{\"status\":\"error\",\"message\":\"Memory allocation error\"}");
                } else {
                    size_t offset = snprintf(dyn_response, resp_cap, "{\"status\":\"success\",\"secrets\":[");
                    for (size_t i = 0; i < vault->count; i++) {
                        char escaped_title[512] = "";
                        escape_json_string(vault->entries[i].title, escaped_title, sizeof(escaped_title));
                        offset += snprintf(dyn_response + offset, resp_cap - offset,
                                           "%s{\"title\":\"%s\",\"type\":\"%s\"}",
                                           i > 0 ? "," : "",
                                           escaped_title,
                                           vault->entries[i].type ? vault->entries[i].type : "none");
                    }
                    snprintf(dyn_response + offset, resp_cap - offset, "]}");
                    
                    uint32_t resp_len = strlen(dyn_response);
                    fwrite(&resp_len, 1, 4, stdout);
                    fwrite(dyn_response, 1, resp_len, stdout);
                    fflush(stdout);
                    
                    sodium_free(dyn_response);
                    response[0] = '\0'; // Already sent
                }
            }

        } else if (strcmp(action, "get") == 0) {
            if (!is_unlocked) {
                snprintf(response, sizeof(response), "{\"status\":\"error\",\"message\":\"Vault is locked\"}");
            } else {
                char title[256] = "";
                get_json_string_value(json_buf, "title", title, sizeof(title));
                
                Entry *entry = vault_find_entry(vault, title);
                if (!entry) {
                    snprintf(response, sizeof(response), "{\"status\":\"error\",\"message\":\"Secret not found\"}");
                } else {
                    size_t resp_cap = 16384;
                    char *dyn_response = (char *)sodium_malloc(resp_cap);
                    if (!dyn_response) {
                        snprintf(response, sizeof(response), "{\"status\":\"error\",\"message\":\"Memory allocation error\"}");
                    } else {
                        size_t offset = snprintf(dyn_response, resp_cap, 
                                                 "{\"status\":\"success\",\"title\":\"%s\",\"type\":\"%s\",\"fields\":[",
                                                 entry->title, entry->type ? entry->type : "none");
                        
                        for (size_t j = 0; j < entry->count; j++) {
                            Field *field = &entry->fields[j];
                            char escaped_name[512] = "";
                            size_t esc_val_cap = strlen(field->value) * 6 + 1;
                            char *escaped_value = (char *)sodium_malloc(esc_val_cap);
                            
                            if (escaped_value) {
                                escape_json_string(field->name, escaped_name, sizeof(escaped_name));
                                escape_json_string(field->value, escaped_value, esc_val_cap);
                                
                                offset += snprintf(dyn_response + offset, resp_cap - offset,
                                                   "%s{\"name\":\"%s\",\"value\":\"%s\",\"is_sensitive\":%s}",
                                                   j > 0 ? "," : "",
                                                   escaped_name, escaped_value,
                                                   field->is_sensitive ? "true" : "false");
                                
                                sodium_memzero(escaped_value, esc_val_cap);
                                sodium_free(escaped_value);
                            }
                        }
                        
                        snprintf(dyn_response + offset, resp_cap - offset, "]}");
                        
                        uint32_t resp_len = strlen(dyn_response);
                        fwrite(&resp_len, 1, 4, stdout);
                        fwrite(dyn_response, 1, resp_len, stdout);
                        fflush(stdout);
                        
                        sodium_free(dyn_response);
                        response[0] = '\0'; // Already sent
                    }
                }
            }
        } else {
            snprintf(response, sizeof(response), "{\"status\":\"error\",\"message\":\"Unknown action\"}");
        }

        sodium_free(json_buf);

        // 5. Send static response if not dynamically sent
        if (response[0] != '\0') {
            uint32_t resp_len = strlen(response);
            fwrite(&resp_len, 1, 4, stdout);
            fwrite(response, 1, resp_len, stdout);
            fflush(stdout);
        }
    }

    // Secure memory cleanup on loop exit
    if (is_unlocked && vault) {
        vault_free(vault);
        sodium_memzero(key, sizeof(key));
    }

    return 0;
}