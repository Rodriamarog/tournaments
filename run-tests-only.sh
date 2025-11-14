#!/bin/bash

echo "🧪 Running tests (no rebuild)..."
echo "=========================================="
docker run --rm --network=host -v "$(pwd):/workspace" -w /workspace tournament-build bash -c "
    cd build/tournament_services/tests &&
    ./tournament_tests_runner
"
echo "=========================================="
