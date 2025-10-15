//
// Created by tomas on 8/22/25.
//

#include "delegate/TeamDelegate.hpp"
#include "persistence/repository/TeamRepository.hpp"

#include <utility>

TeamDelegate::TeamDelegate(std::shared_ptr<IRepository<domain::Team, std::string_view> > repository) : teamRepository(std::move(repository)) {
}

std::vector<std::shared_ptr<domain::Team>> TeamDelegate::GetAllTeams() {
    return teamRepository->ReadAll();
}

std::shared_ptr<domain::Team> TeamDelegate::GetTeam(std::string_view id) {
    return teamRepository->ReadById(id.data());
}

std::expected<std::string, std::string> TeamDelegate::SaveTeam(const domain::Team& team){
    // Check for duplicate team name
    auto teamRepo = std::dynamic_pointer_cast<TeamRepository>(teamRepository);
    if (teamRepo && teamRepo->ExistsByName(team.Name)) {
        return std::unexpected("Team with this name already exists");
    }

    auto id = teamRepository->Create(team);
    return std::string(id);
}

std::expected<void, std::string> TeamDelegate::UpdateTeam(const domain::Team& team) {
    // First check if team exists
    auto existingTeam = teamRepository->ReadById(team.Id);
    if (!existingTeam || existingTeam->Id.empty()) {
        return std::unexpected("Team not found");
    }

    // Perform update
    auto result = teamRepository->Update(team);
    if (result.empty()) {
        return std::unexpected("Update failed");
    }

    return {};
}

