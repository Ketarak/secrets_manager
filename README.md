# Secrets Manager (`passmgr`)

A secure, local secrets manager written in C using `libsodium` with native integration for a Firefox Extension.

---

## 🔒 Security Stack & Architecture

* **Cryptography**:
  * Key Derivation Function (KDF): **Argon2id** (`crypto_pwhash`) to derive a strong 32-byte symmetric key from a master password. This is highly resistant to GPU/ASIC brute-force attacks.
  * Symmetric Encryption: **XSalsa20-Poly1305** (`crypto_secretbox_easy`) to guarantee both confidentiality and integrity (AEAD) of the vault database.
* **Memory Security**:
  * Allocation via `sodium_malloc()` / `sodium_allocarray()` to benefit from guard pages that prevent buffer overflow attacks.
  * Active memory cleaning using `sodium_memzero()` before freeing any sensitive fields, preventing leftover secrets from lingering in RAM.
  * Prevention of disk swap pages via `sodium_mlock()` (recommended alongside a LUKS encrypted system swap).
* **Atomic Persistence**:
  * Written to a temporary file (`vault.enc.tmp`) followed by an atomic `rename()` replacement to prevent database corruption in case of unexpected shutdowns.

---

## 🛠️ Build & Installation

### Prerequisites

Install `libsodium` development headers and `pkg-config` on your Linux machine:

```bash
# Debian / Ubuntu / Mint
sudo apt install libsodium-dev pkg-config

# Fedora / RHEL
sudo dnf install libsodium-devel pkg-config

# Arch Linux
sudo pacman -S libsodium pkg-config
```

### Compiling the Binary

```bash
# Standard compilation
make

# Compile in debug mode with AddressSanitizer (ASan) & UB Sanitizer
make sanitize

# Clean build artifacts
make clean
```

---

## 🚀 Usage

### 1. Interactive Console (REPL)

Run the binary directly to enter the interactive console:

```bash
./passmgr
```

Once inside, the master password is asked securely without echo. You can type the following commands inside the `vault> ` prompt:

* `help` : List all available commands.
* `list` : Show all secret titles in the vault.
* `add <title>` : Start an interactive assistant to add a secret.
* `show <title>` (or `get`) : Show details of a secret (sensitive fields are marked).
* `delete <title>` : Delete a secret from the vault.
* `destroy` : Permanently delete the vault file from disk, wipe RAM, and quit.
* `exit` (or `quit`) : Securely wipe session keys and quit.

### 2. Firefox WebExtension Integration

To install the Native Messaging host and load the extension in Firefox:

1. **Install the Host Manifest**:
   ```bash
   make install-native
   ```
   This generates and installs `~/.mozilla/native-messaging-hosts/passmgr.json` pointing directly to your compiled binary path.

2. **Load the Extension in Firefox**:
   * Open Firefox and go to `about:debugging`.
   * Click on **"This Firefox"** in the left sidebar.
   * Click **"Load Temporary Add-on..."**.
   * Navigate to `src/extension/` and select `manifest.json`.

3. **Interact**:
   Click on the puzzle icon in the toolbar, select **Secrets Manager**, enter your master password, and manage your credentials directly through a sleek glassmorphic UI.
