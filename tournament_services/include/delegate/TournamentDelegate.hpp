//
// Created by tsuny on 8/31/25.
//

#ifndef TOURNAMENTS_TOURNAMENTDELEGATE_HPP
#define TOURNAMENTS_TOURNAMENTDELEGATE_HPP

#include <string>

#include "delegate/ITournamentDelegate.hpp"
#include "persistence/repository/TournamentRepository.hpp"

class TournamentDelegate final : public ITournamentDelegate {
    std::shared_ptr<TournamentRepository> tournamentRepository;
public:
    explicit TournamentDelegate(const std::shared_ptr<TournamentRepository>& repository);
    std::expected<std::string, std::string> CreateTournament(std::shared_ptr<domain::Tournament> tournament) override;
    std::expected<void, std::string> UpdateTournament(const domain::Tournament& tournament) override;
    std::shared_ptr<domain::Tournament> GetTournament(const std::string& id) override;
    std::vector<std::shared_ptr<domain::Tournament>> ReadAll() override;
};

#endif //TOURNAMENTS_TOURNAMENTDELEGATE_HPP