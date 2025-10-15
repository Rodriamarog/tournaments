//
// Created by tsuny on 8/31/25.
//

#ifndef TOURNAMENTS_ITOURNAMENTDELEGATE_HPP
#define TOURNAMENTS_ITOURNAMENTDELEGATE_HPP

#include <string>
#include <memory>
#include <expected>

#include "domain/Tournament.hpp"

class ITournamentDelegate {
public:
    virtual ~ITournamentDelegate() = default;
    virtual std::expected<std::string, std::string> CreateTournament(std::shared_ptr<domain::Tournament> tournament) = 0;
    virtual std::expected<void, std::string> UpdateTournament(const domain::Tournament& tournament) = 0;
    virtual std::shared_ptr<domain::Tournament> GetTournament(const std::string& id) = 0;
    virtual std::vector<std::shared_ptr<domain::Tournament>> ReadAll() = 0;
};

#endif //TOURNAMENTS_ITOURNAMENTDELEGATE_HPP