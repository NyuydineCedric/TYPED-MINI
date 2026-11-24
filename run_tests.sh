#!/bin/bash

echo "===================================="
echo "       TYPED-MINI TEST SUITE"
echo "===================================="

passed=0
failed=0

echo ""
echo "Running valid tests..."

for file in tests/valid/*.tmini; do
    if ./typed-mini.exe "$file" > /dev/null 2>&1; then
        echo "[PASS] $file"
        ((passed++))
    else
        echo "[FAIL] $file"
        ((failed++))
    fi
done

echo ""
echo "Running invalid tests..."

for file in tests/invalid/*.tmini; do
    if ./typed-mini.exe "$file" > /dev/null 2>&1; then
        echo "[FAIL] $file (compiler accepted invalid program)"
        ((failed++))
    else
        echo "[PASS] $file (correctly rejected)"
        ((passed++))
    fi
done

echo ""
echo "===================================="
echo "Tests passed: $passed"
echo "Tests failed: $failed"
echo "===================================="

if [ "$failed" -gt 0 ]; then
    exit 1
fi
