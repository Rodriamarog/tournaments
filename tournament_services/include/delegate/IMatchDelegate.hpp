#ifndef SERVICE_IMATCH_DELEGATE_HPP
#define SERVICE_IMATCH_DELEGATE_HPP

#include <string>
#include <string_view>
#include <vector>
#include <expected>
#include <optional>
#include "domain/Match.hpp"

class IMatchDelegate {
public:
    virtual ~IMatchDelegate() = default;

    virtual std::expected<std::vector<domain::Match>, std::string> GetMatches(
        const std::string_view& tournamentId,
        const std::optional<std::string>& statusFilter
    ) = 0;

    virtual std::expected<domain::Match, std::string> GetMatch(
        const std::string_view& tournamentId,
        const std::string_view& matchId
    ) = 0;

    virtual std::expected<void, std::string> UpdateScore(
        const std::string_view& tournamentId,
        const std::string_view& matchId,
        const domain::Score& score
    ) = 0;
};

#endif /* SERVICE_IMATCH_DELEGATE_HPP */
