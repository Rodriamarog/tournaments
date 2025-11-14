#ifndef SERVICE_MATCH_GENERATION_SERVICE_HPP
#define SERVICE_MATCH_GENERATION_SERVICE_HPP

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <expected>

#include "domain/Match.hpp"
#include "domain/Group.hpp"
#include "domain/Tournament.hpp"
#include "persistence/repository/MatchRepository.hpp"
#include "persistence/repository/GroupRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"

/**
 * Service responsible for generating Round Robin matches for a tournament group.
 *
 * Round Robin algorithm: Each team plays every other team exactly once.
 * For N teams: (N * (N-1)) / 2 matches total
 *
 * Example for 4 teams (A, B, C, D):
 * - Match 1: A vs B
 * - Match 2: A vs C
 * - Match 3: A vs D
 * - Match 4: B vs C
 * - Match 5: B vs D
 * - Match 6: C vs D
 * Total: (4 * 3) / 2 = 6 matches
 */
class MatchGenerationService {
    std::shared_ptr<MatchRepository> matchRepository;
    std::shared_ptr<GroupRepository> groupRepository;
    std::shared_ptr<TournamentRepository> tournamentRepository;

public:
    MatchGenerationService(
        const std::shared_ptr<MatchRepository>& matchRepository,
        const std::shared_ptr<GroupRepository>& groupRepository,
        const std::shared_ptr<TournamentRepository>& tournamentRepository
    );

    /**
     * Generates all round-robin matches for a specific group.
     *
     * @param tournamentId The tournament ID
     * @param groupId The group ID to generate matches for
     * @return Success or error message
     *
     * Business rules:
     * - Only generates matches if group is full (reached max capacity)
     * - Each team plays every other team exactly once
     * - All matches are created with status "pending"
     * - Matches are assigned round "regular"
     * - Skips generation if matches already exist for this group
     */
    std::expected<void, std::string> GenerateRoundRobinMatches(
        const std::string_view& tournamentId,
        const std::string_view& groupId
    );

    /**
     * Checks if a group is ready for match generation.
     * A group is ready when:
     * - It has reached the tournament's max teams per group
     * - No matches have been generated for it yet
     */
    bool IsGroupReadyForMatches(
        const std::string_view& tournamentId,
        const std::string_view& groupId
    );

private:
    /**
     * Calculates the number of matches needed for N teams in round-robin.
     * Formula: (N * (N-1)) / 2
     */
    int CalculateMatchCount(int teamCount) const;
};

inline MatchGenerationService::MatchGenerationService(
    const std::shared_ptr<MatchRepository>& matchRepository,
    const std::shared_ptr<GroupRepository>& groupRepository,
    const std::shared_ptr<TournamentRepository>& tournamentRepository
)
    : matchRepository(matchRepository),
      groupRepository(groupRepository),
      tournamentRepository(tournamentRepository) {}

inline bool MatchGenerationService::IsGroupReadyForMatches(
    const std::string_view& tournamentId,
    const std::string_view& groupId
) {
    // Check if matches already exist for this group
    if (matchRepository->ExistsByGroupId(groupId)) {
        return false;  // Already generated
    }

    // Get tournament to check max teams per group
    auto tournament = tournamentRepository->ReadById(tournamentId.data());
    if (tournament == nullptr) {
        return false;
    }

    // Get group to check current team count
    auto group = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
    if (group == nullptr) {
        return false;
    }

    // Check if group is full
    int maxTeams = tournament->Format().MaxTeamsPerGroup();
    int currentTeams = group->Teams().size();

    return currentTeams >= maxTeams;
}

inline int MatchGenerationService::CalculateMatchCount(int teamCount) const {
    // Double round-robin: each team plays every other team twice (home & away)
    return teamCount * (teamCount - 1);
}

inline std::expected<void, std::string> MatchGenerationService::GenerateRoundRobinMatches(
    const std::string_view& tournamentId,
    const std::string_view& groupId
) {
    // Validate tournament exists
    auto tournament = tournamentRepository->ReadById(tournamentId.data());
    if (tournament == nullptr) {
        return std::unexpected("Tournament doesn't exist");
    }

    // Validate group exists
    auto group = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
    if (group == nullptr) {
        return std::unexpected("Group doesn't exist");
    }

    // Check if group belongs to tournament
    if (group->TournamentId() != tournamentId) {
        return std::unexpected("Group doesn't belong to this tournament");
    }

    // Check if matches already exist
    if (matchRepository->ExistsByGroupId(groupId)) {
        return std::unexpected("Matches already generated for this group");
    }

    // Check if group is full
    int maxTeams = tournament->Format().MaxTeamsPerGroup();
    const auto& teams = group->Teams();
    int currentTeams = teams.size();

    if (currentTeams < maxTeams) {
        return std::unexpected("Group is not full yet. Cannot generate matches.");
    }

    // Generate double round-robin matches: each team plays every other team twice (home & away)
    for (size_t i = 0; i < teams.size(); i++) {
        for (size_t j = 0; j < teams.size(); j++) {
            if (i == j) continue;  // Team doesn't play itself

            domain::Match match;

            // Generate unique match ID (could use UUID in production)
            match.Id() = std::string(tournamentId) + "-" +
                         std::string(groupId) + "-" +
                         teams[i].Id + "-vs-" + teams[j].Id;

            match.TournamentId() = std::string(tournamentId);
            match.GroupId() = std::string(groupId);

            // Set home and visitor teams (i is always home)
            match.Home() = domain::MatchTeam(teams[i].Id, teams[i].Name);
            match.Visitor() = domain::MatchTeam(teams[j].Id, teams[j].Name);

            match.Round() = "regular";
            match.Status() = "pending";

            // Create match in database
            matchRepository->Create(match);
        }
    }

    return {};
}

#endif /* SERVICE_MATCH_GENERATION_SERVICE_HPP */
