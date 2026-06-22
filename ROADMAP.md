# Roadmap — Secrets Manager (passmgr)

Ce document présente la feuille de route mise à jour pour le développement, la sécurisation, le test et la distribution de **Secrets Manager (passmgr)**. 

Conformément aux décisions récentes, le déploiement automatique sur l'**AUR (Arch User Repository)** a été retiré afin de se concentrer sur des modes de distribution plus universels (script `install.sh` propre et paquet autonome `AppImage`).

---

## 1. Migration vers GitHub

### Avant de publier le dépôt
- **Audit de l'historique Git** : Scanner l'historique local pour s'assurer qu'aucun fichier sensible (comme `vault.enc` ou des mots de passe de test contenant de vraies données) n'a été commité par inadvertance.
  ```bash
  git log --all --full-history -p | grep -i "password\|secret\|key\|token"
  ```
  *Note : Si des données sensibles sont détectées, utiliser `git filter-repo` ou `BFG Repo-Cleaner` avant de publier.*
- **Vérification du `.gitignore`** : S'assurer qu'il exclut bien :
  - Les bases de données chiffrées (`*.enc`, `vault.enc`, `test_vault.enc`)
  - Le binaire compilé (`passmgr`)
  - Les fichiers objets (`obj/`)
  - Les dumps de mémoire Valgrind (`vgcore.*`)

### Configuration du dépôt GitHub
- **Création du dépôt vide** : Créer un dépôt sur GitHub sans initialiser le README, la licence ou le `.gitignore` (pour éviter tout conflit lors du premier push).
- **Push initial** :
  ```bash
  git remote add origin https://github.com/<username>/secrets-manager.git
  git branch -M main
  git push -u origin main --tags
  ```
- **Stratégie de branches** :
  - `main` : Branche de production stable, protégée (les commits directs sont interdits, passage par Pull Request).
  - `dev` : Branche d'intégration pour les développements en cours.
  - `feature/*` : Branches temporaires pour le développement de nouvelles fonctionnalités.
- **Licence & Sécurité** :
  - Ajouter une licence open-source claire (ex: MIT ou GPL v3) dans un fichier `LICENSE`.
  - Configurer un fichier `SECURITY.md` pour expliquer la procédure de rapport de failles de sécurité.

---

## 2. Extension Firefox : Fonctionnalités à Compléter

### A. Autofill (Remplissage Automatique)
- **Étape 1 : Demande de credentials (Background Script)**
  - Intercepter le chargement d'une page Web.
  - Envoyer `{ action: "get", domain: "<domaine_courant>" }` au binaire C via le canal Native Messaging (`browser.runtime.connectNative("passmgr")`).
- **Étape 2 : Détection des champs (Content Script)**
  - Injecter un script de contenu léger dans les pages web visitées.
  - Repérer les champs d'identification : en priorité les champs `input[type="password"]`, puis remonter dans le DOM pour trouver le champ identifiant associé (e.g. `type="text"`, `type="email"`).
- **Étape 3 : Injection sécurisée et robuste**
  - Éviter d'écrire directement dans `field.value = "..."`, ce qui casse le fonctionnement des frameworks JS modernes (React, Vue, Angular) qui utilisent des états virtuels.
  - Solution : Utiliser le setter de prototype natif et déclencher les événements `input` et `change` :
    ```javascript
    const setter = Object.getOwnPropertyDescriptor(HTMLInputElement.prototype, 'value').set;
    setter.call(field, value);
    field.dispatchEvent(new Event('input', { bubbles: true }));
    field.dispatchEvent(new Event('change', { bubbles: true }));
    ```
- **Étape 4 : Raccourci clavier**
  - Ajouter une commande dans `manifest.json` (ex: `Ctrl+Shift+L` sous Linux/Windows) pour déclencher l'autofill à la demande de l'utilisateur.

### B. Enregistrement des Secrets depuis le Navigateur
- **Étape 5 : Interception de la soumission du formulaire**
  - Écouter l'événement `submit` en phase de capture (`true`) pour récupérer les identifiants saisis avant que le site ne vide les champs.
  - Distinguer une connexion d'une inscription en observant le nombre de champs password.
- **Étape 6 : Bannière de confirmation non bloquante**
  - Injecter temporairement une bannière HTML stylisée (look natif Firefox/glassmorphic) en haut de la page pour proposer d'enregistrer le mot de passe.
  - Proposer deux options : "Enregistrer" et "Ignorer". Disparition automatique après 15 secondes.
- **Étape 7 : Ajout au coffre**
  - Envoyer `{ action: "add", title: domain, fields: [{ name: "login", value: login }, { name: "password", value: pwd, is_sensitive: true }] }` au binaire C.
  - Le binaire C ajoute l'entrée via la logique déjà existante dans `vault.c`.

---

## 3. Recommandations Post-Implémentation

### Sécurité & Robustesse
- **Validation stricte du JSON côté C** : 
  - Renforcer le parseur JSON dans `src/native.c` (ou utiliser une bibliothèque JSON ultra-légère et sûre comme `jsmn` ou `parson`) pour éviter des buffer overflows en cas d'injection malveillante depuis le navigateur.
  - Valider la taille et les caractères autorisés pour chaque champ.
- **Verrouillage automatique (Auto-lock Timeout)** :
  - Ajouter une variable de timeout dans l'extension ou le binaire pour verrouiller automatiquement le coffre (libérer la clé en RAM) après $N$ minutes d'inactivité.
- **Sécurité RAM** :
  - S'assurer que le mot de passe maître ou la clé dérivée ne sont jamais enregistrés dans le stockage persistant de l'extension (`browser.storage.local`). Ils doivent rester uniquement en mémoire volatile.

### CLI (Interface Ligne de Commande)
- **Commande `edit`** : Permettre d'éditer directement un champ particulier d'un secret sans avoir à supprimer et recréer l'entrée complète.
- **Commande `search`** : Permettre une recherche rapide par mot-clé dans les titres et descriptions des entrées.
- **Commande `export`** : Exporter le coffre dans un format ouvert (ex: JSON décrypté) avec avertissement clair sur la sécurité.

### UX (Expérience Utilisateur)
- **Badge d'état** : Ajouter un badge (cadenas ouvert/fermé) sur l'icône de l'extension pour indiquer l'état du coffre (`browser.action.setBadgeText`).
- **Générateur intégré** : Ajouter un générateur de mots de passe forts dans la popup de l'extension, s'appuyant sur la fonction sécurisée `randombytes_buf` de libsodium.

---

## 4. Stratégie de Tests

### Tests Unitaires (Moteur C)
- Valider la dérivation de clé Argon2id (robustesse, gestion du sel).
- Tester la sérialisation / désérialisation du format binaire TLV avec des structures d'entrées complexes (champs multiples, caractères spéciaux).
- Tester le chiffrement XSalsa20-Poly1305 (validation des nonces, détection des altérations de fichier).

### Tests d'Intégration
- Script shell automatisé testant le flux complet du CLI : `add` -> `list` -> `show` -> `delete` -> validation de la suppression.
- Test de comportement avec de mauvais mots de passe maîtres (s'assurer de l'absence de crash et du bon code d'erreur).

### Analyses de Sécurité Mémoire
- Compiler systématiquement avec les drapeaux d'assainissement d'AddressSanitizer (ASan) et UndefinedBehaviorSanitizer (UBSan) :
  ```makefile
  make sanitize
  ```
- Lancer Valgrind régulièrement pour s'assurer qu'aucune fuite de mémoire (memory leak) ne persiste et que les zones sensibles de mémoire (contenant les clés ou mots de passe décryptés) sont correctement effacées avec `sodium_memzero` ou `sodium_free` :
  ```bash
  valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 ./passmgr
  ```

---

## 5. CI/CD — GitHub Actions

### Workflow de Tests (`ci.yml`)
- Déclenché à chaque push ou pull-request sur toutes les branches.
- **Étapes** :
  1. Cloner le code.
  2. Installer les dépendances (`libsodium-dev`, `valgrind`, outils de compilation).
  3. Compiler le projet (`make`).
  4. Lancer les tests unitaires et d'intégration (`make test`).
  5. Lancer l'analyse Valgrind et lever une erreur si des fuites ou corruptions mémoire sont détectées.

### Workflow de Release automatique (`release.yml`)
- Déclenché uniquement lors d'un push sur `main`, après réussite des tests.
- **Étapes** :
  1. Lire la version spécifiée dans le fichier `VERSION` (situé à la racine).
  2. Vérifier si un tag Git correspondant à cette version existe déjà (ex: `v1.2.0`).
  3. Si le tag n'existe pas, créer le tag Git et pousser la release sur GitHub.
  4. Compiler une version optimisée de production.
  5. Créer l'archive source (avec `git archive`) et y adjoindre le binaire compilé statiquement (ou sous forme d'AppImage).
  6. Générer automatiquement des notes de version à partir de l'historique des commits.
  7. Publier la release GitHub contenant l'archive, le binaire et le hash SHA-256 associé.

---

## 6. Distribution (Linux Universel)

Afin de simplifier l'installation sans dépendre d'un paquet Arch Linux AUR spécifique, deux méthodes complémentaires sont privilégiées :

### A. Script d'installation universel (`install.sh`)
- Détection automatique de la distribution via `/etc/os-release`.
- Installation automatique des dépendances requises (`make`, `libsodium`, `pkg-config`) selon le gestionnaire de paquets de la machine :
  - **Arch / EndeavourOS** : `pacman -S --needed base-devel libsodium`
  - **Debian / Ubuntu / Mint** : `apt-get install -y build-essential libsodium-dev pkg-config`
  - **Fedora** : `dnf install -y make gcc libsodium-devel pkg-config`
  - **openSUSE** : `zypper install -y make gcc libsodium-devel pkg-config`
- Compilation automatique du projet (`make`).
- Installation propre du binaire compilé dans `$PREFIX/bin` (par défaut `/usr/local/bin` ou `~/.local/bin` pour une installation sans privilèges root).
- Génération automatique du fichier de manifest Native Messaging de Firefox et dépôt dans le dossier utilisateur :
  `~/.mozilla/native-messaging-hosts/passmgr.json`

### B. Format AppImage (Portable)
- Construction d'un fichier AppImage auto-exécutable regroupant le binaire `passmgr` ainsi que sa dépendance `libsodium` compilée de manière statique.
- Permet aux utilisateurs d'exécuter l'application sur n'importe quelle distribution Linux moderne sans compiler de code source ni installer manuellement de bibliothèque.

---

## 7. Évolutions à Long Terme

- **Support TOTP (2FA)** : Intégration de la génération de codes de validation temporaires (comme Google Authenticator) directement au sein du coffre-fort à l'aide d'une bibliothèque C dédiée aux algorithmes TOTP (ex: `liboath`).
- **Importation facilitée** : Module de migration pour importer des fichiers d'export `.csv` issus de KeePass, Bitwarden ou de navigateurs pour simplifier la transition.
- **Audit de la robustesse** : Outil d'analyse locale signalant les mots de passe faibles, dupliqués ou expirés.
- **Compatibilité multi-navigateurs** : Adaptation aisée du manifeste Native Messaging pour ajouter le support de Google Chrome, Chromium et Brave (diffère principalement par l'emplacement du fichier manifest).
- **Portabilité macOS** : Adaptation minime pour compiler sous macOS via Homebrew (le code C repose sur des fonctionnalités POSIX standards et libsodium).

---

## 8. Tableau de Priorités

| Priorité | Tâche | Justification / Description | Statut |
| :--- | :--- | :--- | :--- |
| **1** | **Migration GitHub & Licence** | Initialiser le dépôt public de manière propre et sécurisée. | À faire |
| **2** | **Intégration de tests unitaires & de compilation** | Créer la suite de tests et configurer la CI/CD (`ci.yml`) sur GitHub. | À faire |
| **3** | **Script d'installation universel (`install.sh`)** | Rendre l'installation et le paramétrage du Native Messaging triviaux. | À faire |
| **4** | **Autofill intelligent dans Firefox** | Remplissage des credentials dans la page via script de contenu. | À faire |
| **5** | **Enregistrement depuis Firefox** | Détecter les saisies de formulaires et proposer l'ajout rapide au coffre. | À faire |
| **6** | **Validation JSON renforcée côté C** | Sécuriser le parsing des messages reçus par le binaire. | À faire |
| **7** | **Gestion des entrées multiples par domaine** | Gérer les cas où l'utilisateur possède plusieurs comptes sur un même site. | À faire |
| **8** | **Création du paquet AppImage** | Faciliter la distribution sous forme de binaire portable universel. | À faire |
| **9** | **Intégration TOTP / 2FA** | Améliorer la valeur ajoutée de l'outil avec le support des jetons 2FA. | À faire |
| **10** | **Importation KeePass/Bitwarden** | Lever les barrières à l'adoption pour les nouveaux utilisateurs. | À faire |
