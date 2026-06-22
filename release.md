# Notes de Release — Secrets Manager

Toutes les notes de version de l'application sont centralisées ici. Le workflow d'intégration continue utilise ce fichier pour générer automatiquement la description des Releases sur GitHub.

## [0.1.0]
### Fonctionnalités principales
- **Moteur cryptographique sécurisé (C)** : Dérivation de clés via **Argon2id** et chiffrement symétrique **XSalsa20-Poly1305** (`libsodium`).
- **Console REPL interactive (`vault>`)** : Prise en charge des commandes `list`, `add`, `show`, `delete`, `destroy` et `exit`.
- **Mode Non-TTY** : Possibilité de passer des commandes et mots de passe au CLI via des redirections ou des pipes (idéal pour l'automatisation).
- **Intégration Firefox Native Messaging** : Hôte de communication en C avec décodage/encodage JSON léger natif.
- **Extension Firefox** : Interface graphique sombre glassmorphic interactive (popup).
- **Suite de tests automatisée** : Tests unitaires de la couche crypto et tests d'intégration complets du CLI.
- **Intégration Continue (GitHub Actions)** : Validation automatique de compilation et de tests (`make test`).
