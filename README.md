# Secrets Manager (`passmgr`)

Un gestionnaire de secrets local et sécurisé écrit en C (libsodium) avec intégration native pour une extension Firefox.

---

## 🔒 Stack & Architecture de Sécurité

* **Cryptographie** :
  * Dérivation de clé (KDF) : **Argon2id** (`crypto_pwhash`) pour transformer le mot de passe maître en clé symétrique de 32 octets de manière hautement résistante aux attaques par force brute (GPU/ASIC).
  * Chiffrement symétrique : **XSalsa20-Poly1305** (`crypto_secretbox_easy`) pour garantir la confidentialité et l'authenticité (AEAD) du coffre.
* **Sécurité de la Mémoire** :
  * Allocation via `sodium_malloc()` / `sodium_allocarray()` pour bénéficier de pages de garde contre les dépassements de tampon (buffer overflows).
  * Nettoyage actif de la mémoire avec `sodium_memzero()` avant chaque libération de secret.
  * Prévention du swap disque via `sodium_mlock()` (recommandé avec un swap système chiffré par LUKS).
* **Persistance Atomique** :
  * Écriture systématique dans un fichier temporaire (`vault.enc.tmp`) suivie d'un remplacement atomique par `rename()` afin d'éviter toute corruption du coffre original en cas de crash.

---

## 🚀 Étapes d'Implémentation

Le développement est divisé en trois phases principales :

### Phase 1 : Le Moteur C & l'Interface CLI

* **Étape 1 : Setup & Makefile**
  * Initialisation de libsodium avec `sodium_init()`.
  * Configuration du `Makefile` pour lier la bibliothèque `libsodium` (via `pkg-config`).
* **Étape 2 : Dérivation de clé (Argon2id)**
  * Génération du sel (salt) de 16 octets au premier lancement du coffre.
  * Dérivation de la clé maîtresse à l'aide de `crypto_pwhash()`.
* **Étape 3 : Structure de données dynamiques**
  * Implémentation du tableau dynamique d'entrées (`Entry` et `Vault`) en C.
  * Gestion propre des réallocations de mémoire et nettoyage des pointeurs.
* **Étape 4 : Sérialisation (Format TLV)**
  * Conversion de la structure de données dynamique en un flux binaire plat (Type-Length-Value).
  * Gestion d'un octet de version en tête du flux binaire pour assurer la compatibilité future.
* **Étape 5 : Chiffrement & Sauvegarde Atomique**
  * Chiffrement du buffer sérialisé avec un nonce unique de 24 octets généré par `randombytes_buf()`.
  * Écriture sécurisée et atomique du format : `[Salt 16o][Nonce 24o][Ciphertext]`.
* **Étape 6 : CLI & Saisie Sécurisée**
  * Intégration de `getopt` pour analyser les arguments CLI (`add`, `get`, `list`, `delete`).
  * Désactivation de l'écho console lors de la saisie du mot de passe maître en manipulant les flags `ECHO` de `termios.h`.
* **Étape 7 : Audit mémoire & Diagnostics**
  * Utilisation de Valgrind pour traquer les fuites mémoire.
  * Compilation avec AddressSanitizer (`make sanitize`) pour valider la robustesse face aux buffer overflows.

### Phase 2 : L'Extension Firefox & Native Messaging

* **Étape A : Hôte Native Messaging (Binaire C dédié)**
  * Mode d'exécution dédié du binaire (`passmgr --native`) pour communiquer avec Firefox en JSON standard.
  * Déclaration du manifeste de l'hôte native messaging dans le répertoire utilisateur : `~/.mozilla/native-messaging-hosts/passmgr.json`.
* **Étape B : Extension Firefox (Background Script)**
  * Initialisation de la connexion persistante via `browser.runtime.connectNative()`.
  * Conservation du Vault déchiffré en RAM C uniquement pendant la durée d'ouverture du port.
* **Étape C : Content Scripts (Auto-fill)**
  * Script injecté par l'extension pour détecter les formulaires de connexion.
  * Remplissage automatique des identifiants et mots de passe sur demande.
* **Étape D : Auto-lock & Sécurité**
  * Timer d'inactivité côté extension Firefox.
  * Signal de fermeture de connexion qui provoque l'extinction du binaire C et l'effacement immédiat de la RAM (`sodium_memzero()`).

### Phase 3 : Synchronisation (Optionnelle)

* Hébergement du coffre `vault.enc` sur un dossier partagé Syncthing (ex: hébergé sur Raspberry Pi) pour bénéficier de backups automatiques et d'un accès multi-appareils.

---

## 🛠️ Compilation & Utilisation du CLI

### Prérequis
Installer `libsodium` sur votre machine Linux :
```bash
# Debian / Ubuntu / Mint
sudo apt install libsodium-dev pkg-config

# Fedora / RHEL
sudo dnf install libsodium-devel pkg-config

# Arch Linux
sudo pacman -S libsodium pkg-config
```

### Compiler
```bash
# Compilation standard
make

# Compilation en mode debug avec Sanitizers
make sanitize

# Nettoyer les fichiers de compilation
make clean
```

### Exemples d'utilisation du CLI (Phase 1)
```bash
# Ajouter ou mettre à jour un secret
./passmgr -f moncoffre.enc add google.com

# Lister les titres de secrets stockés
./passmgr -f moncoffre.enc list

# Récupérer un secret spécifique (affiche login et mot de passe)
./passmgr -f moncoffre.enc get google.com

# Supprimer un secret
./passmgr -f moncoffre.enc delete google.com
```
