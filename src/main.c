#include "crypto.h"
#include "vault.h"
#include "native.h"
#include <sodium.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <termios.h>

/*
 * Étape 6b : Saisie du mot de passe maître sans écho terminal.
 * Désactive l'affichage (ECHO) sur le terminal pour saisir le mot de passe de façon sécurisée.
 * Retourne 0 en cas de succès, -1 en cas d'erreur.
 */
static int read_password(char *buf, size_t max_len) {
    /*
     * TODO : Saisie sécurisée sans écho
     * 1. Déclarer struct termios old_t, new_t.
     * 2. Récupérer les attributs actuels du terminal avec tcgetattr(STDIN_FILENO, &old_t).
     * 3. Copier old_t dans new_t : new_t = old_t.
     * 4. Désactiver l'écho en désactivant le flag ECHO : new_t.c_lflag &= ~ECHO.
     * 5. Appliquer les nouveaux attributs avec tcsetattr(STDIN_FILENO, TCSANOW, &new_t).
     * 6. Lire le mot de passe depuis stdin en utilisant fgets() ou similaire.
     * 7. Supprimer le retour à la ligne '\n' à la fin de la saisie si présent.
     * 8. Rétablir les attributs originaux avec tcsetattr(STDIN_FILENO, TCSANOW, &old_t).
     * 9. Imprimer un saut de ligne printf("\n") pour que le prompt suivant commence sur une nouvelle ligne.
     * 10. Retourner 0 en cas de succès.
     */
    struct termios old_t, new_t;
    if (tcgetattr(STDIN_FILENO, &old_t) != 0) {
        fprintf(stderr, "Error: Failed to get terminal attributes.\n");
        return -1;
    }
    new_t = old_t;
    new_t.c_lflag &= ~ECHO;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_t) != 0) {
        fprintf(stderr, "Error: Failed to set terminal attributes.\n");
        return -1;
    }
    if (fgets(buf, max_len, stdin) == NULL) {
        fprintf(stderr, "Error: Failed to read password.\n");
        tcsetattr(STDIN_FILENO, TCSANOW, &old_t);
        return -1;
    }
    // Remove the trailing newline if present
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    }
    // Restore original terminal attributes
    if (tcsetattr(STDIN_FILENO, TCSANOW, &old_t) != 0) {
        fprintf(stderr, "Error: Failed to restore terminal attributes.\n");
        return -1;
    }
    printf("\n");
    return 0;
}

int main(int argc, char *argv[]) {
    /*
     * TODO : Étape 6a - Initialisation et parsing CLI
     * 
     * 1. Appeler crypto_init() pour initialiser libsodium.
     * 
     * 2. Parser les options avec getopt_long ou getopt standard :
     *    -f <path> : spécifier le chemin du fichier coffre (par défaut: "vault.enc")
     *    -n / --native : lancer en mode Native Messaging Firefox
     *    -h / --help : afficher l'aide
     * 
     * 3. Si le mode Native Messaging est spécifié (-n) :
     *    - Appeler native_messaging_loop(filepath).
     *    - Retourner le code de retour de la boucle.
     * 
     * 4. Sinon, analyser la commande demandée (add, get, list, delete) :
     *    
     *    a. Commande "list" :
     *       - Demander le mot de passe maître.
     *       - Charger le coffre (si le fichier n'existe pas, initialiser un nouveau Vault).
     *         - Pour charger : lire le salt en tête, dériver la clé, appeler vault_load.
     *       - Afficher la liste des titres d'entrées (ex: "1. google.com\n2. github.com\n").
     *       - Libérer le Vault.
     * 
     *    b. Commande "get <title>" :
     *       - Demander le mot de passe maître.
     *       - Charger le coffre.
     *       - Chercher l'entrée avec le titre.
     *       - Si trouvée, afficher le login et le mot de passe.
     *         (Optionnel : copier le mot de passe dans le presse-papier et lancer un timer d'auto-clear).
     *       - Libérer le Vault.
     * 
     *    c. Commande "add <title>" :
     *       - Demander le mot de passe maître.
     *       - Charger le coffre. Si nouveau coffre :
     *         - Générer un nouveau salt aléatoire avec randombytes_buf().
     *         - Dériver la clé.
     *       - Demander à l'utilisateur de saisir le login.
     *       - Demander à l'utilisateur de saisir le mot de passe (ou proposer une génération aléatoire).
     *       - Ajouter ou modifier l'entrée dans le coffre (vault_add_entry).
     *       - Sauvegarder le coffre (vault_save).
     *       - Libérer le Vault.
     * 
     *    d. Commande "delete <title>" :
     *       - Demander le mot de passe maître.
     *       - Charger le coffre.
     *       - Supprimer l'entrée (vault_delete_entry).
     *       - Sauvegarder le coffre (vault_save).
     *       - Libérer le Vault.
     * 
     * 5. S'assurer de nettoyer toutes les clés et variables sensibles avec sodium_memzero avant de quitter.
     */
    const char *filepath = "vault.enc"; // Default vault file path
    int native_mode = 0;

    crypto_init();
    struct option long_options[] = {
        {"file", required_argument, 0, 'f'},
        {"native", no_argument, 0, 'n'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int c;
    while ((c = getopt_long(argc, argv, "f:nh", long_options, NULL)) != -1) {
        switch (c) {
            case 'f':
                filepath = optarg;
                break;
            case 'n':
                native_mode = 1;
                break;
            case 'h':
                printf("Usage: %s [options] <command> [args]\n", argv[0]);
                printf("Options:\n");
                printf("  -f <path>       Specify the vault file path (default: vault.enc)\n");
                printf("  -n, --native    Run in Native Messaging mode for Firefox\n");
                printf("  -h, --help      Show this help message\n");
                return 0;            
            default:
                fprintf(stderr, "Usage: %s [options]\n", argv[0]);
                return 1;
        }
    }

    if (native_mode) {
        return native_messaging_loop(filepath);
    }

    // 1. Initialisation des variables globales de session
    Vault *vault = NULL;
    unsigned char salt[SALT_SIZE];
    unsigned char key[KEY_SIZE];

    // 2. Détection et déverrouillage / création du coffre
    // On utilise access() de <unistd.h> pour tester la présence du fichier
    if (access(filepath, F_OK) == 0) {
        // Le fichier existe, on le déverrouille
        char master_password[256];
        printf("Vault file found. Enter master password to unlock: ");
        fflush(stdout);
        if (read_password(master_password, sizeof(master_password)) != 0) {
            return 1;
        }

        // On lit d'abord le Salt en tête de fichier
        FILE *f = fopen(filepath, "rb");
        if (!f) {
            fprintf(stderr, "Error: Failed to open vault file for salt extraction.\n");
            sodium_memzero(master_password, sizeof(master_password));
            return 1;
        }
        if (fread(salt, 1, SALT_SIZE, f) != SALT_SIZE) {
            fprintf(stderr, "Error: Failed to read salt from vault.\n");
            fclose(f);
            sodium_memzero(master_password, sizeof(master_password));
            return 1;
        }
        fclose(f);

        // Dérive la clé maître à partir du mot de passe saisi et du salt extrait
        if (derive_key(master_password, salt, key) != 0) {
            sodium_memzero(master_password, sizeof(master_password));
            return 1;
        }
        // Sécurité : effacer le mot de passe maître en clair immédiatement !
        sodium_memzero(master_password, sizeof(master_password));

        // Charge et déchiffre le coffre
        unsigned char dummy_salt[SALT_SIZE];
        vault = vault_load(filepath, key, dummy_salt);
        if (!vault) {
            fprintf(stderr, "Error: Decryption failed (incorrect password or corrupted file).\n");
            sodium_memzero(key, sizeof(key));
            return 1;
        }
        printf("[+] Vault unlocked successfully.\n");

    } else {
        // Le fichier n'existe pas, on crée un nouveau coffre
        char master_password[256];
        char confirm_password[256];

        printf("No vault found at '%s'. Let's create a new one.\n", filepath);
        
        printf("Choose a master password: ");
        fflush(stdout);
        if (read_password(master_password, sizeof(master_password)) != 0) {
            return 1;
        }
        
        printf("Confirm master password: ");
        fflush(stdout);
        if (read_password(confirm_password, sizeof(confirm_password)) != 0) {
            sodium_memzero(master_password, sizeof(master_password));
            return 1;
        }

        if (strcmp(master_password, confirm_password) != 0) {
            fprintf(stderr, "Error: Passwords do not match.\n");
            sodium_memzero(master_password, sizeof(master_password));
            sodium_memzero(confirm_password, sizeof(confirm_password));
            return 1;
        }
        // Sécurité : effacer la confirmation
        sodium_memzero(confirm_password, sizeof(confirm_password));

        // Génère un nouveau sel aléatoire sécurisé
        randombytes_buf(salt, SALT_SIZE);

        // Dérive la clé maître
        if (derive_key(master_password, salt, key) != 0) {
            sodium_memzero(master_password, sizeof(master_password));
            return 1;
        }
        sodium_memzero(master_password, sizeof(master_password));

        // Crée la structure vide en RAM
        vault = vault_create();
        if (!vault) {
            fprintf(stderr, "Error: Failed to create new vault structure.\n");
            sodium_memzero(key, sizeof(key));
            return 1;
        }

        // Sauvegarde immédiatement pour créer le fichier sur le disque
        if (vault_save(vault, filepath, key, salt) != 0) {
            fprintf(stderr, "Error: Failed to initialize vault file.\n");
            vault_free(vault);
            sodium_memzero(key, sizeof(key));
            return 1;
        }
        printf("[+] Created new vault file successfully.\n");
    }

    // 3. Boucle interactive (REPL)
    char cmd_line[1024];
    printf("\n=== Secrets Manager Console ===\n");
    printf("Type 'help' to list commands, 'exit' to quit.\n\n");

    while (1) {
        printf("vault> ");
        fflush(stdout);

        if (fgets(cmd_line, sizeof(cmd_line), stdin) == NULL) {
            printf("\n");
            break; // Arrêt propre sur Ctrl+D (EOF)
        }

        // Nettoyage du saut de ligne final
        size_t len = strlen(cmd_line);
        if (len > 0 && cmd_line[len - 1] == '\n') {
            cmd_line[len - 1] = '\0';
        }

        // Trim des espaces de début de commande
        char *p = cmd_line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0') continue; // Ligne vide

        // Découpage de la commande et de son argument (format: <commande> [argument])
        char command[256] = "";
        char arg[768] = "";
        int num = sscanf(p, "%255s %767[^\n]", command, arg);
        if (num <= 0) continue;

        if (strcmp(command, "help") == 0) {
            printf("Available commands:\n");
            printf("  list             List all secret titles\n");
            printf("  show <title>     Show details of a secret\n");
            printf("  add <title>      Add a new secret interactively\n");
            printf("  delete <title>   Delete a secret\n");
            printf("  destroy          Delete the vault file from disk and quit\n");
            printf("  exit / quit      Wipe RAM and quit\n");

        } else if (strcmp(command, "list") == 0) {
            if (vault->count == 0) {
                printf("Vault is empty.\n");
            } else {
                printf("Secrets in vault (%zu) :\n", vault->count);
                for (size_t i = 0; i < vault->count; i++) {
                    printf("  - %s (type: %s, fields: %zu)\n", 
                           vault->entries[i].title, 
                           vault->entries[i].type ? vault->entries[i].type : "none",
                           vault->entries[i].count);
                }
            }

        } else if (strcmp(command, "show") == 0 || strcmp(command, "get") == 0) {
            if (num < 2) {
                printf("Usage: show <title>\n");
                continue;
            }
            Entry *entry = vault_find_entry(vault, arg);
            if (!entry) {
                printf("Error: Secret '%s' not found.\n", arg);
            } else {
                printf("Secret '%s' (type: %s) :\n", entry->title, entry->type ? entry->type : "none");
                for (size_t j = 0; j < entry->count; j++) {
                    Field *field = &entry->fields[j];
                    if (field->is_sensitive) {
                        printf("  - %s (sensitive) : %s\n", field->name, field->value);
                    } else {
                        printf("  - %s : %s\n", field->name, field->value);
                    }
                }
            }

        } else if (strcmp(command, "add") == 0) {
            if (num < 2) {
                printf("Usage: add <title>\n");
                continue;
            }
            if (vault_find_entry(vault, arg) != NULL) {
                printf("Error: Secret '%s' already exists.\n", arg);
                continue;
            }

            char type[256] = "";
            printf("Enter secret type (login, ssh, card, note) [default: login]: ");
            fflush(stdout);
            if (fgets(type, sizeof(type), stdin) == NULL) continue;
            size_t t_len = strlen(type);
            if (t_len > 0 && type[t_len - 1] == '\n') type[t_len - 1] = '\0';
            if (strlen(type) == 0) {
                strcpy(type, "login");
            }

            Entry *entry = vault_add_entry(vault, arg, type);
            if (!entry) {
                printf("Error: Failed to create entry.\n");
                continue;
            }

            printf("Enter fields for '%s'. Leave field name empty to finish.\n", arg);
            while (1) {
                char f_name[256] = "";
                printf("  Field name: ");
                fflush(stdout);
                if (fgets(f_name, sizeof(f_name), stdin) == NULL) break;
                size_t fn_len = strlen(f_name);
                if (fn_len > 0 && f_name[fn_len - 1] == '\n') f_name[fn_len - 1] = '\0';
                if (strlen(f_name) == 0) break; // Fin de saisie

                char f_val[2048] = "";
                printf("  Field value: ");
                fflush(stdout);
                if (fgets(f_val, sizeof(f_val), stdin) == NULL) {
                    sodium_memzero(f_val, sizeof(f_val));
                    break;
                }
                size_t fv_len = strlen(f_val);
                if (fv_len > 0 && f_val[fv_len - 1] == '\n') f_val[fv_len - 1] = '\0';

                char sens[16] = "";
                printf("  Is this field sensitive (y/n)? [n]: ");
                fflush(stdout);
                if (fgets(sens, sizeof(sens), stdin) == NULL) {
                    sodium_memzero(f_val, sizeof(f_val));
                    sodium_memzero(f_name, sizeof(f_name));
                    sodium_memzero(sens, sizeof(sens));                 
                    break;
                }
                int is_sens = (sens[0] == 'y' || sens[0] == 'Y') ? 1 : 0;

                if (entry_set_field(entry, f_name, f_val, is_sens) != 0) {
                    printf("  Error: Failed to set field.\n");
                }

                // Sécurité : effacer la valeur locale du mot de passe s'il est sensible
                if (is_sens) {
                    sodium_memzero(f_val, sizeof(f_val));
                }
                sodium_memzero(f_val, sizeof(f_val));
                sodium_memzero(f_name, sizeof(f_name));
            }

            // Sauvegarde automatique
            if (vault_save(vault, filepath, key, salt) == 0) {
                printf("[+] Secret '%s' added and vault saved.\n", arg);
            } else {
                printf("Error: Failed to auto-save changes.\n");
            }

        } else if (strcmp(command, "delete") == 0) {
            if (num < 2) {
                printf("Usage: delete <title>\n");
                continue;
            }
            if (vault_delete_entry(vault, arg) != 0) {
                printf("Error: Secret '%s' not found.\n", arg);
            } else {
                // Sauvegarde automatique après suppression
                if (vault_save(vault, filepath, key, salt) == 0) {
                    printf("[+] Secret '%s' deleted and vault saved.\n", arg);
                } else {
                    printf("Error: Failed to auto-save changes.\n");
                }
            }

        } else if (strcmp(command, "destroy") == 0) {
            char confirm[16] = "";
            printf("WARNING: This will permanently delete the vault file '%s' and all its contents!\n", filepath);
            printf("Are you absolutely sure you want to proceed? (type 'yes' to confirm): ");
            fflush(stdout);
            if (fgets(confirm, sizeof(confirm), stdin) == NULL) {
                sodium_memzero(confirm, sizeof(confirm));
                continue;
            }
            size_t conf_len = strlen(confirm);
            if (conf_len > 0 && confirm[conf_len - 1] == '\n') {
                confirm[conf_len - 1] = '\0';
            }
            if (strcmp(confirm, "yes") == 0) {
                if (remove(filepath) == 0) {
                    printf("[+] Vault file successfully deleted from disk.\n");
                    sodium_memzero(confirm, sizeof(confirm));
                    break;
                } else {
                    perror("Error: Failed to delete vault file");
                }
            } else {
                printf("Action cancelled.\n");
            }
            sodium_memzero(confirm, sizeof(confirm));

        } else if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0) {
            printf("Locking vault and exiting. Goodbye!\n");
            break;

        } else {
            printf("Unknown command: '%s'. Type 'help' for a list.\n", command);
        }
    }

    // 4. Nettoyage de fin de session en RAM
    vault_free(vault);
    sodium_memzero(key, sizeof(key));
    printf("[+] Session RAM wiped cleanly.\n");

    return 0;
}
