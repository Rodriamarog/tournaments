#!/bin/bash
set -x

echo "Current directory: $(pwd)"
echo "Configuration file:"
cat configuration.json
echo ""
echo "Starting service..."

# Try to run the service and capture any output
./tournament_services 2>&1 || {
    EXIT_CODE=$?
    echo "Service exited with code: $EXIT_CODE"
    if [ $EXIT_CODE -eq 139 ]; then
        echo "Segmentation fault detected"
    fi
    exit $EXIT_CODE
}
