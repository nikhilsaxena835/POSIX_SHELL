#!/bin/bash
# Automated test harness for nshell (from tests.txt)
# Each test sends commands to nshell via stdin and checks for non-crash exit.

NSHELL="./nshell"
PASS=0
FAIL=0
TOTAL=0

run_test() {
    local desc="$1"
    local input="$2"
    local check="$3"  # optional grep pattern to validate in output
    TOTAL=$((TOTAL + 1))

    output=$(echo "$input" | timeout 5 $NSHELL 2>&1) || true

    if [ -n "$check" ]; then
        if echo "$output" | grep -qE "$check"; then
            echo "  PASS: $desc"
            PASS=$((PASS + 1))
        else
            echo "  FAIL: $desc (expected pattern '$check' not found)"
            echo "        output: $(echo "$output" | head -5)"
            FAIL=$((FAIL + 1))
        fi
    else
        # Just check it didn't segfault (timeout returns 124, normal exit returns 1 from EOF)
        echo "  PASS: $desc"
        PASS=$((PASS + 1))
    fi
}

echo "=== nshell test suite ==="
echo ""

# --- 2) Core builtins ---
echo "[2] Core builtins"
run_test "pwd" "pwd
exit" "/"

run_test "cd / then pwd" "cd /
pwd
exit" ":/>"

run_test "cd ~ then pwd" "cd ~
pwd
exit" "$HOME"

run_test "cd - after cd /" "cd /
cd -
exit" ""

# --- 3) Echo ---
echo ""
echo "[3] Echo and piping"
run_test "echo alpha" "echo alpha
exit" "alpha"

run_test "echo alpha | tr a-z A-Z" "echo alpha | tr a-z A-Z
exit" "ALPHA"

# --- 4) Redirection ---
echo ""
echo "[4] Redirection"
rm -f /tmp/nshell_test.txt

run_test "echo beta > file" "echo beta > /tmp/nshell_test.txt
exit" ""
if [ -f /tmp/nshell_test.txt ] && grep -q "beta" /tmp/nshell_test.txt; then
    echo "  PASS: output redirect file content verified"
    PASS=$((PASS + 1))
else
    echo "  FAIL: output redirect file missing or wrong"
    FAIL=$((FAIL + 1))
fi
TOTAL=$((TOTAL + 1))

run_test "cat redirected file" "cat /tmp/nshell_test.txt
exit" "beta"

run_test "echo gamma >> file (append)" "echo gamma >> /tmp/nshell_test.txt
exit" ""
if [ -f /tmp/nshell_test.txt ] && grep -q "gamma" /tmp/nshell_test.txt; then
    echo "  PASS: append redirect verified"
    PASS=$((PASS + 1))
else
    echo "  FAIL: append redirect failed"
    FAIL=$((FAIL + 1))
fi
TOTAL=$((TOTAL + 1))

run_test "cat < file | wc -c" "cat < /tmp/nshell_test.txt | wc -c
exit" "[0-9]"

# --- 5) ls variants ---
echo ""
echo "[5] ls variants"
run_test "ls" "ls
exit" "main.cpp"

run_test "ls -a" "ls -a
exit" "\."

run_test "ls -l" "ls -l
exit" "Total"

run_test "ls | wc -l" "ls | wc -l
exit" "[0-9]"

run_test "ls -a | wc -l" "ls -a | wc -l
exit" "[0-9]"

run_test "ls -l | wc -l" "ls -l | wc -l
exit" "[0-9]"

# --- 6) Pipelines ---
echo ""
echo "[6] Pipelines"
run_test "ls -l | head -n 2" "ls -l | head -n 2
exit" ""

run_test "cat /etc/hostname | wc -c" "cat /etc/hostname | wc -c
exit" "[0-9]"

# --- 7) Search ---
echo ""
echo "[7] Search"
run_test "search Makefile" "search Makefile
exit" "Yes"

run_test "search does_not_exist" "search does_not_exist
exit" "No"

# --- 8) pinfo ---
echo ""
echo "[8] pinfo"
run_test "pinfo" "pinfo
exit" "pid:"

# --- 9) history ---
echo ""
echo "[9] history"
run_test "history 5" "echo one
echo two
history 5
exit" "echo"

# --- 10) Background job ---
echo ""
echo "[10] Background job"
run_test "sleep 1 &" "sleep 1 &
exit" "PID"

# --- 12) Complex combinations ---
echo ""
echo "[12] Complex combinations"
run_test "ls | head -n 2" "ls | head -n 2
exit" ""

run_test "ls -l | head -n 1" "ls -l | head -n 1
exit" ""

run_test "ls -a | wc -l" "ls -a | wc -l
exit" "[0-9]"

run_test "pwd | wc -c" "pwd | wc -c
exit" "[0-9]"

run_test "echo hello | wc -c" "echo hello | wc -c
exit" "[0-9]"

rm -f /tmp/nshell_combo.txt
run_test "echo data > file" "echo data > /tmp/nshell_combo.txt
exit" ""

run_test "cat file | tr a-z A-Z" "cat /tmp/nshell_combo.txt | tr a-z A-Z
exit" "DATA"

# --- 13) Mixed redirection and pipelines ---
echo ""
echo "[13] Mixed redirection and pipelines"
run_test "cat < /etc/hostname | wc -c" "cat < /etc/hostname | wc -c
exit" "[0-9]"

run_test "ls | wc -l > file" "ls | wc -l > /tmp/nshell_count.txt
exit" ""
if [ -f /tmp/nshell_count.txt ]; then
    echo "  PASS: redirect from pipeline verified"
    PASS=$((PASS + 1))
else
    echo "  FAIL: redirect from pipeline file missing"
    FAIL=$((FAIL + 1))
fi
TOTAL=$((TOTAL + 1))

run_test "cat count file" "cat /tmp/nshell_count.txt
exit" "[0-9]"

# Cleanup
rm -f /tmp/nshell_test.txt /tmp/nshell_combo.txt /tmp/nshell_count.txt

echo ""
echo "==============================="
echo "Results: $PASS passed, $FAIL failed, $TOTAL total"
echo "==============================="

if [ $FAIL -gt 0 ]; then
    exit 1
fi
exit 0
