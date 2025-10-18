#!/bin/bash

# Tournament API - Valid API Calls Test Script
# Demonstrates successful operations with proper responses

BASE_URL="http://localhost:8080"
TIMESTAMP=$(date +%s)

echo "=========================================="
echo "Tournament API - Valid Calls Test"
echo "=========================================="
echo ""

# Test 1: Create Team (Real Madrid)
echo "Test 1: Create Team (Real Madrid)"
echo "POST $BASE_URL/teams"
RESPONSE=$(curl -s -i "$BASE_URL/teams" -H "Content-Type: application/json" -d "{\"name\": \"Real Madrid ${TIMESTAMP}\"}")
TEAM_ID=$(echo "$RESPONSE" | grep -i "^location:" | awk '{print $2}' | tr -d '\r\n')
echo "✓ Team created with ID: $TEAM_ID"
echo ""

# Test 2: Create Team (Barcelona)
echo "Test 2: Create Team (Barcelona)"
echo "POST $BASE_URL/teams"
RESPONSE=$(curl -s -i "$BASE_URL/teams" -H "Content-Type: application/json" -d "{\"name\": \"Barcelona ${TIMESTAMP}\"}")
TEAM2_ID=$(echo "$RESPONSE" | grep -i "^location:" | awk '{print $2}' | tr -d '\r\n')
echo "✓ Team created with ID: $TEAM2_ID"
echo ""

# Test 3: Get All Teams
echo "Test 3: Get All Teams"
echo "GET $BASE_URL/teams"
curl -s "$BASE_URL/teams" | jq '.'
echo "✓ Retrieved all teams"
echo ""

# Test 4: Get Team by ID
echo "Test 4: Get Team by ID"
echo "GET $BASE_URL/teams/$TEAM_ID"
curl -s "$BASE_URL/teams/$TEAM_ID" | jq '.'
echo "✓ Retrieved team by ID"
echo ""

# Test 5: Update Team Name
echo "Test 5: Update Team Name (PATCH)"
echo "PATCH $BASE_URL/teams/$TEAM_ID"
curl -s -X PATCH "$BASE_URL/teams/$TEAM_ID" \
  -H "Content-Type: application/json" \
  -d "{\"name\": \"Real Madrid CF ${TIMESTAMP}\"}"
echo ""
echo "✓ Team updated"
echo ""

# Test 6: Verify Team Was Updated
echo "Test 6: Verify Team Was Updated"
echo "GET $BASE_URL/teams/$TEAM_ID"
curl -s "$BASE_URL/teams/$TEAM_ID" | jq '.'
echo "✓ Verified team name changed"
echo ""

# Test 7: Create Tournament
echo "Test 7: Create Tournament"
echo "POST $BASE_URL/tournaments"
RESPONSE=$(curl -s -i "$BASE_URL/tournaments" -H "Content-Type: application/json" -d "{\"name\": \"Champions League ${TIMESTAMP}\"}")
TOURNAMENT_ID=$(echo "$RESPONSE" | grep -i "^location:" | awk '{print $2}' | tr -d '\r\n')
echo "✓ Tournament created with ID: $TOURNAMENT_ID"
echo ""

# Test 8: Get All Tournaments
echo "Test 8: Get All Tournaments"
echo "GET $BASE_URL/tournaments"
curl -s "$BASE_URL/tournaments" | jq '.'
echo "✓ Retrieved all tournaments"
echo ""

# Test 9: Get Tournament by ID
echo "Test 9: Get Tournament by ID"
echo "GET $BASE_URL/tournaments/$TOURNAMENT_ID"
curl -s "$BASE_URL/tournaments/$TOURNAMENT_ID" | jq '.'
echo "✓ Retrieved tournament by ID"
echo ""

# Test 10: Update Tournament
echo "Test 10: Update Tournament (PATCH)"
echo "PATCH $BASE_URL/tournaments/$TOURNAMENT_ID"
curl -s -X PATCH "$BASE_URL/tournaments/$TOURNAMENT_ID" \
  -H "Content-Type: application/json" \
  -d "{\"name\": \"UEFA Champions League ${TIMESTAMP}\"}"
echo ""
echo "✓ Tournament updated"
echo ""

# Test 11: Create Group
echo "Test 11: Create Group"
echo "POST $BASE_URL/tournaments/$TOURNAMENT_ID/groups"
RESPONSE=$(curl -s -i "$BASE_URL/tournaments/$TOURNAMENT_ID/groups" -H "Content-Type: application/json" -d "{\"name\": \"Group A ${TIMESTAMP}\"}")
GROUP_ID=$(echo "$RESPONSE" | grep -i "^location:" | awk '{print $2}' | tr -d '\r\n')
echo "✓ Group created with ID: $GROUP_ID"
echo ""

# Test 12: Get All Groups in Tournament
echo "Test 12: Get All Groups in Tournament"
echo "GET $BASE_URL/tournaments/$TOURNAMENT_ID/groups"
curl -s "$BASE_URL/tournaments/$TOURNAMENT_ID/groups" | jq '.'
echo "✓ Retrieved all groups"
echo ""

# Test 13: Get Specific Group
echo "Test 13: Get Specific Group"
echo "GET $BASE_URL/tournaments/$TOURNAMENT_ID/groups/$GROUP_ID"
curl -s "$BASE_URL/tournaments/$TOURNAMENT_ID/groups/$GROUP_ID" | jq '.'
echo "✓ Retrieved specific group"
echo ""

# Test 14: Update Group Name
echo "Test 14: Update Group Name (PATCH)"
echo "PATCH $BASE_URL/tournaments/$TOURNAMENT_ID/groups/$GROUP_ID"
curl -s -X PATCH "$BASE_URL/tournaments/$TOURNAMENT_ID/groups/$GROUP_ID" \
  -H "Content-Type: application/json" \
  -d "{\"name\": \"Group A - Elite ${TIMESTAMP}\"}"
echo ""
echo "✓ Group updated"
echo ""

# Test 15: Add Real Madrid to Group
echo "Test 15: Add Real Madrid to Group"
echo "POST $BASE_URL/tournaments/$TOURNAMENT_ID/groups/$GROUP_ID"
curl -s "$BASE_URL/tournaments/$TOURNAMENT_ID/groups/$GROUP_ID" \
  -H "Content-Type: application/json" \
  -d "{\"id\": \"$TEAM_ID\"}" | jq '.'
echo "✓ Team added to group"
echo ""

# Test 16: Add Barcelona to Group
echo "Test 16: Add Barcelona to Group"
echo "POST $BASE_URL/tournaments/$TOURNAMENT_ID/groups/$GROUP_ID"
curl -s "$BASE_URL/tournaments/$TOURNAMENT_ID/groups/$GROUP_ID" \
  -H "Content-Type: application/json" \
  -d "{\"id\": \"$TEAM2_ID\"}" | jq '.'
echo "✓ Second team added to group"
echo ""

# Test 17: Get Group with Teams
echo "Test 17: Get Group with Teams"
echo "GET $BASE_URL/tournaments/$TOURNAMENT_ID/groups/$GROUP_ID"
curl -s "$BASE_URL/tournaments/$TOURNAMENT_ID/groups/$GROUP_ID" | jq '.'
echo "✓ Retrieved group with teams"
echo ""

echo "=========================================="
echo "All Valid Tests Completed Successfully!"
echo "=========================================="
echo ""
echo "Summary:"
echo "  - Teams: Real Madrid CF, Barcelona"
echo "  - Tournament: UEFA Champions League"
echo "  - Group: Group A - Elite (with 2 teams)"
echo ""
echo "HTTP Methods demonstrated:"
echo "  - GET: Retrieve resources"
echo "  - POST: Create resources"
echo "  - PATCH: Update resources"
echo ""
