#!/bin/bash

echo "Starting Tournaments server..."

# Check if container image exists
if ! docker images | grep -q "tournaments.*dev"; then
    echo "Container 'tournaments:dev' not found!"
    echo "Please build it first with: docker build -t tournaments:dev -f Dockerfile.dev ."
    exit 1
fi

echo "Running container with Supabase connectivity..."
echo "Server will be available at: http://localhost:8080"
echo "API endpoints:"
echo "   - GET/POST http://localhost:8080/teams"
echo "   - GET/POST http://localhost:8080/tournaments"
echo ""
echo "Press Ctrl+C to stop the server"
echo ""

# Run the container
docker run --rm --network host tournaments:dev bash -c "cd /workspace && mkdir -p build && cd build && cmake .. && make && cd tournament_services && ./tournament_services"