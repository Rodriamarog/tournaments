#!/bin/bash

set -e  # Exit on error

echo "=========================================="
echo "🔨 Incremental Test Rebuild"
echo "=========================================="
echo ""

# Build only the tests (incremental)
echo "Building tests incrementally..."
docker run --rm -v "$(pwd):/workspace" -w /workspace tournament-build bash -c "
    cd build &&
    cmake --build . --target tournament_tests_runner -j\$(nproc)
"
echo "✅ Tests built successfully"
echo ""

# Run tests
echo "🧪 Running tests..."
echo "=========================================="
docker run --rm --network=host -v "$(pwd):/workspace" -w /workspace tournament-build bash -c "
    cd build/tournament_services/tests &&
    ./tournament_tests_runner
"
echo "=========================================="
