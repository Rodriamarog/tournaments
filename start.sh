#!/bin/bash

echo "Starting Tournaments server..."

# Check if container image exists
if ! docker images | grep -q "tournament-build"; then
    echo "Container 'tournament-build' not found!"
    echo "Please build it first with: ./build-and-test.sh"
    exit 1
fi

echo "Running container with Supabase connectivity..."
echo "Server will be available at: http://localhost:8080"
echo "API endpoints:"
echo "   - GET/POST http://localhost:8080/teams"
echo "   - GET/POST http://localhost:8080/tournaments"
echo "   - GET/POST http://localhost:8080/groups"
echo ""
echo "Press Ctrl+C to stop the server"
echo ""

# Run the container
docker run --rm --network host -v "$(pwd):/workspace" -w /workspace tournament-build bash -c "cd build/tournament_services && ./tournament_services"