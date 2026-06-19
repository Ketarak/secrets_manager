#include "crypto.h"
#include "vault.h"
#include "native.h"
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
    return -1;
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
    printf("Secrets Manager - Squelette initialisé.\n");
    return 0;
}
