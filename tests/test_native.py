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

    finally:
        proc.stdin.close()
        proc.wait()
        cleanup()

    print("=== ALL NATIVE MESSAGING TESTS PASSED ===")

if __name__ == "__main__":
    main()
