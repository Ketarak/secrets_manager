# Secrets Manager (`passmgr`)

A secure, local secrets manager written in C using `libsodium` with native integration for a Firefox Extension.

---

## 🔒 Security Stack & Architecture

* **Cryptography**:
  * **Key Derivation Function (KDF)**: **Argon2id** (`crypto_pwhash`) to derive a strong 32-byte symmetric key from a master password. This is highly resistant to GPU/ASIC brute-force attacks.
  * **Symmetric Encryption**: **XSalsa20-Poly1305** (`crypto_secretbox_easy`) to guarantee both confidentiality and integrity (AEAD) of the vault database.
* **Memory Security**:
  * Allocation via `sodium_malloc()` to benefit from guard pages that prevent buffer overflow attacks.
  * Active memory cleaning using `sodium_memzero()` before freeing any sensitive fields, preventing leftover secrets from lingering in RAM.
  * Prevention of disk swap pages via `sodium_mlock()` (recommended alongside a LUKS encrypted system swap).
* **Atomic Persistence**:
  * Written to a temporary file (`vault.enc.tmp`) followed by an atomic `rename()` replacement to prevent database corruption in case of unexpected shutdowns.
* **Security & Non-TTY Scripting**:
  * The C reader detects non-tty input environments. You can securely automate actions using redirections or piping without triggering interactive prompt issues.

---

## 🚀 Quick Install (Recommended)

You can build and install the entire application (including dependencies, the local C binary, and the Firefox Native Messaging host manifest) with a single script:

```bash
./install.sh
```

The script automatically:
1. Detects your Linux distribution (Arch, Debian, Fedora, openSUSE).
2. Verifies or installs required dependencies (`libsodium`, `make`, `gcc`, `pkg-config`).
3. Compiles the binary.
4. Installs the executable to `$HOME/.local/bin/passmgr` (no root required for execution).
5. Installs the Firefox Native Messaging host manifest at `~/.mozilla/native-messaging-hosts/passmgr.json` pointing to your local binary.

*(Note: Make sure `$HOME/.local/bin` is in your `$PATH` to run `passmgr` from anywhere).*

---

## 🛠️ Manual Build & Development

If you prefer to compile, install, or test manually:

### Compile the Binary
```bash
# Standard compilation
make

# Compile with AddressSanitizer (ASan) & UndefinedBehaviorSanitizer (UBSan)
make sanitize

# Clean build artifacts
make clean
```

### Run the Test Suite
The project features a full unit and integration test suite:
```bash
# Compiles and runs unit tests (crypto) and integration tests (CLI commands)
make test
```

### Manual Installation
```bash
# Copy binary to $PREFIX/bin (defaults to $HOME/.local/bin)
make install

# Configure the Firefox Native Messaging manifest
make install-native
```

---

## 💡 Usage

### 1. Interactive Console (REPL)

Run the binary directly to enter the interactive console:

```bash
passmgr
```

Once inside, you will be prompted for your master password securely (without echo). You can type the following commands inside the `vault> ` prompt:

* `help` : List all available commands.
* `list` : Show all secret titles in the vault.
* `add <title>` : Start an interactive assistant to add a secret.
* `show <title>` (or `get`) : Show details of a secret (sensitive fields are masked).
* `delete <title>` : Delete a secret from the vault.
* `destroy` : Permanently delete the vault file from disk, wipe RAM, and quit.
* `exit` (or `quit`) : Securely wipe session keys and quit.

### 2. Firefox WebExtension Integration

1. Load the extension in Firefox:
   * Open Firefox and navigate to `about:debugging`.
   * Click on **"This Firefox"** in the left sidebar.
   * Click **"Load Temporary Add-on..."**.
   * Navigate to your project directory, enter `src/extension/` and select `manifest.json`.

2. Manage credentials:
   * Click on the puzzle icon in the Firefox toolbar, select **Secrets Manager**.
   * Enter your master password to unlock the vault.
   * Search, view, and copy your passwords securely using the sleek glassmorphic UI.
