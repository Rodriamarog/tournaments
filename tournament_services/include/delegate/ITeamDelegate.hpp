#ifndef ITEAM_DELEGATE_HPP
#define ITEAM_DELEGATE_HPP

#include <string_view>
#include <memory>
#include <expected>

#include "domain/Team.hpp"

class ITeamDelegate {
    public:
    virtual ~ITeamDelegate() = default;
    virtual std::shared_ptr<domain::Team> GetTeam(std::string_view id) = 0;
    virtual std::vector<std::shared_ptr<domain::Team>> GetAllTeams() = 0;
    virtual std::expected<std::string, std::string> SaveTeam(const domain::Team& team) = 0;
    virtual std::expected<void, std::string> UpdateTeam(const domain::Team& team) = 0;
};

#endif /* ITEAM_DELEGATE_HPP */
