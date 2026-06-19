#ifndef VAULT_H
#define VAULT_H

#include <stddef.h>

#define VAULT_VERSION 2

/* 
 * Structure représentant un champ clé-valeur d'un secret.
 * Permet de stocker n'importe quelle métadonnée (login, mot de passe, clé SSH privée, etc.)
 */
typedef struct {
    char *name;       /* Nom du champ (ex: "username", "password", "private_key") */
    char *value;      /* Valeur associée (donnée potentiellement sensible) */
    int is_sensitive; /* Flag : 1 si la donnée est ultra-sensible (à masquer/effacer en priorité), 0 sinon */
} Field;

/* 
 * Structure représentant une entrée (un secret individuel)
 * Contient un tableau dynamique de champs (Fields)
 */
typedef struct {
    char *title;      /* Nom unique identifiant le secret (ex: google.com, id_ed25519) */
    char *type;       /* Type de secret (ex: "login", "ssh", "card", "note") */
    Field *fields;    /* Tableau dynamique de champs */
    size_t count;     /* Nombre de champs dans cette entrée */
    size_t capacity;  /* Capacité maximale allouée pour les champs */
} Entry;

/* 
 * Structure représentant le coffre complet en mémoire.
 * Contient un tableau dynamique d'entrées (Entries)
 */
typedef struct {
    Entry *entries;   /* Tableau dynamique d'entrées */
    size_t count;     /* Nombre actuel d'entrées */
    size_t capacity;  /* Capacité maximale allouée pour les entrées */
} Vault;

/*
 * Étape 3a : Gestion de la structure dynamique du Coffre.
 * Crée un coffre vide alloué dynamiquement en mémoire.
 * Retourne le pointeur vers le Vault créé, ou NULL en cas d'erreur.
 */
Vault *vault_create(void);

/*
 * Étape 3b : Libération propre de la mémoire.
 * Libère récursivement le tableau d'entrées, les structures Entry, leurs tableaux de Field,
 * et toutes les chaînes de caractères associées.
 * IMPORTANT : utiliser sodium_memzero() pour effacer activement de la RAM les valeurs des champs 
 * sensibles (ou tous les champs par précaution) avant de libérer.
 */
void vault_free(Vault *vault);

/*
 * Étape 3c : Ajout d'une entrée vide.
 * Crée et ajoute une nouvelle entrée vide (sans champs) avec un titre et un type.
 * Gère la réallocation dynamique du tableau d'entrées si la capacité du Vault est atteinte.
 * Retourne le pointeur vers l'Entry créée, ou NULL si le titre existe déjà ou en cas d'erreur.
 */
Entry *vault_add_entry(Vault *vault, const char *title, const char *type);

/*
 * Étape 3d : Ajout ou mise à jour d'un champ dans une entrée.
 * Ajoute un champ (name, value, is_sensitive) à une entrée spécifique.
 * Si le champ 'name' existe déjà dans cette entrée, met à jour sa valeur et son flag.
 * Gère la réallocation dynamique du tableau de champs (fields) de l'Entry si nécessaire.
 * Retourne 0 en cas de succès, -1 en cas d'erreur.
 */
int entry_set_field(Entry *entry, const char *name, const char *value, int is_sensitive);

/*
 * Étape 3e : Recherche d'une entrée.
 * Cherche une entrée par son titre.
 * Retourne un pointeur vers l'Entry trouvée, ou NULL si elle n'existe pas.
 */
Entry *vault_find_entry(Vault *vault, const char *title);

/*
 * Étape 3f : Recherche d'un champ dans une entrée.
 * Cherche un champ par son nom dans une entrée.
 * Retourne un pointeur vers le Field trouvé, ou NULL s'il n'existe pas.
 */
Field *entry_find_field(Entry *entry, const char *name);

/*
 * Étape 3g : Suppression d'une entrée.
 * Supprime une entrée par son titre et libère sa mémoire (et ses champs).
 * Réorganise le tableau dynamique pour combler le vide.
 * Retourne 0 en cas de succès, -1 si l'entrée n'a pas été trouvée.
 */
int vault_delete_entry(Vault *vault, const char *title);

/*
 * Étape 4a : Sérialisation (TLV / Length-Prefix récursif)
 * Convertit le contenu du coffre en un buffer binaire sérialisé.
 * out_buf sera alloué dynamiquement et contiendra :
 *   [Version : 1o] [Nb d'entrées : 4o (uint32_t)]
 *   Puis pour chaque entrée :
 *     [Titre len : 2o] [Titre string]
 *     [Type len : 2o] [Type string]
 *     [Nb de champs : 2o (uint16_t)]
 *     Puis pour chaque champ :
 *       [Nom len : 2o] [Nom string]
 *       [Valeur len : 4o (uint32_t pour de grands volumes comme les clés SSH)] [Valeur string]
 *       [IsSensitive : 1o]
 * Retourne 0 en cas de succès, -1 en cas d'erreur.
 */
int vault_serialize(const Vault *vault, unsigned char **out_buf, size_t *out_len);

/*
 * Étape 4b : Désérialisation (TLV / Length-Prefix récursif)
 * Reconstruit un objet Vault complet depuis un buffer binaire sérialisé.
 * Retourne le pointeur vers le nouveau Vault, ou NULL en cas d'erreur.
 */
Vault *vault_deserialize(const unsigned char *buf, size_t len);

/*
 * Étape 5c : Sauvegarde sécurisée et atomique dans le fichier.
 * (Sérialise, chiffre avec encrypt_data, écrit dans filepath.tmp puis rename).
 * Retourne 0 en cas de succès, -1 en cas d'erreur.
 */
int vault_save(const Vault *vault, const char *filepath, const unsigned char *key, const unsigned char *salt);

/*
 * Étape 5d : Chargement et déchiffrement depuis le fichier.
 * (Ouvre le fichier, extrait le salt et le nonce, déchiffre, désérialise).
 * Retourne l'objet Vault déchiffré, ou NULL en cas d'erreur.
 */
Vault *vault_load(const char *filepath, const unsigned char *key, unsigned char *out_salt);

#endif /* VAULT_H */
