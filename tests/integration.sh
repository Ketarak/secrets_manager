#!/bin/bash
set -e

# Integration tests for Secrets Manager (passmgr)

VAULT_FILE="tests/test_vault_tmp.enc"
BIN="./passmgr"

# Ensure we clean up on exit or error
cleanup() {
    if [ -f "$VAULT_FILE" ]; then
        rm -f "$VAULT_FILE"
    fi
}
trap cleanup EXIT

echo "=== STARTING INTEGRATION TESTS ==="

# Clean up any leftover test vault
cleanup

# 1. Test Vault Creation
echo "[Test 1/5] Creating a new vault..."
# Feed password, confirmation password, and then exit
printf "mypassword123\nmypassword123\nexit\n" | $BIN -f "$VAULT_FILE" > /dev/null

if [ ! -f "$VAULT_FILE" ]; then
    echo "FAILED: Vault file was not created!"
    exit 1
fi
echo "Pass: Vault file created successfully."

# 2. Test Adding a Secret
echo "[Test 2/5] Adding a secret..."
# Feed password, add command, type, fields, then exit
# Assistant asks:
# - Enter secret type (login, ssh, card, note) [default: login]:
# - Enter field name (or press Enter to finish): login
# - Enter field value: myusername
# - Is this field sensitive? (y/n) [default: n]: n
# - Enter field name (or press Enter to finish): password
# - Enter field value: mysecretpassword
# - Is this field sensitive? (y/n) [default: n]: y
# - Enter field name (or press Enter to finish): (Enter to finish)
printf "mypassword123\nadd github.com\nlogin\nlogin\nmyusername\nn\npassword\nmysecretpassword\ny\n\nexit\n" | $BIN -f "$VAULT_FILE" > /dev/null
echo "Pass: Secret added successfully."

# 3. Test Listing and Showing a Secret
echo "[Test 3/5] Listing and displaying secret details..."
OUTPUT=$(printf "mypassword123\nlist\nshow github.com\nexit\n" | $BIN -f "$VAULT_FILE")

# Check listing
if ! echo "$OUTPUT" | grep -q "github.com"; then
    echo "FAILED: Secret 'github.com' not found in listing!"
    echo "Output was:"
    echo "$OUTPUT"
    exit 1
fi

# Check fields in details
if ! echo "$OUTPUT" | grep -q "login : myusername"; then
    echo "FAILED: Field 'login' not displayed correctly!"
    exit 1
fi

if ! echo "$OUTPUT" | grep -q "password (sensitive) : mysecretpassword"; then
    echo "FAILED: Sensitive field 'password' not displayed correctly!"
    exit 1
fi
echo "Pass: List and Show output verified."

# 4. Test Deleting a Secret
echo "[Test 4/5] Deleting a secret..."
OUTPUT_DEL=$(printf "mypassword123\ndelete github.com\nlist\nexit\n" | $BIN -f "$VAULT_FILE")

if echo "$OUTPUT_DEL" | grep -q "github.com (type:"; then
    echo "FAILED: Secret 'github.com' was not deleted!"
    exit 1
fi
echo "Pass: Secret deleted successfully."

# 5. Test Accessing with Wrong Password
echo "[Test 5/5] Accessing vault with an incorrect password..."
# Set +e temporarily to capture exit status of the failed command
set +e
printf "wrongpwd\n" | $BIN -f "$VAULT_FILE" &> /dev/null
EXIT_CODE=$?
set -e

if [ $EXIT_CODE -eq 0 ]; then
    echo "FAILED: Vault opened or exited with code 0 using incorrect password!"
    exit 1
fi
echo "Pass: Access denied with incorrect password (exit code $EXIT_CODE)."

echo "=== ALL INTEGRATION TESTS PASSED ==="
exit 0
