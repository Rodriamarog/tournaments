#ifndef SERVICE_MATCH_DELEGATE_HPP
#define SERVICE_MATCH_DELEGATE_HPP

#include <string>
#include <string_view>
#include <memory>
#include <expected>
#include <optional>
#include <format>

#include "IMatchDelegate.hpp"
#include "persistence/repository/MatchRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"
#include "cms/IQueueMessageProducer.hpp"
#include <nlohmann/json.hpp>

class MatchDelegate : public IMatchDelegate {
    std::shared_ptr<MatchRepository> matchRepository;
    std::shared_ptr<TournamentRepository> tournamentRepository;
    std::shared_ptr<IQueueMessageProducer> messageProducer;

public:
    MatchDelegate(
        const std::shared_ptr<MatchRepository>& matchRepository,
        const std::shared_ptr<TournamentRepository>& tournamentRepository,
        const std::shared_ptr<IQueueMessageProducer>& messageProducer
    );

    std::expected<std::vector<domain::Match>, std::string> GetMatches(
        const std::string_view& tournamentId,
        const std::optional<std::string>& statusFilter
    ) override;

    std::expected<domain::Match, std::string> GetMatch(
        const std::string_view& tournamentId,
        const std::string_view& matchId
    ) override;

    std::expected<void, std::string> UpdateScore(
        const std::string_view& tournamentId,
        const std::string_view& matchId,
        const domain::Score& score
    ) override;
};

MatchDelegate::MatchDelegate(
    const std::shared_ptr<MatchRepository>& matchRepository,
    const std::shared_ptr<TournamentRepository>& tournamentRepository,
    const std::shared_ptr<IQueueMessageProducer>& messageProducer
)
    : matchRepository(std::move(matchRepository)),
      tournamentRepository(std::move(tournamentRepository)),
      messageProducer(std::move(messageProducer)) {}

inline std::expected<std::vector<domain::Match>, std::string> MatchDelegate::GetMatches(
    const std::string_view& tournamentId,
    const std::optional<std::string>& statusFilter
) {
    // Validate tournament exists
    auto tournament = tournamentRepository->ReadById(tournamentId.data());
    if (tournament == nullptr) {
        return std::unexpected("Tournament doesn't exist");
    }

    // Fetch matches based on filter
    std::vector<std::shared_ptr<domain::Match>> matchPtrs;
    if (statusFilter.has_value()) {
        matchPtrs = matchRepository->FindByTournamentIdAndStatus(tournamentId, statusFilter.value());
    } else {
        matchPtrs = matchRepository->FindByTournamentId(tournamentId);
    }

    // Convert to value vector
    std::vector<domain::Match> matches;
    for (const auto& matchPtr : matchPtrs) {
        matches.push_back(*matchPtr);
    }

    return matches;
}

inline std::expected<domain::Match, std::string> MatchDelegate::GetMatch(
    const std::string_view& tournamentId,
    const std::string_view& matchId
) {
    // Validate tournament exists
    auto tournament = tournamentRepository->ReadById(tournamentId.data());
    if (tournament == nullptr) {
        return std::unexpected("Tournament doesn't exist");
    }

    // Fetch match
    auto matchPtr = matchRepository->ReadById(matchId.data());
    if (matchPtr == nullptr) {
        return std::unexpected("Match doesn't exist");
    }

    // Verify match belongs to tournament
    if (matchPtr->TournamentId() != tournamentId) {
        return std::unexpected("Match doesn't belong to this tournament");
    }

    return *matchPtr;
}

inline std::expected<void, std::string> MatchDelegate::UpdateScore(
    const std::string_view& tournamentId,
    const std::string_view& matchId,
    const domain::Score& score
) {
    // Validate tournament exists
    auto tournament = tournamentRepository->ReadById(tournamentId.data());
    if (tournament == nullptr) {
        return std::unexpected("Tournament doesn't exist");
    }

    // Fetch match
    auto matchPtr = matchRepository->ReadById(matchId.data());
    if (matchPtr == nullptr) {
        return std::unexpected("Match doesn't exist");
    }

    // Verify match belongs to tournament
    if (matchPtr->TournamentId() != tournamentId) {
        return std::unexpected("Match doesn't belong to this tournament");
    }

    // Validate score range (0-10 as per spec)
    if (score.home < 0 || score.visitor < 0) {
        return std::unexpected("Score cannot be negative");
    }
    if (score.home > 10 || score.visitor > 10) {
        return std::unexpected("Score must be between 0 and 10");
    }

    // Validate ties: allowed in regular season, not allowed in playoffs
    if (score.home == score.visitor && matchPtr->Round() != "regular") {
        return std::unexpected("Ties are not allowed in playoff rounds");
    }

    // Update match with score
    matchPtr->SetScore(score);
    auto updateResult = matchRepository->Update(*matchPtr);
    if (updateResult.empty()) {
        return std::unexpected("Failed to update match");
    }

    // Publish event for score registration (for playoff generation in Phase 5)
    nlohmann::json message;
    message["tournamentId"] = tournamentId;
    message["matchId"] = matchId;
    message["score"] = score;
    message["round"] = matchPtr->Round();
    message["winnerId"] = matchPtr->GetWinnerId().value_or("");
    messageProducer->SendMessage(message.dump(), "tournament.score-registered");

    return {};
}

#endif /* SERVICE_MATCH_DELEGATE_HPP */
