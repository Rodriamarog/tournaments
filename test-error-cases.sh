#!/bin/bash

# Tournament API - Error Cases Test Script
# Demonstrates error handling and proper HTTP status codes

BASE_URL="http://localhost:8080"
TIMESTAMP=$(date +%s)

echo "=========================================="
echo "Tournament API - Error Cases Test"
echo "=========================================="
echo ""

# Setup: Create valid resources for error testing
echo "Setup: Creating valid resources for error testing..."
RESPONSE=$(curl -s -i "$BASE_URL/teams" -H "Content-Type: application/json" -d "{\"name\": \"Test Team ${TIMESTAMP}\"}")
TEAM_ID=$(echo "$RESPONSE" | grep -i "^location:" | awk '{print $2}' | tr -d '\r\n')
echo "Created team with ID: $TEAM_ID"

RESPONSE=$(curl -s -i "$BASE_URL/tournaments" -H "Content-Type: application/json" -d "{\"name\": \"Test Tournament ${TIMESTAMP}\"}")
TOURNAMENT_ID=$(echo "$RESPONSE" | grep -i "^location:" | awk '{print $2}' | tr -d '\r\n')
echo "Created tournament with ID: $TOURNAMENT_ID"

RESPONSE=$(curl -s -i "$BASE_URL/tournaments/$TOURNAMENT_ID/groups" -H "Content-Type: application/json" -d "{\"name\": \"Test Group ${TIMESTAMP}\"}")
GROUP_ID=$(echo "$RESPONSE" | grep -i "^location:" | awk '{print $2}' | tr -d '\r\n')
echo "Created group with ID: $GROUP_ID"
echo ""

echo "=========================================="
echo "Testing Error Responses"
echo "=========================================="
echo ""

# Error Test 1: Missing Required Field
echo "Error Test 1: Create Team - Missing Required Field"
echo "POST $BASE_URL/teams"
echo "Body: {}"
RESPONSE=$(curl -s -w "\nHTTP_STATUS:%{http_code}" "$BASE_URL/teams" \
  -H "Content-Type: application/json" -d '{}')
HTTP_STATUS=$(echo "$RESPONSE" | grep "HTTP_STATUS" | cut -d: -f2)
BODY=$(echo "$RESPONSE" | sed '/HTTP_STATUS/d')
echo "Response: $BODY"
echo "HTTP Status: $HTTP_STATUS (Expected: 400)"
echo ""

# Error Test 2: Duplicate Team
echo "Error Test 2: Create Duplicate Team"
echo "POST $BASE_URL/teams"
RESPONSE=$(curl -s -w "\nHTTP_STATUS:%{http_code}" "$BASE_URL/teams" \
  -H "Content-Type: application/json" -d "{\"name\": \"Test Team ${TIMESTAMP}\"}")
HTTP_STATUS=$(echo "$RESPONSE" | grep "HTTP_STATUS" | cut -d: -f2)
BODY=$(echo "$RESPONSE" | sed '/HTTP_STATUS/d')
echo "Response: $BODY"
echo "HTTP Status: $HTTP_STATUS (Expected: 409)"
echo ""

# Error Test 3: Get Non-Existent Team
echo "Error Test 3: Get Non-Existent Team"
echo "GET $BASE_URL/teams/00000000-0000-0000-0000-000000000000"
RESPONSE=$(curl -s -w "\nHTTP_STATUS:%{http_code}" "$BASE_URL/teams/00000000-0000-0000-0000-000000000000")
HTTP_STATUS=$(echo "$RESPONSE" | grep "HTTP_STATUS" | cut -d: -f2)
BODY=$(echo "$RESPONSE" | sed '/HTTP_STATUS/d')
echo "Response: $BODY"
echo "HTTP Status: $HTTP_STATUS (Expected: 404)"
echo ""

# Error Test 4: Update Non-Existent Team
echo "Error Test 4: Update Non-Existent Team (PATCH)"
echo "PATCH $BASE_URL/teams/00000000-0000-0000-0000-000000000000"
RESPONSE=$(curl -s -w "\nHTTP_STATUS:%{http_code}" -X PATCH "$BASE_URL/teams/00000000-0000-0000-0000-000000000000" \
  -H "Content-Type: application/json" -d '{"name": "New Name"}')
HTTP_STATUS=$(echo "$RESPONSE" | grep "HTTP_STATUS" | cut -d: -f2)
BODY=$(echo "$RESPONSE" | sed '/HTTP_STATUS/d')
echo "Response: $BODY"
echo "HTTP Status: $HTTP_STATUS (Expected: 404)"
echo ""

# Error Test 5: Tournament Missing Required Field
echo "Error Test 5: Create Tournament - Missing Required Field"
echo "POST $BASE_URL/tournaments"
echo "Body: {}"
RESPONSE=$(curl -s -w "\nHTTP_STATUS:%{http_code}" "$BASE_URL/tournaments" \
  -H "Content-Type: application/json" -d '{}')
HTTP_STATUS=$(echo "$RESPONSE" | grep "HTTP_STATUS" | cut -d: -f2)
BODY=$(echo "$RESPONSE" | sed '/HTTP_STATUS/d')
echo "Response: $BODY"
echo "HTTP Status: $HTTP_STATUS (Expected: 400)"
echo ""

# Error Test 6: Duplicate Tournament
echo "Error Test 6: Create Duplicate Tournament"
echo "POST $BASE_URL/tournaments"
RESPONSE=$(curl -s -w "\nHTTP_STATUS:%{http_code}" "$BASE_URL/tournaments" \
  -H "Content-Type: application/json" -d "{\"name\": \"Test Tournament ${TIMESTAMP}\"}")
HTTP_STATUS=$(echo "$RESPONSE" | grep "HTTP_STATUS" | cut -d: -f2)
BODY=$(echo "$RESPONSE" | sed '/HTTP_STATUS/d')
echo "Response: $BODY"
echo "HTTP Status: $HTTP_STATUS (Expected: 409)"
echo ""

# Error Test 7: Get Non-Existent Tournament
echo "Error Test 7: Get Non-Existent Tournament"
echo "GET $BASE_URL/tournaments/00000000-0000-0000-0000-000000000000"
RESPONSE=$(curl -s -w "\nHTTP_STATUS:%{http_code}" "$BASE_URL/tournaments/00000000-0000-0000-0000-000000000000")
HTTP_STATUS=$(echo "$RESPONSE" | grep "HTTP_STATUS" | cut -d: -f2)
BODY=$(echo "$RESPONSE" | sed '/HTTP_STATUS/d')
echo "Response: $BODY"
echo "HTTP Status: $HTTP_STATUS (Expected: 404)"
echo ""

# Error Test 8: Group Missing Required Field
echo "Error Test 8: Create Group - Missing Required Field"
echo "POST $BASE_URL/tournaments/$TOURNAMENT_ID/groups"
echo "Body: {}"
RESPONSE=$(curl -s -w "\nHTTP_STATUS:%{http_code}" "$BASE_URL/tournaments/$TOURNAMENT_ID/groups" \
  -H "Content-Type: application/json" -d '{}')
HTTP_STATUS=$(echo "$RESPONSE" | grep "HTTP_STATUS" | cut -d: -f2)
BODY=$(echo "$RESPONSE" | sed '/HTTP_STATUS/d')
echo "Response: $BODY"
echo "HTTP Status: $HTTP_STATUS (Expected: 400)"
echo ""

# Error Test 9: Duplicate Group
echo "Error Test 9: Create Duplicate Group in Same Tournament"
echo "POST $BASE_URL/tournaments/$TOURNAMENT_ID/groups"
RESPONSE=$(curl -s -w "\nHTTP_STATUS:%{http_code}" "$BASE_URL/tournaments/$TOURNAMENT_ID/groups" \
  -H "Content-Type: application/json" -d "{\"name\": \"Test Group ${TIMESTAMP}\"}")
HTTP_STATUS=$(echo "$RESPONSE" | grep "HTTP_STATUS" | cut -d: -f2)
BODY=$(echo "$RESPONSE" | sed '/HTTP_STATUS/d')
echo "Response: $BODY"
echo "HTTP Status: $HTTP_STATUS (Expected: 409)"
echo ""

# Error Test 10: Get Non-Existent Group
echo "Error Test 10: Get Non-Existent Group"
echo "GET $BASE_URL/tournaments/$TOURNAMENT_ID/groups/00000000-0000-0000-0000-000000000000"
RESPONSE=$(curl -s -w "\nHTTP_STATUS:%{http_code}" "$BASE_URL/tournaments/$TOURNAMENT_ID/groups/00000000-0000-0000-0000-000000000000")
HTTP_STATUS=$(echo "$RESPONSE" | grep "HTTP_STATUS" | cut -d: -f2)
BODY=$(echo "$RESPONSE" | sed '/HTTP_STATUS/d')
echo "Response: $BODY"
echo "HTTP Status: $HTTP_STATUS (Expected: 404)"
echo ""

# Error Test 11: Add Team to Group - Missing ID Field
echo "Error Test 11: Add Team to Group - Missing ID Field"
echo "POST $BASE_URL/tournaments/$TOURNAMENT_ID/groups/$GROUP_ID"
echo "Body: {}"
RESPONSE=$(curl -s -w "\nHTTP_STATUS:%{http_code}" "$BASE_URL/tournaments/$TOURNAMENT_ID/groups/$GROUP_ID" \
  -H "Content-Type: application/json" -d '{}')
HTTP_STATUS=$(echo "$RESPONSE" | grep "HTTP_STATUS" | cut -d: -f2)
BODY=$(echo "$RESPONSE" | sed '/HTTP_STATUS/d')
echo "Response: $BODY"
echo "HTTP Status: $HTTP_STATUS (Expected: 400)"
echo ""

# Error Test 12: Add Non-Existent Team to Group
echo "Error Test 12: Add Non-Existent Team to Group"
echo "POST $BASE_URL/tournaments/$TOURNAMENT_ID/groups/$GROUP_ID"
RESPONSE=$(curl -s -w "\nHTTP_STATUS:%{http_code}" "$BASE_URL/tournaments/$TOURNAMENT_ID/groups/$GROUP_ID" \
  -H "Content-Type: application/json" -d '{"id": "00000000-0000-0000-0000-000000000000"}')
HTTP_STATUS=$(echo "$RESPONSE" | grep "HTTP_STATUS" | cut -d: -f2)
BODY=$(echo "$RESPONSE" | sed '/HTTP_STATUS/d')
echo "Response: $BODY"
echo "HTTP Status: $HTTP_STATUS (Expected: 422)"
echo ""

# Error Test 13: Invalid JSON Syntax
echo "Error Test 13: Invalid JSON Syntax"
echo "POST $BASE_URL/teams"
echo "Body: {invalid json}"
RESPONSE=$(curl -s -w "\nHTTP_STATUS:%{http_code}" "$BASE_URL/teams" \
  -H "Content-Type: application/json" -d '{invalid json}')
HTTP_STATUS=$(echo "$RESPONSE" | grep "HTTP_STATUS" | cut -d: -f2)
BODY=$(echo "$RESPONSE" | sed '/HTTP_STATUS/d')
echo "Response: $BODY"
echo "HTTP Status: $HTTP_STATUS (Expected: 400)"
echo ""

echo "=========================================="
echo "Error Testing Completed!"
echo "=========================================="
echo ""
echo "HTTP Error Codes Demonstrated:"
echo "  - 400 Bad Request: Missing required fields, invalid JSON"
echo "  - 404 Not Found: Resource doesn't exist"
echo "  - 409 Conflict: Duplicate resources"
echo "  - 422 Unprocessable Entity: Business rule violations"
echo ""
