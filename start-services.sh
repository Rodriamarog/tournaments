#!/bin/bash

# Start Tournament Services - Runs the REST API and Consumer

set -e

echo "🚀 Starting Tournament Management System..."
echo ""

# Check if ActiveMQ is running
if ! docker ps | grep -q tournament-activemq; then
    echo "❌ ActiveMQ is not running!"
    echo "Starting ActiveMQ..."
    docker compose up -d activemq
    echo "⏳ Waiting for ActiveMQ to be healthy..."
    sleep 10
fi

echo "✅ ActiveMQ is running"
echo ""

# Start the tournament service (REST API)
echo "📡 Starting Tournament REST API Service..."
docker run --rm -d \
    --name tournament-services \
    --network=host \
    -v "$(pwd)/tournament_services/configuration.json:/app/configuration.json" \
    -v "$(pwd):/workspace" \
    -w /workspace/build/tournament_services \
    tournament-build \
    ./tournament_services

echo "✅ Tournament API started on http://localhost:8080"
echo ""

# Start the tournament consumer (Event processor)
echo "📨 Starting Tournament Event Consumer..."
docker run --rm -d \
    --name tournament-consumer \
    --network=host \
    -v "$(pwd)/tournament_consumer/configuration.json:/app/configuration.json" \
    -v "$(pwd):/workspace" \
    -w /workspace/build/tournament_consumer \
    tournament-build \
    ./tournament_consumer

echo "✅ Tournament Consumer started"
echo ""

# Wait a moment for services to start
sleep 3

# Check if services are running
if curl -s http://localhost:8080/tournaments > /dev/null 2>&1; then
    echo "🎉 All services are running!"
    echo ""
    echo "Services:"
    echo "  - ActiveMQ Web Console: http://localhost:8161 (admin/admin)"
    echo "  - Tournament REST API:  http://localhost:8080"
    echo ""
    echo "To stop services:"
    echo "  docker stop tournament-services tournament-consumer"
    echo "  docker compose down"
else
    echo "❌ Tournament service failed to start"
    echo "Check logs with: docker logs tournament-services"
    exit 1
fi
