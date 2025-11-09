#!/bin/bash

# test-match-flow.sh
# Integration test script for Match System - demonstrates complete tournament flow
#
# Flow:
# 1. Create tournament (4 groups, 4 teams per group)
# 2. Add teams to groups (triggers round-robin match generation via ActiveMQ)
# 3. Register scores for all group matches (triggers quarterfinals generation)
# 4. Register scores for quarterfinals (triggers semifinals generation)
# 5. Register scores for semifinals (triggers finals generation)
# 6. Register final match score
# 7. Verify complete tournament bracket

set -e  # Exit on error

# Configuration
API_BASE="http://localhost:8080"
TOURNAMENT_ID="test-tournament-$(date +%s)"

echo "========================================="
echo "  Match System Integration Test"
echo "========================================="
echo "Tournament ID: $TOURNAMENT_ID"
echo ""

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Helper function for API calls
api_call() {
    local method=$1
    local endpoint=$2
    local data=$3

    if [ -z "$data" ]; then
        curl -s -X "$method" "$API_BASE$endpoint"
    else
        curl -s -X "$method" "$API_BASE$endpoint" \
            -H "Content-Type: application/json" \
            -d "$data"
    fi
}

# Step 1: Create Tournament
echo -e "${BLUE}Step 1: Creating tournament with 4 groups, 4 teams per group...${NC}"
TOURNAMENT_DATA='{
  "id": "'$TOURNAMENT_ID'",
  "name": "Match System Test Tournament",
  "format": {
    "type": "round-robin-with-playoffs",
    "numberOfGroups": 4,
    "maxTeamsPerGroup": 4
  }
}'

TOURNAMENT_RESULT=$(api_call POST "/api/tournaments" "$TOURNAMENT_DATA")
echo -e "${GREEN}✓ Tournament created${NC}"
echo ""

# Step 2: Create Groups
echo -e "${BLUE}Step 2: Creating groups A, B, C, D...${NC}"
GROUPS=("A" "B" "C" "D")
for GROUP in "${GROUPS[@]}"; do
    GROUP_DATA='{
      "tournamentId": "'$TOURNAMENT_ID'",
      "name": "Group '$GROUP'"
    }'
    api_call POST "/api/groups" "$GROUP_DATA" > /dev/null
    echo -e "${GREEN}✓ Group $GROUP created${NC}"
done
echo ""

# Step 3: Add Teams to Groups (triggers round-robin match generation)
echo -e "${BLUE}Step 3: Adding teams to groups (triggers match generation via ActiveMQ)...${NC}"
declare -A TEAM_IDS

for GROUP in "${GROUPS[@]}"; do
    echo -e "${YELLOW}Group $GROUP:${NC}"
    for i in {1..4}; do
        TEAM_ID="team-${GROUP}-${i}"
        TEAM_IDS["${GROUP}-${i}"]=$TEAM_ID

        TEAM_DATA='{
          "id": "'$TEAM_ID'",
          "name": "Team '$GROUP$i'",
          "country": "Country '$GROUP$i'"
        }'

        # Create team
        api_call POST "/api/teams" "$TEAM_DATA" > /dev/null

        # Add team to group (sends ActiveMQ event)
        ADD_TEAM_DATA='{
          "tournamentId": "'$TOURNAMENT_ID'",
          "groupId": "Group '$GROUP'",
          "teamId": "'$TEAM_ID'"
        }'
        api_call POST "/api/groups/add-team" "$ADD_TEAM_DATA" > /dev/null

        echo "  ✓ Team $GROUP$i added"

        # Wait a bit for ActiveMQ consumer to process
        if [ $i -eq 4 ]; then
            echo -e "  ${YELLOW}⏳ Waiting for match generation...${NC}"
            sleep 2
        fi
    done
done
echo ""

# Step 4: Verify Round-Robin Matches Generated
echo -e "${BLUE}Step 4: Verifying round-robin matches were generated...${NC}"
MATCHES=$(api_call GET "/api/tournaments/$TOURNAMENT_ID/matches")
MATCH_COUNT=$(echo "$MATCHES" | jq '. | length')
EXPECTED_MATCHES=24  # 4 groups * 6 matches per group (4 teams = C(4,2) = 6 matches)

echo "Generated matches: $MATCH_COUNT (expected: $EXPECTED_MATCHES)"
if [ "$MATCH_COUNT" -eq "$EXPECTED_MATCHES" ]; then
    echo -e "${GREEN}✓ All round-robin matches generated correctly${NC}"
else
    echo -e "${RED}✗ Match count mismatch!${NC}"
    exit 1
fi
echo ""

# Step 5: Register Scores for All Group Stage Matches
echo -e "${BLUE}Step 5: Registering scores for all group stage matches...${NC}"
GROUP_MATCHES=$(echo "$MATCHES" | jq -r '.[] | select(.round == "regular") | .id')

MATCH_NUM=1
for MATCH_ID in $GROUP_MATCHES; do
    # Generate random scores (home wins more often to create clear standings)
    if [ $((RANDOM % 10)) -lt 7 ]; then
        # Home team wins
        HOME_SCORE=$((2 + RANDOM % 3))
        VISITOR_SCORE=$((RANDOM % 2))
    else
        # Visitor wins
        HOME_SCORE=$((RANDOM % 2))
        VISITOR_SCORE=$((2 + RANDOM % 3))
    fi

    SCORE_DATA='{
      "home": '$HOME_SCORE',
      "visitor": '$VISITOR_SCORE'
    }'

    api_call PUT "/api/matches/$MATCH_ID/score" "$SCORE_DATA" > /dev/null

    if [ $((MATCH_NUM % 6)) -eq 0 ]; then
        echo -e "${GREEN}✓ Completed group $((MATCH_NUM / 6)) matches${NC}"
    fi

    MATCH_NUM=$((MATCH_NUM + 1))
done

echo -e "${YELLOW}⏳ Waiting for quarterfinals generation...${NC}"
sleep 3
echo ""

# Step 6: Verify Quarterfinals Generated
echo -e "${BLUE}Step 6: Verifying quarterfinals were generated...${NC}"
PLAYOFF_MATCHES=$(api_call GET "/api/tournaments/$TOURNAMENT_ID/matches")
QF_MATCHES=$(echo "$PLAYOFF_MATCHES" | jq '[.[] | select(.round == "quarterfinals")] | length')

echo "Quarterfinal matches: $QF_MATCHES (expected: 4)"
if [ "$QF_MATCHES" -eq 4 ]; then
    echo -e "${GREEN}✓ Quarterfinals generated correctly${NC}"
else
    echo -e "${RED}✗ Quarterfinals not generated!${NC}"
    exit 1
fi

# Display quarterfinal matchups
echo -e "${YELLOW}Quarterfinal Matchups:${NC}"
echo "$PLAYOFF_MATCHES" | jq -r '.[] | select(.round == "quarterfinals") | "  " + .home.name + " vs " + .visitor.name'
echo ""

# Step 7: Register Quarterfinal Scores
echo -e "${BLUE}Step 7: Registering quarterfinal scores...${NC}"
QF_MATCH_IDS=$(echo "$PLAYOFF_MATCHES" | jq -r '.[] | select(.round == "quarterfinals") | .id')

for MATCH_ID in $QF_MATCH_IDS; do
    # Home team wins by 1 goal
    SCORE_DATA='{"home": 2, "visitor": 1}'
    api_call PUT "/api/matches/$MATCH_ID/score" "$SCORE_DATA" > /dev/null
    echo -e "${GREEN}✓ QF match scored${NC}"
done

echo -e "${YELLOW}⏳ Waiting for semifinals generation...${NC}"
sleep 3
echo ""

# Step 8: Verify Semifinals Generated
echo -e "${BLUE}Step 8: Verifying semifinals were generated...${NC}"
PLAYOFF_MATCHES=$(api_call GET "/api/tournaments/$TOURNAMENT_ID/matches")
SF_MATCHES=$(echo "$PLAYOFF_MATCHES" | jq '[.[] | select(.round == "semifinals")] | length')

echo "Semifinal matches: $SF_MATCHES (expected: 2)"
if [ "$SF_MATCHES" -eq 2 ]; then
    echo -e "${GREEN}✓ Semifinals generated correctly${NC}"
else
    echo -e "${RED}✗ Semifinals not generated!${NC}"
    exit 1
fi

# Display semifinal matchups
echo -e "${YELLOW}Semifinal Matchups:${NC}"
echo "$PLAYOFF_MATCHES" | jq -r '.[] | select(.round == "semifinals") | "  " + .home.name + " vs " + .visitor.name'
echo ""

# Step 9: Register Semifinal Scores
echo -e "${BLUE}Step 9: Registering semifinal scores...${NC}"
SF_MATCH_IDS=$(echo "$PLAYOFF_MATCHES" | jq -r '.[] | select(.round == "semifinals") | .id')

for MATCH_ID in $SF_MATCH_IDS; do
    SCORE_DATA='{"home": 3, "visitor": 1}'
    api_call PUT "/api/matches/$MATCH_ID/score" "$SCORE_DATA" > /dev/null
    echo -e "${GREEN}✓ SF match scored${NC}"
done

echo -e "${YELLOW}⏳ Waiting for finals generation...${NC}"
sleep 3
echo ""

# Step 10: Verify Finals Generated
echo -e "${BLUE}Step 10: Verifying finals were generated...${NC}"
PLAYOFF_MATCHES=$(api_call GET "/api/tournaments/$TOURNAMENT_ID/matches")
FINAL_MATCHES=$(echo "$PLAYOFF_MATCHES" | jq '[.[] | select(.round == "finals")] | length')

echo "Final matches: $FINAL_MATCHES (expected: 1)"
if [ "$FINAL_MATCHES" -eq 1 ]; then
    echo -e "${GREEN}✓ Finals generated correctly${NC}"
else
    echo -e "${RED}✗ Finals not generated!${NC}"
    exit 1
fi

# Display final matchup
echo -e "${YELLOW}Final Matchup:${NC}"
echo "$PLAYOFF_MATCHES" | jq -r '.[] | select(.round == "finals") | "  " + .home.name + " vs " + .visitor.name'
echo ""

# Step 11: Register Final Score
echo -e "${BLUE}Step 11: Registering final score...${NC}"
FINAL_MATCH_ID=$(echo "$PLAYOFF_MATCHES" | jq -r '.[] | select(.round == "finals") | .id')

SCORE_DATA='{"home": 2, "visitor": 1}'
api_call PUT "/api/matches/$FINAL_MATCH_ID/score" "$SCORE_DATA" > /dev/null
echo -e "${GREEN}✓ Final match scored${NC}"
echo ""

# Step 12: Display Tournament Summary
echo -e "${BLUE}Step 12: Tournament Summary${NC}"
echo "========================================="

FINAL_MATCHES=$(api_call GET "/api/tournaments/$TOURNAMENT_ID/matches")

echo -e "${YELLOW}Round Robin Matches:${NC} $(echo "$FINAL_MATCHES" | jq '[.[] | select(.round == "regular")] | length')"
echo -e "${YELLOW}Quarterfinals:${NC} $(echo "$FINAL_MATCHES" | jq '[.[] | select(.round == "quarterfinals")] | length')"
echo -e "${YELLOW}Semifinals:${NC} $(echo "$FINAL_MATCHES" | jq '[.[] | select(.round == "semifinals")] | length')"
echo -e "${YELLOW}Finals:${NC} $(echo "$FINAL_MATCHES" | jq '[.[] | select(.round == "finals")] | length')"

echo ""
echo -e "${YELLOW}Tournament Winner:${NC}"
FINAL_MATCH=$(echo "$FINAL_MATCHES" | jq -r '.[] | select(.round == "finals")')
WINNER=$(echo "$FINAL_MATCH" | jq -r 'if .score.home > .score.visitor then .home.name else .visitor.name end')
FINAL_SCORE=$(echo "$FINAL_MATCH" | jq -r '(.home.name) + " " + (.score.home | tostring) + " - " + (.score.visitor | tostring) + " " + (.visitor.name)')

echo -e "${GREEN}🏆 $WINNER${NC}"
echo -e "   Final Score: $FINAL_SCORE"

echo ""
echo "========================================="
echo -e "${GREEN}✓ All integration tests passed!${NC}"
echo "========================================="
