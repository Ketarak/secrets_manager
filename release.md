# Release Notes — Secrets Manager

All application release notes are centralized here. The continuous integration workflow uses this file to automatically generate the description for GitHub Releases.

## [0.2.0]
### Key Features & Enhancements
- **Vault Initialization & Creation from Extension**: Users can now create and initialize a brand-new encrypted vault directly from the WebExtension popup without using the CLI.
- **Manual Secret Creation & Deletion**:
  - Dynamically add new secrets of various types (Logins, SSH Keys, Credit Cards, Secure Notes) with customized templates and custom fields.
  - Safe 3-second double-confirmation mechanism on secret deletion to prevent accidental loss.
- **Cryptographically Secure Password Generator**: Integrated a strong password generator (`window.crypto.getRandomValues`) accessible with a magic wand click on any sensitive input field.
- **Automatic Form Submission Capture & Update**:
  - Automatically intercepts login form submissions on any website.
  - If the vault is unlocked, prompts the user via a sleek glassmorphic top-banner to save new credentials or update changed ones.
- **Advanced Autofill Suite**:
  - **Keyboard Shortcut**: Press `Ctrl+Shift+L` (or `Cmd+Shift+L` on macOS) on any page to instantly autofill matching credentials.
  - **Suggested Accounts**: The extension popup suggests matching accounts for the active website at the very top of the search view for quick one-click filling.
  - **In-Page Key Widget Dropdown**: Integrates interactive key icons in page input fields that display a Shadow-DOM-isolated dropdown of saved credentials, avoiding styling conflicts with the website's CSS.
- **Redundant Message & Flickering Fix**: Solved a bug in background messaging where double-relayed native host responses caused the popup views to flicker and switch rapidly.

## [0.1.0]
### Key Features
- **Secure Cryptographic Engine (C)**: Key derivation via **Argon2id** and symmetric encryption via **XSalsa20-Poly1305** (`libsodium`).
- **Interactive REPL Console (`vault>`)**: Support for `list`, `add`, `show`, `delete`, `destroy`, and `exit` commands.
- **Non-TTY Support**: Ability to pass commands and passwords to the CLI via pipes or redirections (ideal for automation and scripting).
- **Firefox Native Messaging Integration**: C-based communication host with built-in lightweight JSON encoding/decoding.
- **Firefox Extension**: Sleek glassmorphic interactive UI popup.
- **Automated Test Suite**: Unit tests for the crypto layer and end-to-end integration tests for the CLI.
- **Continuous Integration (GitHub Actions)**: Automated build validation and test suite execution (`make test`).
- **Universal Installer Script (`install.sh`)**: Automated dependency check, compilation, local installation (`~/.local/bin`), and Firefox Native Messaging setup in one command.
- **Project Documentation Revamp**: Fully rewritten README.md with detailed instructions for the installer, tests, and CLI usage.
