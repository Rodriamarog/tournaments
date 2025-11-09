#!/bin/bash

set -e  # Exit on error

echo "=========================================="
echo "🏗️  Tournament API - Build & Test Script"
echo "=========================================="
echo ""

# Step 1: Build Docker image
echo "📦 Step 1/4: Building Docker image..."
docker build -f Dockerfile.build -t tournament-build .
echo "✅ Docker image built successfully"
echo ""

# Step 2: Clean old build artifacts
echo "🧹 Step 2/4: Cleaning old build artifacts..."
docker run --rm -v "$(pwd):/workspace" -w /workspace tournament-build rm -rf build
echo "✅ Build directory cleaned"
echo ""

# Step 3: Build the project
echo "🔨 Step 3/4: Building project (this may take a few minutes)..."
docker run --rm -v "$(pwd):/workspace" -w /workspace tournament-build bash -c "
    mkdir -p build && cd build &&
    cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=/vcpkg/scripts/buildsystems/vcpkg.cmake &&
    cmake --build . -j\$(nproc)
"
echo "✅ Project built successfully"
echo ""

# Step 4: Run tests
echo "🧪 Step 4/4: Running all tests..."
echo "=========================================="
docker run --rm --network=host -v "$(pwd):/workspace" -w /workspace tournament-build bash -c "
    cd build &&
    ctest --output-on-failure --verbose
"
echo "=========================================="
echo ""
echo "✅ All done!"
echo ""
echo "Test Summary:"
echo "  - Teams API: 16 tests (8 Controller + 8 Delegate)"
echo "  - Tournaments API: 16 tests (8 Controller + 8 Delegate)"
echo "  - Groups API: 23 tests (11 Controller + 12 Delegate)"
echo "  - Total: 55 tests"
echo ""
