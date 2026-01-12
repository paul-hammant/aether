#!/usr/bin/env bash
# Test: apkg package manager functionality
# This test verifies basic apkg commands work correctly

echo "Testing apkg package manager..."

# Create test directory
TEST_DIR="test_apkg_project"
if [ -d "$TEST_DIR" ]; then
    rm -rf "$TEST_DIR"
fi
mkdir -p "$TEST_DIR"

# Test 1: apkg init
echo ""
echo "[Test 1] apkg init..."
cd "$TEST_DIR"
../apkg init myproject
INIT_RESULT=$?
cd ..

if [ $INIT_RESULT -eq 0 ] && [ -f "$TEST_DIR/aether.toml" ]; then
    echo "PASS: apkg init creates project"
else
    echo "FAIL: apkg init failed"
    rm -rf "$TEST_DIR"
    exit 1
fi

# Test 2: Check generated files
echo ""
echo "[Test 2] Generated files..."
EXPECTED_FILES=("aether.toml" "src/main.ae" "README.md")
ALL_EXIST=true

for FILE in "${EXPECTED_FILES[@]}"; do
    if [ ! -f "$TEST_DIR/$FILE" ]; then
        echo "FAIL: Missing $FILE"
        ALL_EXIST=false
    fi
done

if [ "$ALL_EXIST" = true ]; then
    echo "PASS: All expected files created"
else
    rm -rf "$TEST_DIR"
    exit 1
fi

# Test 3: apkg help
echo ""
echo "[Test 3] apkg help..."
if ./apkg help > /dev/null 2>&1; then
    echo "PASS: apkg help works"
else
    echo "FAIL: apkg help failed"
    rm -rf "$TEST_DIR"
    exit 1
fi

# Cleanup
echo ""
echo "Cleaning up test directory..."
rm -rf "$TEST_DIR"

echo ""
echo "All apkg tests passed!"
exit 0
