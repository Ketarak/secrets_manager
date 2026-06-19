#ifndef VAULT_H
#define VAULT_H

#include <stddef.h>

#define VAULT_VERSION 1

/* Structure représentant une entrée dans le gestionnaire de secrets */
typedef struct {
    char *title;      /* Nom unique identifiant le secret (ex: google.com) */
    char *login;      /* Identifiant ou adresse email */
    char *password;   /* Mot de passe (donnée ultra-sensible) */
} Entry;

/* Structure représentant le coffre complet en mémoire */
typedef struct {
    Entry *entries;   /* Tableau dynamique d'entrées */
    size_t count;     /* Nombre actuel d'entrées */
    size_t capacity;  /* Capacité maximale allouée */
} Vault;

/*
 * Étape 3a : Gestion de la structure dynamique du Coffre.
 * Crée un coffre vide alloué dynamiquement.
 * Retourne le pointeur vers le Vault créé, ou NULL en cas d'erreur.
 */
Vault *vault_create(void);

/*
 * Étape 3b : Libération propre de la mémoire.
 * Libère récursivement le tableau d'entrées, les chaînes de chaque entrée
 * (title, login, password) et la structure Vault elle-même.
 * IMPORTANT : utiliser sodium_memzero() ou sodium_free() pour effacer activement
 * les mots de passe de la mémoire avant de libérer.
 */
void vault_free(Vault *vault);

/*
 * Étape 3c : Ajout d'une entrée.
 * Ajoute ou met à jour une entrée dans le coffre.
 * Si le titre existe déjà, remplace l'ancien login et password.
 * Gère la réallocation dynamique du tableau (realloc) si la capacité maximale est atteinte.
 * Retourne 0 en cas de succès, -1 en cas d'erreur.
 */
int vault_add_entry(Vault *vault, const char *title, const char *login, const char *password);

/*
 * Étape 3d : Recherche d'une entrée.
 * Cherche une entrée par son titre.
 * Retourne un pointeur vers l'Entry trouvée, ou NULL si elle n'existe pas.
 */
Entry *vault_find_entry(Vault *vault, const char *title);

/*
 * Étape 3e : Suppression d'une entrée.
 * Supprime une entrée par son titre et libère sa mémoire.
 * Réorganise le tableau dynamique pour combler le vide.
 * Retourne 0 en cas de succès, -1 si l'entrée n'a pas été trouvée.
 */
int vault_delete_entry(Vault *vault, const char *title);

/*
 * Étape 4a : Sérialisation (TLV / Length-Prefix)
 * Convertit le contenu du coffre en un buffer binaire sérialisé.
 * out_buf sera alloué dynamiquement et contiendra :
 *   [Version : 1o] [Nb d'entrées : 4o (uint32_t)]
 *   Puis pour chaque entrée :
 *     [Titre len : 2o (uint16_t)] [Titre string]
 *     [Login len : 2o (uint16_t)] [Login string]
 *     [Pass len : 2o (uint16_t)] [Pass string]
 * Retourne 0 en cas de succès, -1 en cas d'erreur.
 */
int vault_serialize(const Vault *vault, unsigned char **out_buf, size_t *out_len);

/*
 * Étape 4b : Désérialisation (TLV / Length-Prefix)
 * Reconstruit un objet Vault complet depuis un buffer binaire sérialisé.
 * Lit les longueurs successives de chaque champ pour parser les chaînes de taille variable.
 * Retourne le pointeur vers le nouveau Vault, ou NULL en cas d'erreur.
 */
Vault *vault_deserialize(const unsigned char *buf, size_t len);

/*
 * Étape 5c : Sauvegarde sécurisée et atomique dans le fichier.
 * 1. Sérialiser le coffre en mémoire.
 * 2. Chiffrer les données sérialisées (génère ciphertext + nonce).
 * 3. Écrire le fichier de manière atomique dans un fichier temporaire :
 *    - Ouvrir "filepath.tmp" en écriture binaire.
 *    - Écrire le sel (16o) + le nonce (24o) + le ciphertext chiffré.
 *    - Fermer le fichier.
 * 4. Remplacer le fichier d'origine par le fichier temporaire avec rename().
 * Retourne 0 en cas de succès, -1 en cas d'erreur.
 */
int vault_save(const Vault *vault, const char *filepath, const unsigned char *key, const unsigned char *salt);

/*
 * Étape 5d : Chargement et déchiffrement depuis le fichier.
 * 1. Ouvrir le fichier "filepath" en lecture binaire.
 * 2. Lire le sel (16o) en tête de fichier (utile pour redériver la clé maître).
 * 3. Lire le nonce (24o) puis le reste du fichier (ciphertext).
 * 4. Déchiffrer le ciphertext avec la clé et le nonce pour obtenir le buffer sérialisé.
 * 5. Désérialiser le buffer pour recréer l'objet Vault.
 * Retourne l'objet Vault déchiffré, ou NULL en cas d'erreur (mauvaise clé/corruption).
 */
Vault *vault_load(const char *filepath, const unsigned char *key, unsigned char *out_salt);

#endif /* VAULT_H */
