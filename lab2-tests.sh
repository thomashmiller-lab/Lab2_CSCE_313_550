#!/usr/bin/env bash

# ============================================================
# CSCE 313 - Lab 2 Public Test Script
# Mini Unix Shell
# ============================================================

# Function to clean and rebuild
remake () {
    make -s clean
    make -s >/dev/null 2>&1
}

# ------------------------------------------------------------
# Colors
# ------------------------------------------------------------

echo -e "To remove colour from tests, set COLOUR to 1 in sh file\n"

COLOUR=0

if [[ $COLOUR -eq 0 ]]; then
    ORANGE='\033[0;33m'
    GREEN='\033[0;32m'
    RED='\033[0;31m'
    NC='\033[0m'
else
    ORANGE='\033[0m'
    GREEN='\033[0m'
    RED='\033[0m'
    NC='\033[0m'
fi


SCORE=0

echo -e "\n========================================"
echo -e "        CSCE 313 - Lab 2 Tests"
echo -e "========================================\n"


# ============================================================
# 1. Compilation - 5 pts
# ============================================================

echo -e "Testing :: Compilation\n"

make -s clean

if make -s >/dev/null 2>&1; then
    echo -e "  ${GREEN}Passed${NC}"
    SCORE=$((SCORE+5))
else
    echo -e "  ${RED}Failed${NC}"
    echo -e "\nCannot continue testing because compilation failed."
    echo -e "\nSCORE: ${SCORE}/100\n"
    exit 0
fi


# ============================================================
# 2. Shell Exit / No Crash - 5 pts
# ============================================================

remake

echo -e "\nTesting :: Shell Exit / No Crash\n"

if ./shell < ./test-files/test_exit.txt >/dev/null 2>&1; then
    echo -e "  ${GREEN}Passed${NC}"
    SCORE=$((SCORE+5))
else
    echo -e "  ${RED}Failed${NC}"
fi


# ============================================================
# 3. Multiple Input Lines - 10 pts
# ============================================================

remake

echo -e "\nTesting :: Correct Output For Multiple Lines\n"

cat ./test-files/test_multiple_lines.txt \
    ./test-files/test_exit.txt \
    > ./test-files/cmd.txt

RES=$(. ./test-files/test_multiple_lines.txt)

if ./shell < ./test-files/cmd.txt 2>/dev/null |
    grep -qF -- "${RES}"; then

    echo -e "  ${GREEN}Passed${NC}"
    SCORE=$((SCORE+10))

else
    echo -e "  ${RED}Failed${NC}"
fi


# ============================================================
# 4. Two-Command Pipe - 10 pts
# ============================================================

remake

echo -e "\nTesting :: Two-Command Pipeline\n"

cat ./test-files/test_two_commands.txt \
    ./test-files/test_exit.txt \
    > ./test-files/cmd.txt

RES=$(. ./test-files/test_two_commands.txt)

if ./shell < ./test-files/cmd.txt 2>/dev/null |
    grep -qF -- "${RES}"; then

    echo -e "  ${GREEN}Passed${NC}"
    SCORE=$((SCORE+10))

else
    echo -e "  ${RED}Failed${NC}"
fi


# ============================================================
# 5. Three or More Commands - 10 pts
# ============================================================

remake

echo -e "\nTesting :: Three or More Piped Commands\n"

cat ./test-files/test_four_commands.txt \
    ./test-files/test_exit.txt \
    > ./test-files/cmd.txt

RES=$(. ./test-files/test_four_commands.txt)

if ./shell < ./test-files/cmd.txt 2>/dev/null |
    grep -qF -- "${RES}"; then

    echo -e "  ${GREEN}Passed${NC}"
    SCORE=$((SCORE+10))

else
    echo -e "  ${RED}Failed${NC}"
fi


# ============================================================
# 6. Input Redirection (<) - 10 pts
# ============================================================

remake

echo -e "\nTesting :: Input Redirection (<)\n"

OUTPUT=$(./shell < ./test-files/test_input_redirection.txt 2>/dev/null)

if echo "$OUTPUT" | grep -qF "hello world" &&
   echo "$OUTPUT" | grep -qF "hello linux" &&
   echo "$OUTPUT" | grep -qF "hello shell"; then

    echo -e "  ${GREEN}Passed${NC}"
    SCORE=$((SCORE+10))

else
    echo -e "  ${RED}Failed${NC}"
fi


# ============================================================
# 7. Output Redirection (>) - 10 pts
# ============================================================

remake
rm -f test-output.txt

echo -e "\nTesting :: Output Redirection (>)\n"

./shell < ./test-files/test_output_redirection.txt \
    >/dev/null 2>&1

if [[ -f test-output.txt ]] &&
   grep -qF "Hello_from_Lab2" test-output.txt; then

    echo -e "  ${GREEN}Passed${NC}"
    SCORE=$((SCORE+10))

else
    echo -e "  ${RED}Failed${NC}"
fi


# ============================================================
# 8. Append Redirection (>>) - 10 pts
# ============================================================

remake
rm -f test-append.txt

echo -e "\nTesting :: Append Redirection (>>)\n"

./shell < ./test-files/test_append_redirection.txt \
    >/dev/null 2>&1

if [[ -f test-append.txt ]] &&
   grep -qF "first_line" test-append.txt &&
   grep -qF "second_line" test-append.txt; then

    echo -e "  ${GREEN}Passed${NC}"
    SCORE=$((SCORE+10))

else
    echo -e "  ${RED}Failed${NC}"
fi


# ============================================================
# 9. Pipes + Redirection - 10 pts
# ============================================================

remake
rm -f test-pipe-output.txt

echo -e "\nTesting :: Pipes Combined With Redirection\n"

./shell < ./test-files/test_pipe_redirection.txt \
    >/dev/null 2>&1

if [[ -f test-pipe-output.txt ]] &&
   grep -qE '^[[:space:]]*3[[:space:]]*$' test-pipe-output.txt; then

    echo -e "  ${GREEN}Passed${NC}"
    SCORE=$((SCORE+10))

else
    echo -e "  ${RED}Failed${NC}"
fi


# ============================================================
# 10. mycat - 5 pts
# ============================================================

remake

echo -e "\nTesting :: mycat Built-in\n"

OUTPUT=$(./shell < ./test-files/test_mycat.txt 2>/dev/null)

if echo "$OUTPUT" | grep -qF "hello world" &&
   echo "$OUTPUT" | grep -qF "operating systems" &&
   echo "$OUTPUT" | grep -qF "hello shell"; then

    echo -e "  ${GREEN}Passed${NC}"
    SCORE=$((SCORE+5))

else
    echo -e "  ${RED}Failed${NC}"
fi


# ============================================================
# 11. myls / File Permissions - 5 pts
# ============================================================

remake

# Reset permission before testing
chmod 644 ./test-files/permission_test.txt

echo -e "\nTesting :: myls File Permissions\n"

OUTPUT=$(./shell < ./test-files/test_myls.txt 2>/dev/null)

if echo "$OUTPUT" |
    grep -qF "Permissions: rw-r--r--"; then

    echo -e "  ${GREEN}Passed${NC}"
    SCORE=$((SCORE+5))

else
    echo -e "  ${RED}Failed${NC}"
fi


# ============================================================
# 12. mychmod - 10 pts
# ============================================================

remake

# Always start from 644
chmod 644 ./test-files/permission_test.txt

echo -e "\nTesting :: mychmod Built-in\n"

OUTPUT=$(./shell < ./test-files/test_mychmod.txt 2>/dev/null)

if echo "$OUTPUT" |
    grep -qF "Permissions: rwxr-xr-x"; then

    echo -e "  ${GREEN}Passed${NC}"
    SCORE=$((SCORE+10))

else
    echo -e "  ${RED}Failed${NC}"
fi


# ============================================================
# Cleanup
# ============================================================

rm -f ./test-files/cmd.txt
rm -f test-output.txt
rm -f test-append.txt
rm -f test-pipe-output.txt

# Restore permission test file
chmod 644 ./test-files/permission_test.txt

make -s clean


# ============================================================
# Final Score
# ============================================================

echo -e "\n========================================"
echo -e "SCORE: ${SCORE}/100"
echo -e "========================================\n"

exit 0
