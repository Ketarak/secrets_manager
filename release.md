# Release Notes — Secrets Manager

All application release notes are centralized here. The continuous integration workflow uses this file to automatically generate the description for GitHub Releases.

## [0.1.0]
### Key Features
- **Secure Cryptographic Engine (C)**: Key derivation via **Argon2id** and symmetric encryption via **XSalsa20-Poly1305** (`libsodium`).
- **Interactive REPL Console (`vault>`)**: Support for `list`, `add`, `show`, `delete`, `destroy`, and `exit` commands.
- **Non-TTY Support**: Ability to pass commands and passwords to the CLI via pipes or redirections (ideal for automation and scripting).
- **Firefox Native Messaging Integration**: C-based communication host with built-in lightweight JSON encoding/decoding.
- **Firefox Extension**: Sleek glassmorphic interactive UI popup.
- **Automated Test Suite**: Unit tests for the crypto layer and end-to-end integration tests for the CLI.
- **Continuous Integration (GitHub Actions)**: Automated build validation and test suite execution (`make test`).
