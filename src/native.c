#include "native.h"
#include "vault.h"
#include "crypto.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

int native_messaging_loop(const char *filepath) {
    /*
     * TODO : Phase 2 - Boucle Native Messaging
     * 
     * Cette boucle doit tourner indéfiniment tant que stdin est ouvert.
     * 
     * 1. Configurer stdin et stdout en mode binaire si nécessaire (sur Windows, 
     *    mais sur Linux ce n'est pas strictement indispensable, bien qu'il faille
     *    éviter toute interférence de printf classiques).
     * 
     * 2. Dans une boucle while(1) :
     *    a. Lire 4 octets de stdin représentant la taille du message (uint32_t length).
     *       - Si fread renvoie 0 ou < 4, cela signifie que la connexion est coupée (EOF).
     *         C'est l'arrêt normal du programme. Sortir de la boucle proprement.
     *    b. Allouer un buffer temporaire pour le message de taille (length + 1) octets.
     *       Utiliser sodium_malloc pour les buffers de messages sensibles si nécessaire.
     *    c. Lire 'length' octets depuis stdin dans ce buffer.
     *       - Si la lecture échoue ou est incomplète, libérer le buffer et retourner -1.
     *    d. Ajouter un caractère de fin de chaîne '\0' pour pouvoir manipuler le message comme une string.
     *    e. Parser le JSON (ex: extraire le champ "action", "title", "password", etc.).
     *       - Conseil : utiliser une lib JSON simple (cJSON, jsmn) ou écrire un parser minimaliste 
     *         spécifique à notre besoin si on veut éviter les dépendances externes.
     *    f. Traiter la requête en fonction de l'action :
     *       - Action "unlock" :
     *         - Dériver la clé à partir du mot de passe reçu.
     *         - Tenter de charger et déchiffrer le Vault (vault_load).
     *         - Si succès, garder le Vault déchiffré en RAM et renvoyer un JSON de succès {"status": "unlocked"}.
     *         - Si échec, renvoyer {"status": "error", "message": "Wrong password"}.
     *       - Action "get" :
     *         - Si le Vault n'est pas déverrouillé, renvoyer {"status": "error", "message": "Locked"}.
     *         - Rechercher le titre demandé (vault_find_entry).
     *         - Si trouvé, renvoyer {"status": "success", "login": "...", "password": "..."}.
     *         - Si non trouvé, renvoyer {"status": "error", "message": "Not found"}.
     *       - Action "lock" :
     *         - Libérer le Vault en RAM avec vault_free().
     *         - Renvoyer {"status": "locked"}.
     *    g. Envoyer la réponse JSON sur stdout :
     *       - Calculer la taille de la réponse JSON en octets (strlen(response_json)).
     *       - Écrire d'abord la taille en 4 octets (uint32_t) sur stdout (fwrite).
     *       - Écrire la chaîne JSON (fwrite).
     *       - Appeler fflush(stdout) pour forcer l'envoi immédiat du message à Firefox.
     *       - Libérer les buffers temporaires et nettoyer les variables sensibles.
     * 
     * 3. Avant de quitter (fin de boucle), s'assurer que si un Vault était déverrouillé, 
     *    il soit bien libéré de la mémoire RAM de manière sécurisée (vault_free).
     */
    return -1;
}
