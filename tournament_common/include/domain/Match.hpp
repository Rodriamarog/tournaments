#ifndef DOMAIN_MATCH_HPP
#define DOMAIN_MATCH_HPP

#include <string>
#include <optional>
#include <nlohmann/json.hpp>
#include "Team.hpp"

namespace domain {

    // Embedded team info for matches (no full Team object needed)
    struct MatchTeam {
        std::string id;
        std::string name;

        MatchTeam() = default;
        MatchTeam(const std::string& id, const std::string& name)
            : id(id), name(name) {}
    };

    // Score for a match
    struct Score {
        int home;
        int visitor;

        Score() : home(0), visitor(0) {}
        Score(int home, int visitor) : home(home), visitor(visitor) {}
    };

    class Match {
    private:
        std::string id_;
        std::string tournamentId_;
        std::optional<std::string> groupId_;  // null for playoff matches
        MatchTeam home_;
        MatchTeam visitor_;
        std::optional<Score> score_;
        std::string round_;   // "regular", "quarterfinals", "semifinals", "finals"
        std::string status_;  // "pending", "played"

    public:
        // Constructors
        Match() = default;

        Match(const std::string& id, const std::string& tournamentId,
              const std::optional<std::string>& groupId,
              const MatchTeam& home, const MatchTeam& visitor,
              const std::string& round)
            : id_(id), tournamentId_(tournamentId), groupId_(groupId),
              home_(home), visitor_(visitor), round_(round), status_("pending") {}

        // Getters
        const std::string& Id() const { return id_; }
        const std::string& TournamentId() const { return tournamentId_; }
        const std::optional<std::string>& GroupId() const { return groupId_; }
        const MatchTeam& Home() const { return home_; }
        const MatchTeam& Visitor() const { return visitor_; }
        const std::optional<Score>& GetScore() const { return score_; }
        const std::string& Round() const { return round_; }
        const std::string& Status() const { return status_; }

        // Setters
        std::string& Id() { return id_; }
        std::string& TournamentId() { return tournamentId_; }
        std::optional<std::string>& GroupId() { return groupId_; }
        MatchTeam& Home() { return home_; }
        MatchTeam& Visitor() { return visitor_; }
        std::optional<Score>& GetScore() { return score_; }
        std::string& Round() { return round_; }
        std::string& Status() { return status_; }

        // Set score and mark as played
        void SetScore(const Score& score) {
            score_ = score;
            status_ = "played";
        }

        // Determine winner (if score exists)
        std::optional<std::string> GetWinnerId() const {
            if (!score_.has_value()) {
                return std::nullopt;
            }
            if (score_->home > score_->visitor) {
                return home_.id;
            } else if (score_->visitor > score_->home) {
                return visitor_.id;
            }
            return std::nullopt;  // tie
        }
    };

    // JSON serialization for MatchTeam
    inline void to_json(nlohmann::json& j, const MatchTeam& team) {
        j = nlohmann::json{
            {"id", team.id},
            {"name", team.name}
        };
    }

    inline void from_json(const nlohmann::json& j, MatchTeam& team) {
        j.at("id").get_to(team.id);
        j.at("name").get_to(team.name);
    }

    // JSON serialization for Score
    inline void to_json(nlohmann::json& j, const Score& score) {
        j = nlohmann::json{
            {"home", score.home},
            {"visitor", score.visitor}
        };
    }

    inline void from_json(const nlohmann::json& j, Score& score) {
        j.at("home").get_to(score.home);
        j.at("visitor").get_to(score.visitor);
    }

    // JSON serialization for Match
    inline void to_json(nlohmann::json& j, const Match& match) {
        j = nlohmann::json{
            {"id", match.Id()},
            {"tournamentId", match.TournamentId()},
            {"home", match.Home()},
            {"visitor", match.Visitor()},
            {"round", match.Round()},
            {"status", match.Status()}
        };

        if (match.GroupId().has_value()) {
            j["groupId"] = match.GroupId().value();
        }

        if (match.GetScore().has_value()) {
            j["score"] = match.GetScore().value();
        }
    }

    inline void from_json(const nlohmann::json& j, Match& match) {
        match.TournamentId() = j.at("tournamentId").get<std::string>();
        match.Home() = j.at("home").get<MatchTeam>();
        match.Visitor() = j.at("visitor").get<MatchTeam>();
        match.Round() = j.at("round").get<std::string>();
        match.Status() = j.at("status").get<std::string>();

        if (j.contains("groupId") && !j["groupId"].is_null()) {
            match.GroupId() = j.at("groupId").get<std::string>();
        }

        if (j.contains("score") && !j["score"].is_null()) {
            match.GetScore() = j.at("score").get<Score>();
        }
    }
}

#endif // DOMAIN_MATCH_HPP
