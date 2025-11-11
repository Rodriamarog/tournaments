#!/bin/bash

# Tournament Management System - End-to-End Demo
# This script demonstrates a complete tournament flow from creation to winner

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Configuration
API_URL="http://localhost:8080"
TOURNAMENT_ID=""
TEAM_IDS=()
GROUP_IDS=()

# Function to wait for user
wait_for_user() {
    echo ""
    echo -e "${YELLOW}Press ENTER to continue to next step...${NC}"
    read
}

# Function to print step header
print_step() {
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}$1${NC}"
    echo -e "${GREEN}========================================${NC}"
}

# Function to print info
print_info() {
    echo -e "${BLUE}ℹ  $1${NC}"
}

# Function to print success
print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

# Function to print error
print_error() {
    echo -e "${RED}✗ $1${NC}"
}

# Check if service is running
check_service() {
    print_step "Step 0: Checking if tournament service is running"
    if curl -s "$API_URL/health" > /dev/null 2>&1; then
        print_success "Tournament service is running at $API_URL"
    else
        print_error "Tournament service is NOT running!"
        echo -e "${YELLOW}Please start the service first:${NC}"
        echo "  ./tournament_services/tournament_services"
        exit 1
    fi
    wait_for_user
}

# Step 1: Create Tournament
create_tournament() {
    print_step "Step 1: Creating a Round-Robin Tournament"
    print_info "Tournament Details:"
    print_info "  - Name: 'World Cup 2025'"
    print_info "  - Format: Round Robin"
    print_info "  - 4 Groups"
    print_info "  - 4 Teams per group (16 teams total)"

    RESPONSE=$(curl -s -X POST "$API_URL/tournaments" \
        -H "Content-Type: application/json" \
        -d '{
            "name": "World Cup 2025",
            "format": {
                "type": "round-robin",
                "numberOfGroups": 4,
                "maxTeamsPerGroup": 4
            }
        }')

    TOURNAMENT_ID=$(echo $RESPONSE | grep -o '"id":"[^"]*"' | cut -d'"' -f4)

    if [ -z "$TOURNAMENT_ID" ]; then
        print_error "Failed to create tournament"
        echo "Response: $RESPONSE"
        exit 1
    fi

    print_success "Tournament created!"
    print_info "Tournament ID: $TOURNAMENT_ID"
    echo ""
    print_info "Go to Supabase and check the 'tournaments' table"
    print_info "You should see: World Cup 2025"

    wait_for_user
}

# Step 2: Create Teams
create_teams() {
    print_step "Step 2: Creating 16 Teams (4 per group)"

    TEAM_NAMES=(
        "Brazil" "Argentina" "Germany" "France"
        "Spain" "Italy" "England" "Portugal"
        "Netherlands" "Belgium" "Uruguay" "Croatia"
        "Mexico" "Colombia" "Japan" "South Korea"
    )

    print_info "Creating teams:"
    for TEAM_NAME in "${TEAM_NAMES[@]}"; do
        RESPONSE=$(curl -s -X POST "$API_URL/teams" \
            -H "Content-Type: application/json" \
            -d "{\"name\": \"$TEAM_NAME\"}")

        TEAM_ID=$(echo $RESPONSE | grep -o '"id":"[^"]*"' | cut -d'"' -f4)
        TEAM_IDS+=("$TEAM_ID")

        echo "  ✓ $TEAM_NAME (ID: $TEAM_ID)"
    done

    print_success "Created ${#TEAM_IDS[@]} teams"
    echo ""
    print_info "Go to Supabase and check the 'teams' table"
    print_info "You should see all 16 teams"

    wait_for_user
}

# Step 3: Create Groups
create_groups() {
    print_step "Step 3: Creating 4 Groups (A, B, C, D)"

    GROUP_NAMES=("Group A" "Group B" "Group C" "Group D")

    for GROUP_NAME in "${GROUP_NAMES[@]}"; do
        RESPONSE=$(curl -s -X POST "$API_URL/tournaments/$TOURNAMENT_ID/groups" \
            -H "Content-Type: application/json" \
            -d "{\"name\": \"$GROUP_NAME\"}")

        GROUP_ID=$(echo $RESPONSE | grep -o '"id":"[^"]*"' | cut -d'"' -f4)
        GROUP_IDS+=("$GROUP_ID")

        print_success "Created $GROUP_NAME (ID: $GROUP_ID)"
    done

    echo ""
    print_info "Go to Supabase and check the 'groups' table"
    print_info "You should see 4 groups for tournament: $TOURNAMENT_ID"

    wait_for_user
}

# Step 4: Add teams to groups
add_teams_to_groups() {
    print_step "Step 4: Adding Teams to Groups (4 teams per group)"

    GROUP_NAMES=("Group A" "Group B" "Group C" "Group D")

    for i in {0..3}; do
        GROUP_ID="${GROUP_IDS[$i]}"
        GROUP_NAME="${GROUP_NAMES[$i]}"

        print_info "Adding teams to $GROUP_NAME:"

        # Add 4 teams to each group
        START_IDX=$((i * 4))
        TEAM_IDS_JSON="["

        for j in {0..3}; do
            TEAM_IDX=$((START_IDX + j))
            TEAM_ID="${TEAM_IDS[$TEAM_IDX]}"

            if [ $j -gt 0 ]; then
                TEAM_IDS_JSON+=","
            fi
            TEAM_IDS_JSON+="{\"id\":\"$TEAM_ID\"}"

            echo "  - Team ${TEAM_IDS[$TEAM_IDX]}"
        done

        TEAM_IDS_JSON+="]"

        # Add all 4 teams at once
        curl -s -X PUT "$API_URL/tournaments/$TOURNAMENT_ID/groups/$GROUP_ID/teams" \
            -H "Content-Type: application/json" \
            -d "$TEAM_IDS_JSON" > /dev/null

        print_success "Added 4 teams to $GROUP_NAME"
        echo ""
    done

    print_info "IMPORTANT: When the 4th team is added to each group,"
    print_info "the system automatically generates round-robin matches!"
    echo ""
    print_info "Go to Supabase and check:"
    print_info "  1. 'groups' table - each group should have 4 teams"
    print_info "  2. 'matches' table - should have 24 matches created"
    print_info "     (6 matches per group × 4 groups = 24 total)"

    wait_for_user
}

# Step 5: Get matches for a group
view_group_matches() {
    print_step "Step 5: Viewing Generated Matches"

    print_info "Let's see the matches for Group A..."

    RESPONSE=$(curl -s "$API_URL/tournaments/$TOURNAMENT_ID/matches?showMatches=pending")

    echo ""
    echo "Pending matches:"
    echo "$RESPONSE" | jq '.'

    echo ""
    print_info "All matches are in 'pending' status"
    print_info "Each team plays every other team once (round-robin)"

    wait_for_user
}

# Step 6: Play group stage matches
play_group_matches() {
    print_step "Step 6: Playing Group Stage Matches"

    print_info "We'll update scores for all 24 group stage matches"
    print_info "Simulating match results..."

    # Get all pending matches
    MATCHES=$(curl -s "$API_URL/tournaments/$TOURNAMENT_ID/matches?showMatches=pending")

    # Extract match IDs (this is simplified - in real script you'd parse JSON properly)
    MATCH_IDS=$(echo "$MATCHES" | jq -r '.[].id')

    MATCH_COUNT=0
    for MATCH_ID in $MATCH_IDS; do
        # Generate random scores (0-3 goals each, no ties)
        HOME_SCORE=$((RANDOM % 4))
        VISITOR_SCORE=$((RANDOM % 4))

        # Ensure no ties (requirement for this tournament)
        while [ $HOME_SCORE -eq $VISITOR_SCORE ]; do
            VISITOR_SCORE=$((RANDOM % 4))
        done

        # Update match score
        curl -s -X PATCH "$API_URL/tournaments/$TOURNAMENT_ID/matches/$MATCH_ID" \
            -H "Content-Type: application/json" \
            -d "{\"score\": {\"home\": $HOME_SCORE, \"visitor\": $VISITOR_SCORE}}" > /dev/null

        MATCH_COUNT=$((MATCH_COUNT + 1))
        echo "  Match $MATCH_COUNT: Score $HOME_SCORE - $VISITOR_SCORE"
    done

    print_success "Played all $MATCH_COUNT group stage matches!"
    echo ""
    print_info "IMPORTANT: After the LAST group match is completed,"
    print_info "the system automatically generates the playoff bracket!"
    echo ""
    print_info "Go to Supabase and check the 'matches' table:"
    print_info "  - 24 matches with round='regular' should have status='played'"
    print_info "  - 4 NEW matches with round='quarterfinals' should appear!"

    wait_for_user
}

# Step 7: View playoff bracket
view_playoffs() {
    print_step "Step 7: Viewing Playoff Bracket (Quarterfinals)"

    print_info "The top 2 teams from each group advance to quarterfinals"
    print_info "Quarterfinal matchups:"

    RESPONSE=$(curl -s "$API_URL/tournaments/$TOURNAMENT_ID/matches")
    QUARTERFINALS=$(echo "$RESPONSE" | jq '[.[] | select(.round == "quarterfinals")]')

    echo "$QUARTERFINALS" | jq -r '.[] | "  \(.home.name) vs \(.visitor.name)"'

    echo ""
    print_info "Go to Supabase - you should see 4 quarterfinal matches"

    wait_for_user
}

# Step 8: Play quarterfinals
play_quarterfinals() {
    print_step "Step 8: Playing Quarterfinal Matches"

    # Get quarterfinal matches
    MATCHES=$(curl -s "$API_URL/tournaments/$TOURNAMENT_ID/matches")
    QUARTERFINAL_IDS=$(echo "$MATCHES" | jq -r '[.[] | select(.round == "quarterfinals")] | .[].id')

    QF_COUNT=0
    for MATCH_ID in $QUARTERFINAL_IDS; do
        HOME_SCORE=$((RANDOM % 4))
        VISITOR_SCORE=$((RANDOM % 4))

        while [ $HOME_SCORE -eq $VISITOR_SCORE ]; do
            VISITOR_SCORE=$((RANDOM % 4))
        done

        curl -s -X PATCH "$API_URL/tournaments/$TOURNAMENT_ID/matches/$MATCH_ID" \
            -H "Content-Type: application/json" \
            -d "{\"score\": {\"home\": $HOME_SCORE, \"visitor\": $VISITOR_SCORE}}" > /dev/null

        QF_COUNT=$((QF_COUNT + 1))
        echo "  Quarterfinal $QF_COUNT: Score $HOME_SCORE - $VISITOR_SCORE"
    done

    print_success "Quarterfinals complete!"
    echo ""
    print_info "After completing ALL quarterfinals,"
    print_info "the system generates 2 semifinal matches!"
    echo ""
    print_info "Go to Supabase and check:"
    print_info "  - 4 quarterfinal matches should be 'played'"
    print_info "  - 2 NEW semifinal matches should appear!"

    wait_for_user
}

# Step 9: Play semifinals
play_semifinals() {
    print_step "Step 9: Playing Semifinal Matches"

    MATCHES=$(curl -s "$API_URL/tournaments/$TOURNAMENT_ID/matches")
    SEMIFINAL_IDS=$(echo "$MATCHES" | jq -r '[.[] | select(.round == "semifinals")] | .[].id')

    SF_COUNT=0
    for MATCH_ID in $SEMIFINAL_IDS; do
        HOME_SCORE=$((RANDOM % 4))
        VISITOR_SCORE=$((RANDOM % 4))

        while [ $HOME_SCORE -eq $VISITOR_SCORE ]; do
            VISITOR_SCORE=$((RANDOM % 4))
        done

        curl -s -X PATCH "$API_URL/tournaments/$TOURNAMENT_ID/matches/$MATCH_ID" \
            -H "Content-Type: application/json" \
            -d "{\"score\": {\"home\": $HOME_SCORE, \"visitor\": $VISITOR_SCORE}}" > /dev/null

        SF_COUNT=$((SF_COUNT + 1))
        echo "  Semifinal $SF_COUNT: Score $HOME_SCORE - $VISITOR_SCORE"
    done

    print_success "Semifinals complete!"
    echo ""
    print_info "After completing BOTH semifinals,"
    print_info "the system generates 1 FINAL match!"
    echo ""
    print_info "Go to Supabase and check:"
    print_info "  - 2 semifinal matches should be 'played'"
    print_info "  - 1 NEW final match should appear!"

    wait_for_user
}

# Step 10: Play the final
play_final() {
    print_step "Step 10: Playing the FINAL Match!"

    MATCHES=$(curl -s "$API_URL/tournaments/$TOURNAMENT_ID/matches")
    FINAL=$(echo "$MATCHES" | jq -r '[.[] | select(.round == "finals")] | .[0]')
    FINAL_ID=$(echo "$FINAL" | jq -r '.id')
    HOME_TEAM=$(echo "$FINAL" | jq -r '.home.name')
    VISITOR_TEAM=$(echo "$FINAL" | jq -r '.visitor.name')

    print_info "🏆 FINAL MATCH 🏆"
    print_info "$HOME_TEAM vs $VISITOR_TEAM"
    echo ""

    HOME_SCORE=$((RANDOM % 4))
    VISITOR_SCORE=$((RANDOM % 4))

    while [ $HOME_SCORE -eq $VISITOR_SCORE ]; do
        VISITOR_SCORE=$((RANDOM % 4))
    done

    curl -s -X PATCH "$API_URL/tournaments/$TOURNAMENT_ID/matches/$FINAL_ID" \
        -H "Content-Type: application/json" \
        -d "{\"score\": {\"home\": $HOME_SCORE, \"visitor\": $VISITOR_SCORE}}" > /dev/null

    echo ""
    print_success "Final Score: $HOME_TEAM $HOME_SCORE - $VISITOR_SCORE $VISITOR_TEAM"

    if [ $HOME_SCORE -gt $VISITOR_SCORE ]; then
        echo ""
        echo -e "${GREEN}🏆🏆🏆 CHAMPION: $HOME_TEAM! 🏆🏆🏆${NC}"
    else
        echo ""
        echo -e "${GREEN}🏆🏆🏆 CHAMPION: $VISITOR_TEAM! 🏆🏆🏆${NC}"
    fi

    echo ""
    print_info "Go to Supabase and check the final match - it's now 'played'!"
    print_info "The tournament is complete!"

    wait_for_user
}

# Step 11: View final standings
view_final_results() {
    print_step "Step 11: Tournament Complete - View All Results"

    print_info "Let's view all played matches:"

    RESPONSE=$(curl -s "$API_URL/tournaments/$TOURNAMENT_ID/matches?showMatches=played")

    echo ""
    echo "Total matches played:"
    echo "$RESPONSE" | jq 'length'

    echo ""
    echo "Breakdown by round:"
    echo "$RESPONSE" | jq -r 'group_by(.round) | .[] | "\(.[0].round): \(length) matches"'

    echo ""
    print_success "Tournament 'World Cup 2025' is complete!"
    print_info "Total matches: 31"
    print_info "  - Regular: 24 matches (4 groups × 6 matches)"
    print_info "  - Quarterfinals: 4 matches"
    print_info "  - Semifinals: 2 matches"
    print_info "  - Finals: 1 match"

    wait_for_user
}

# Main execution
main() {
    clear
    echo -e "${BLUE}"
    echo "╔════════════════════════════════════════════════╗"
    echo "║   Tournament Management System - DEMO          ║"
    echo "║   End-to-End Round Robin + Playoffs Flow       ║"
    echo "╚════════════════════════════════════════════════╝"
    echo -e "${NC}"

    check_service
    create_tournament
    create_teams
    create_groups
    add_teams_to_groups
    view_group_matches
    play_group_matches
    view_playoffs
    play_quarterfinals
    play_semifinals
    play_final
    view_final_results

    echo ""
    echo -e "${GREEN}╔════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║            DEMO COMPLETE!                      ║${NC}"
    echo -e "${GREEN}╚════════════════════════════════════════════════╝${NC}"
    echo ""
    print_info "You've just witnessed a complete tournament from start to finish!"
    print_info "The event-driven architecture automatically:"
    print_info "  1. Generated matches when groups were full"
    print_info "  2. Created playoff brackets when group stage finished"
    print_info "  3. Advanced winners through each playoff round"
}

# Run the demo
main
