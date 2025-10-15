//
// Created by tomas on 8/31/25.
//
#include <string_view>
#include <memory>

#include "delegate/TournamentDelegate.hpp"

#include "persistence/repository/TournamentRepository.hpp"

TournamentDelegate::TournamentDelegate(
    const std::shared_ptr<TournamentRepository>& repository)
    : tournamentRepository(repository){}

std::expected<std::string, std::string> TournamentDelegate::CreateTournament(std::shared_ptr<domain::Tournament> tournament) {
    // Check for duplicate tournament name
    if (tournamentRepository->ExistsByName(tournament->Name())) {
        return std::unexpected("Tournament with this name already exists");
    }

    //fill groups according to max groups
    std::shared_ptr<domain::Tournament> tp = std::move(tournament);
    // for (auto[i, g] = std::tuple{0, 'A'}; i < tp->Format().NumberOfGroups(); i++,g++) {
    //     tp->Groups().push_back(domain::Group{std::format("Tournament {}", g)});
    // }

    std::string id = tournamentRepository->Create(*tp);

    //if groups are completed also create matches

    return id;
}

std::expected<void, std::string> TournamentDelegate::UpdateTournament(const domain::Tournament& tournament) {
    // First check if tournament exists
    auto existingTournament = tournamentRepository->ReadById(tournament.Id());
    if (!existingTournament || existingTournament->Id().empty()) {
        return std::unexpected("Tournament not found");
    }

    // Perform update
    auto result = tournamentRepository->Update(tournament);
    if (result.empty()) {
        return std::unexpected("Update failed");
    }

    return {};
}

std::shared_ptr<domain::Tournament> TournamentDelegate::GetTournament(const std::string& id) {
    return tournamentRepository->ReadById(id);
}

std::vector<std::shared_ptr<domain::Tournament> > TournamentDelegate::ReadAll() {
    return tournamentRepository->ReadAll();
}