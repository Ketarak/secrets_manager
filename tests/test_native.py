#!/usr/bin/env python3
import os
import sys
import subprocess
import json
import struct

VAULT_FILE = "tests/test_native_vault.enc"
BIN = "./passmgr"

def cleanup():
    if os.path.exists(VAULT_FILE):
        os.remove(VAULT_FILE)

def send_msg(proc, payload):
    data = json.dumps(payload).encode('utf-8')
    length = len(data)
    # Write 4 bytes length header (native format) and payload
    proc.stdin.write(struct.pack('I', length))
    proc.stdin.write(data)
    proc.stdin.flush()

def read_msg(proc):
    header = proc.stdout.read(4)
    if not header or len(header) < 4:
        return None
    length = struct.unpack('I', header)[0]
    data = proc.stdout.read(length)
    return json.loads(data.decode('utf-8'))

def main():
    print("=== STARTING NATIVE MESSAGING INTEGRATION TESTS ===")
    cleanup()

    # Start passmgr in native messaging mode
    proc = subprocess.Popen(
        [BIN, "--native", "-f", VAULT_FILE],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )

    try:
        # 1. Test status (vault does not exist)
        print("[Test 1] Query status when vault does not exist...")
        send_msg(proc, {"action": "status"})
        resp = read_msg(proc)
        print(f"Response: {resp}")
        assert resp["status"] == "locked", f"Expected 'locked', got: {resp}"
        assert resp["vault_exists"] is False, f"Expected vault_exists=False, got: {resp}"
        print("Pass: Status correct.")

        # 2. Test create vault
        print("[Test 2] Creating a new vault via native message...")
        send_msg(proc, {"action": "create", "password": "supersecretpassword123"})
        resp = read_msg(proc)
        print(f"Response: {resp}")
        assert resp["status"] == "success", f"Expected success, got: {resp}"
        assert "Vault created" in resp["message"], f"Expected creation message, got: {resp}"
        assert os.path.exists(VAULT_FILE), "Vault file should exist on disk."
        print("Pass: Vault created successfully.")

        # 3. Test status immediately after creation (should be unlocked automatically)
        print("[Test 3] Query status after creation (should be auto-unlocked)...")
        send_msg(proc, {"action": "status"})
        resp = read_msg(proc)
        print(f"Response: {resp}")
        assert resp["status"] == "unlocked", f"Expected unlocked, got: {resp}"
        print("Pass: Auto-unlock worked.")

        # 4. Lock vault
        print("[Test 4] Locking the vault...")
        send_msg(proc, {"action": "lock"})
        resp = read_msg(proc)
        print(f"Response: {resp}")
        assert resp["status"] == "success", f"Expected success, got: {resp}"
        
        # Verify status is now locked and vault_exists is true
        send_msg(proc, {"action": "status"})
        resp = read_msg(proc)
        print(f"Response: {resp}")
        assert resp["status"] == "locked", f"Expected locked, got: {resp}"
        assert resp["vault_exists"] is True, f"Expected vault_exists=True, got: {resp}"
        print("Pass: Vault locked and existence reported correctly.")

        # 5. Unlock with wrong password
        print("[Test 5] Unlock with incorrect password...")
        send_msg(proc, {"action": "unlock", "password": "wrongpassword"})
        resp = read_msg(proc)
        print(f"Response: {resp}")
        assert resp["status"] == "error", f"Expected error, got: {resp}"
        assert "Incorrect password" in resp["message"], f"Expected incorrect password error, got: {resp}"
        print("Pass: Wrong password rejected.")

        # 6. Unlock with correct password
        print("[Test 6] Unlock with correct password...")
        send_msg(proc, {"action": "unlock", "password": "supersecretpassword123"})
        resp = read_msg(proc)
        print(f"Response: {resp}")
        assert resp["status"] == "success", f"Expected success, got: {resp}"
        
        send_msg(proc, {"action": "status"})
        resp = read_msg(proc)
        print(f"Response: {resp}")
        assert resp["status"] == "unlocked", f"Expected unlocked, got: {resp}"
        print("Pass: Correct password unlocks successfully.")

        # 7. Try to create again when it already exists (should fail)
        print("[Test 7] Try to create a vault that already exists...")
        send_msg(proc, {"action": "create", "password": "anotherpassword"})
        resp = read_msg(proc)
        print(f"Response: {resp}")
        assert resp["status"] == "error", f"Expected error, got: {resp}"
        assert "already exists" in resp["message"].lower(), f"Expected 'already exists' message, got: {resp}"
        print("Pass: Vault overwrite prevention verified.")

        # 8. Test adding a secret
        print("[Test 8] Adding a secret via native message...")
        send_msg(proc, {
            "action": "add",
            "title": "github.com",
            "type": "login",
            "fields": [
                {"name": "username", "value": "maxime", "is_sensitive": False},
                {"name": "password", "value": "mysecurepassword456", "is_sensitive": True}
            ]
        })
        resp = read_msg(proc)
        print(f"Response: {resp}")
        assert resp["status"] == "success", f"Expected success, got: {resp}"
        print("Pass: Secret added successfully.")

        # 9. Test getting the added secret
        print("[Test 9] Retrieving the added secret...")
        send_msg(proc, {"action": "get", "title": "github.com"})
        resp = read_msg(proc)
        print(f"Response: {resp}")
        assert resp["status"] == "success", f"Expected success, got: {resp}"
        assert resp["title"] == "github.com", f"Expected title github.com, got: {resp}"
        assert resp["type"] == "login", f"Expected type login, got: {resp}"
        assert len(resp["fields"]) == 2, f"Expected 2 fields, got: {resp}"
        # Fields order might match insertion order
        fields = {f["name"]: f for f in resp["fields"]}
        assert fields["username"]["value"] == "maxime", f"Expected username value, got: {resp}"
        assert fields["username"]["is_sensitive"] is False
        assert fields["password"]["value"] == "mysecurepassword456", f"Expected password value, got: {resp}"
        assert fields["password"]["is_sensitive"] is True
        print("Pass: Secret fields retrieved correctly.")

        # 10. Test listing secrets
        print("[Test 10] Listing secrets...")
        send_msg(proc, {"action": "list"})
        resp = read_msg(proc)
        print(f"Response: {resp}")
        assert resp["status"] == "success", f"Expected success, got: {resp}"
        assert len(resp["secrets"]) == 1, f"Expected 1 secret, got: {resp}"
        assert resp["secrets"][0]["title"] == "github.com", f"Expected github.com, got: {resp}"
        print("Pass: Secrets listed correctly.")

        # 11. Test deleting the secret
        print("[Test 11] Deleting the secret...")
        send_msg(proc, {"action": "delete", "title": "github.com"})
        resp = read_msg(proc)
        print(f"Response: {resp}")
        assert resp["status"] == "success", f"Expected success, got: {resp}"
        print("Pass: Secret deleted successfully.")

        # 12. Verify secret is gone via list and get
        print("[Test 12] Verifying deletion...")
        send_msg(proc, {"action": "list"})
        resp = read_msg(proc)
        print(f"Response: {resp}")
        assert resp["status"] == "success", f"Expected success, got: {resp}"
        assert len(resp["secrets"]) == 0, f"Expected 0 secrets, got: {resp}"

        send_msg(proc, {"action": "get", "title": "github.com"})
        resp = read_msg(proc)
        print(f"Response: {resp}")
        assert resp["status"] == "error", f"Expected error, got: {resp}"
        assert "not found" in resp["message"].lower(), f"Expected not found, got: {resp}"
        print("Pass: Secret deletion verified.")

    finally:
        proc.stdin.close()
        proc.wait()
        cleanup()

    print("=== ALL NATIVE MESSAGING TESTS PASSED ===")

if __name__ == "__main__":
    main()
