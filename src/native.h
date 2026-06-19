#ifndef NATIVE_H
#define NATIVE_H

/*
 * Phase 2 — Native Messaging Host Loop
 *
 * Cette boucle écoute sur stdin les messages envoyés par l'extension Firefox
 * et répond sur stdout selon le protocole imposé par Mozilla :
 *   - Chaque message (requête et réponse) commence par une taille de 32 bits
 *     (4 octets, format natif du système, habituellement Little-Endian sur x86/ARM)
 *     représentant la longueur en octets de la chaîne JSON qui suit.
 *
 * Retourne 0 en cas d'arrêt propre (ex: fin de fichier stdin), -1 en cas d'erreur.
 */
int native_messaging_loop(const char *filepath);

#endif /* NATIVE_H */
