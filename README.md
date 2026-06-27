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

1. **Load the extension in Firefox**:
   * Open Firefox and navigate to `about:debugging`.
   * Click on **"This Firefox"** in the left sidebar.
   * Click **"Load Temporary Add-on..."**.
   * Navigate to your project directory, enter `src/extension/` and select `manifest.json`.

2. **Unlock the Vault**:
   * Click on the puzzle or key icon in the Firefox toolbar to open the **Secrets Manager** popup.
   * Enter your master password to unlock your vault (it utilizes Argon2id derived key for decryption).
   * If no vault exists yet, you can initialize a brand-new vault directly from the extension.

3. **Manage Credentials**:
   * **Creation**: Click the `+` button in the unlocked popup to create new secrets. Choose templates (Logins, SSH keys, Cards, Notes) or add custom sensitive/non-sensitive fields dynamically.
   * **Password Generator**: Utilize the magic wand button on any sensitive field to generate cryptographically secure passwords (`window.crypto.getRandomValues`).
   * **Deletion**: Select any secret and click "Supprimer". Confirm within 3 seconds to permanently remove the secret.

4. **Automatic Capture & Update**:
   * Submit a login form on any website. If the vault is unlocked, the extension intercepts it and displays a sleek glassmorphic banner asking if you'd like to save a new secret or update an existing one.

5. **Autofill Capabilities**:
   * **Keyboard Shortcut**: Press **`Ctrl+Shift+L`** (or `Cmd+Shift+L` on macOS) on any page to fill matching credentials instantly.
   * **Popup Suggestions**: When opening the popup, matching accounts for the active website are suggested at the very top for quick one-click filling.
   * **In-Page Key Widget**: Interactive key icons are injected in login fields. Clicking the key displays a Shadow-DOM-isolated dropdown overlay listing username options to populate the forms seamlessly.
