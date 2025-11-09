#ifndef SERVICE_PLAYOFF_GENERATION_SERVICE_HPP
#define SERVICE_PLAYOFF_GENERATION_SERVICE_HPP

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <expected>
#include <algorithm>
#include <map>

#include "domain/Match.hpp"
#include "domain/Group.hpp"
#include "domain/Tournament.hpp"
#include "domain/Team.hpp"
#include "persistence/repository/MatchRepository.hpp"
#include "persistence/repository/GroupRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"

/**
 * Service responsible for generating Single Elimination playoff brackets.
 *
 * Playoff Structure for Round Robin tournaments:
 * - Quarterfinals: Top 2 teams from each of 4 groups (8 teams total)
 * - Semifinals: Winners of quarterfinals (4 teams)
 * - Finals: Winners of semifinals (2 teams)
 *
 * Trigger: Generated when ALL group stage matches are completed (all have status "played")
 *
 * Seeding:
 * - Group winners (1st place) face group runners-up (2nd place)
 * - Example: Group A 1st vs Group D 2nd, Group B 1st vs Group C 2nd, etc.
 */
class PlayoffGenerationService {
    std::shared_ptr<MatchRepository> matchRepository;
    std::shared_ptr<GroupRepository> groupRepository;
    std::shared_ptr<TournamentRepository> tournamentRepository;

public:
    PlayoffGenerationService(
        const std::shared_ptr<MatchRepository>& matchRepository,
        const std::shared_ptr<GroupRepository>& groupRepository,
        const std::shared_ptr<TournamentRepository>& tournamentRepository
    );

    /**
     * Checks if all group stage matches for a tournament are completed.
     * @return true if all "regular" round matches have status "played"
     */
    bool AreAllGroupMatchesCompleted(const std::string_view& tournamentId);

    /**
     * Generates the quarterfinal playoff bracket.
     * Called when all group stage matches are completed.
     *
     * @param tournamentId The tournament ID
     * @return Success or error message
     *
     * Business rules:
     * - Only generates if ALL group matches are "played"
     * - Takes top 2 teams from each group (8 teams total for 4 groups)
     * - Creates 4 quarterfinal matches
     * - Skips if quarterfinals already exist
     */
    std::expected<void, std::string> GenerateQuarterfinals(
        const std::string_view& tournamentId
    );

    /**
     * Advances winners from one playoff round to the next.
     * Called when a playoff match is completed.
     *
     * @param tournamentId The tournament ID
     * @param round The round that was just completed ("quarterfinals" or "semifinals")
     * @return Success or error message
     */
    std::expected<void, std::string> AdvanceWinners(
        const std::string_view& tournamentId,
        const std::string& round
    );

private:
    struct TeamStanding {
        domain::Team team;
        int wins;
        int goalsFor;
        int goalsAgainst;
        int goalDifference;

        TeamStanding() : wins(0), goalsFor(0), goalsAgainst(0), goalDifference(0) {}
    };

    /**
     * Calculates team standings for a group based on match results.
     * Returns teams sorted by: 1) Wins, 2) Goal difference, 3) Goals scored
     */
    std::vector<TeamStanding> CalculateGroupStandings(
        const std::string_view& tournamentId,
        const std::string_view& groupId
    );

    /**
     * Gets the top N teams from a group's standings.
     */
    std::vector<domain::Team> GetTopTeams(
        const std::vector<TeamStanding>& standings,
        int count
    );
};

inline PlayoffGenerationService::PlayoffGenerationService(
    const std::shared_ptr<MatchRepository>& matchRepository,
    const std::shared_ptr<GroupRepository>& groupRepository,
    const std::shared_ptr<TournamentRepository>& tournamentRepository
)
    : matchRepository(matchRepository),
      groupRepository(groupRepository),
      tournamentRepository(tournamentRepository) {}

inline bool PlayoffGenerationService::AreAllGroupMatchesCompleted(
    const std::string_view& tournamentId
) {
    // Get all "regular" round matches
    auto matches = matchRepository->FindByTournamentIdAndRound(tournamentId, "regular");

    if (matches.empty()) {
        return false;  // No matches yet
    }

    // Check if all are "played"
    for (const auto& match : matches) {
        if (match->Status() != "played") {
            return false;  // Found a pending match
        }
    }

    return true;  // All matches completed
}

inline std::vector<PlayoffGenerationService::TeamStanding>
PlayoffGenerationService::CalculateGroupStandings(
    const std::string_view& tournamentId,
    const std::string_view& groupId
) {
    // Get all matches for this group
    auto matches = matchRepository->FindByGroupId(groupId);

    // Get group to get team list
    auto group = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
    if (group == nullptr) {
        return {};
    }

    // Initialize standings for each team
    std::map<std::string, TeamStanding> standings;
    for (const auto& team : group->Teams()) {
        TeamStanding standing;
        standing.team = team;
        standings[team.Id] = standing;
    }

    // Calculate standings from match results
    for (const auto& match : matches) {
        if (match->Status() != "played" || !match->GetScore().has_value()) {
            continue;  // Skip unplayed matches
        }

        const auto& score = match->GetScore().value();
        std::string homeId = match->Home().id;
        std::string visitorId = match->Visitor().id;

        // Update goals
        standings[homeId].goalsFor += score.home;
        standings[homeId].goalsAgainst += score.visitor;
        standings[visitorId].goalsFor += score.visitor;
        standings[visitorId].goalsAgainst += score.home;

        // Update wins
        if (score.home > score.visitor) {
            standings[homeId].wins++;
        } else if (score.visitor > score.home) {
            standings[visitorId].wins++;
        }
    }

    // Calculate goal difference
    std::vector<TeamStanding> result;
    for (auto& [teamId, standing] : standings) {
        standing.goalDifference = standing.goalsFor - standing.goalsAgainst;
        result.push_back(standing);
    }

    // Sort by: 1) Wins (desc), 2) Goal difference (desc), 3) Goals for (desc)
    std::sort(result.begin(), result.end(), [](const TeamStanding& a, const TeamStanding& b) {
        if (a.wins != b.wins) return a.wins > b.wins;
        if (a.goalDifference != b.goalDifference) return a.goalDifference > b.goalDifference;
        return a.goalsFor > b.goalsFor;
    });

    return result;
}

inline std::vector<domain::Team> PlayoffGenerationService::GetTopTeams(
    const std::vector<TeamStanding>& standings,
    int count
) {
    std::vector<domain::Team> teams;
    for (int i = 0; i < std::min(count, (int)standings.size()); i++) {
        teams.push_back(standings[i].team);
    }
    return teams;
}

inline std::expected<void, std::string> PlayoffGenerationService::GenerateQuarterfinals(
    const std::string_view& tournamentId
) {
    // Validate tournament exists
    auto tournament = tournamentRepository->ReadById(tournamentId.data());
    if (tournament == nullptr) {
        return std::unexpected("Tournament doesn't exist");
    }

    // Check if all group matches are completed
    if (!AreAllGroupMatchesCompleted(tournamentId)) {
        return std::unexpected("Not all group stage matches are completed");
    }

    // Check if quarterfinals already exist
    auto existingQuarterfinals = matchRepository->FindByTournamentIdAndRound(tournamentId, "quarterfinals");
    if (!existingQuarterfinals.empty()) {
        return std::unexpected("Quarterfinals already generated");
    }

    // Get all groups
    auto groups = groupRepository->FindByTournamentId(tournamentId);
    if (groups.size() < 4) {
        return std::unexpected("Tournament must have at least 4 groups for playoffs");
    }

    // Get top 2 teams from each group
    std::vector<std::vector<domain::Team>> groupTopTeams;
    for (const auto& group : groups) {
        auto standings = CalculateGroupStandings(tournamentId, group->Id());
        auto topTeams = GetTopTeams(standings, 2);  // Top 2 teams
        groupTopTeams.push_back(topTeams);
    }

    // Generate quarterfinal matchups (example seeding):
    // QF1: Group A 1st vs Group D 2nd
    // QF2: Group B 1st vs Group C 2nd
    // QF3: Group C 1st vs Group B 2nd
    // QF4: Group D 1st vs Group A 2nd

    std::vector<std::pair<domain::Team, domain::Team>> quarterfinalPairs;
    if (groupTopTeams.size() >= 4) {
        quarterfinalPairs.push_back({groupTopTeams[0][0], groupTopTeams[3][1]});  // A1 vs D2
        quarterfinalPairs.push_back({groupTopTeams[1][0], groupTopTeams[2][1]});  // B1 vs C2
        quarterfinalPairs.push_back({groupTopTeams[2][0], groupTopTeams[1][1]});  // C1 vs B2
        quarterfinalPairs.push_back({groupTopTeams[3][0], groupTopTeams[0][1]});  // D1 vs A2
    }

    // Create quarterfinal matches
    for (size_t i = 0; i < quarterfinalPairs.size(); i++) {
        domain::Match match;
        match.Id() = std::string(tournamentId) + "-qf-" + std::to_string(i + 1);
        match.TournamentId() = std::string(tournamentId);
        match.GroupId() = std::nullopt;  // Playoffs don't belong to groups
        match.Home() = domain::MatchTeam(quarterfinalPairs[i].first.Id, quarterfinalPairs[i].first.Name);
        match.Visitor() = domain::MatchTeam(quarterfinalPairs[i].second.Id, quarterfinalPairs[i].second.Name);
        match.Round() = "quarterfinals";
        match.Status() = "pending";

        matchRepository->Create(match);
    }

    return {};
}

inline std::expected<void, std::string> PlayoffGenerationService::AdvanceWinners(
    const std::string_view& tournamentId,
    const std::string& round
) {
    // Validate tournament exists
    auto tournament = tournamentRepository->ReadById(tournamentId.data());
    if (tournament == nullptr) {
        return std::unexpected("Tournament doesn't exist");
    }

    std::string nextRound;
    int matchesNeeded;

    if (round == "quarterfinals") {
        nextRound = "semifinals";
        matchesNeeded = 4;  // Need all 4 QF matches completed
    } else if (round == "semifinals") {
        nextRound = "finals";
        matchesNeeded = 2;  // Need both SF matches completed
    } else {
        return std::unexpected("Invalid round for advancement");
    }

    // Get all matches from the completed round
    auto completedMatches = matchRepository->FindByTournamentIdAndRound(tournamentId, round);

    if (completedMatches.size() < matchesNeeded) {
        return std::unexpected("Not enough matches in " + round);
    }

    // Check if all matches are completed
    std::vector<std::string> winners;
    for (const auto& match : completedMatches) {
        if (match->Status() != "played") {
            return {};  // Not ready yet, some matches still pending
        }

        auto winnerId = match->GetWinnerId();
        if (winnerId.has_value()) {
            winners.push_back(winnerId.value());
        }
    }

    if (winners.size() < matchesNeeded) {
        return std::unexpected("Not all matches have winners");
    }

    // Check if next round already exists
    auto existingNextRound = matchRepository->FindByTournamentIdAndRound(tournamentId, nextRound);
    if (!existingNextRound.empty()) {
        return {};  // Already generated
    }

    // Generate next round matches
    for (size_t i = 0; i < winners.size(); i += 2) {
        if (i + 1 >= winners.size()) break;

        // Find team details
        domain::MatchTeam homeTeam;
        domain::MatchTeam visitorTeam;

        for (const auto& match : completedMatches) {
            if (match->GetWinnerId() == winners[i]) {
                if (match->Home().id == winners[i]) {
                    homeTeam = match->Home();
                } else {
                    homeTeam = match->Visitor();
                }
            }
            if (match->GetWinnerId() == winners[i + 1]) {
                if (match->Home().id == winners[i + 1]) {
                    visitorTeam = match->Home();
                } else {
                    visitorTeam = match->Visitor();
                }
            }
        }

        domain::Match nextMatch;
        nextMatch.Id() = std::string(tournamentId) + "-" + nextRound + "-" + std::to_string(i / 2 + 1);
        nextMatch.TournamentId() = std::string(tournamentId);
        nextMatch.GroupId() = std::nullopt;
        nextMatch.Home() = homeTeam;
        nextMatch.Visitor() = visitorTeam;
        nextMatch.Round() = nextRound;
        nextMatch.Status() = "pending";

        matchRepository->Create(nextMatch);
    }

    return {};
}

#endif /* SERVICE_PLAYOFF_GENERATION_SERVICE_HPP */
