#ifndef TOURNAMENTS_MATCHREPOSITORY_HPP
#define TOURNAMENTS_MATCHREPOSITORY_HPP

#include <string>
#include <string_view>
#include <memory>
#include <vector>
#include <nlohmann/json.hpp>

#include "persistence/configuration/IDbConnectionProvider.hpp"
#include "persistence/configuration/PostgresConnection.hpp"
#include "IRepository.hpp"
#include "domain/Match.hpp"
#include "domain/Utilities.hpp"

class MatchRepository : public IRepository<domain::Match, std::string> {
    std::shared_ptr<IDbConnectionProvider> connectionProvider;

public:
    MatchRepository(const std::shared_ptr<IDbConnectionProvider>& connectionProvider);

    std::shared_ptr<domain::Match> ReadById(std::string id) override;
    std::string Create(const domain::Match& entity) override;
    std::string Update(const domain::Match& entity) override;
    void Delete(std::string id) override;
    std::vector<std::shared_ptr<domain::Match>> ReadAll() override;

    // Additional methods for match operations
    virtual std::vector<std::shared_ptr<domain::Match>> FindByTournamentId(const std::string_view& tournamentId);
    virtual std::vector<std::shared_ptr<domain::Match>> FindByTournamentIdAndStatus(const std::string_view& tournamentId, const std::string& status);
    virtual std::vector<std::shared_ptr<domain::Match>> FindByGroupId(const std::string_view& groupId);
    virtual std::vector<std::shared_ptr<domain::Match>> FindByTournamentIdAndRound(const std::string_view& tournamentId, const std::string& round);
    virtual bool ExistsByGroupId(const std::string_view& groupId);
};

inline MatchRepository::MatchRepository(const std::shared_ptr<IDbConnectionProvider>& connectionProvider)
    : connectionProvider(std::move(connectionProvider)) {}

inline std::shared_ptr<domain::Match> MatchRepository::ReadById(std::string id) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    pqxx::result result = tx.exec_params(
        "SELECT id, document FROM matches WHERE id = $1", id
    );
    tx.commit();

    if (result.empty()) {
        return nullptr;
    }

    nlohmann::json matchJson = nlohmann::json::parse(result[0]["document"].c_str());
    auto match = std::make_shared<domain::Match>(matchJson);
    match->Id() = result[0]["id"].c_str();

    return match;
}

inline std::string MatchRepository::Create(const domain::Match& entity) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    nlohmann::json matchBody;
    domain::to_json(matchBody, entity);

    pqxx::work tx(*(connection->connection));
    pqxx::result result = tx.exec_params(
        "INSERT INTO matches (tournament_id, document) VALUES ($1, $2) RETURNING id",
        entity.TournamentId(), matchBody.dump()
    );
    tx.commit();

    return result[0]["id"].c_str();
}

inline std::string MatchRepository::Update(const domain::Match& entity) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    nlohmann::json matchBody;
    domain::to_json(matchBody, entity);

    pqxx::work tx(*(connection->connection));
    pqxx::result result = tx.exec_params(
        "UPDATE matches SET document = $1 WHERE id = $2 RETURNING id",
        matchBody.dump(), entity.Id()
    );
    tx.commit();

    if (result.empty()) {
        return "";
    }
    return result[0]["id"].c_str();
}

inline void MatchRepository::Delete(std::string id) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    tx.exec_params("DELETE FROM matches WHERE id = $1", id);
    tx.commit();
}

inline std::vector<std::shared_ptr<domain::Match>> MatchRepository::ReadAll() {
    std::vector<std::shared_ptr<domain::Match>> matches;

    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    pqxx::result result{tx.exec("SELECT id, document FROM matches")};
    tx.commit();

    for (auto row : result) {
        nlohmann::json matchJson = nlohmann::json::parse(row["document"].c_str());
        auto match = std::make_shared<domain::Match>(matchJson);
        match->Id() = row["id"].c_str();
        matches.push_back(match);
    }

    return matches;
}

inline std::vector<std::shared_ptr<domain::Match>> MatchRepository::FindByTournamentId(const std::string_view& tournamentId) {
    std::vector<std::shared_ptr<domain::Match>> matches;

    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    pqxx::result result = tx.exec_params(
        "SELECT id, document FROM matches WHERE document->>'tournamentId' = $1 ORDER BY created_at",
        tournamentId.data()
    );
    tx.commit();

    for (auto row : result) {
        nlohmann::json matchJson = nlohmann::json::parse(row["document"].c_str());
        auto match = std::make_shared<domain::Match>(matchJson);
        match->Id() = row["id"].c_str();
        matches.push_back(match);
    }

    return matches;
}

inline std::vector<std::shared_ptr<domain::Match>> MatchRepository::FindByTournamentIdAndStatus(
    const std::string_view& tournamentId, const std::string& status) {
    std::vector<std::shared_ptr<domain::Match>> matches;

    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    pqxx::result result = tx.exec_params(
        "SELECT id, document FROM matches WHERE document->>'tournamentId' = $1 AND document->>'status' = $2 ORDER BY created_at",
        tournamentId.data(), status
    );
    tx.commit();

    for (auto row : result) {
        nlohmann::json matchJson = nlohmann::json::parse(row["document"].c_str());
        auto match = std::make_shared<domain::Match>(matchJson);
        match->Id() = row["id"].c_str();
        matches.push_back(match);
    }

    return matches;
}

inline std::vector<std::shared_ptr<domain::Match>> MatchRepository::FindByGroupId(const std::string_view& groupId) {
    std::vector<std::shared_ptr<domain::Match>> matches;

    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    pqxx::result result = tx.exec_params(
        "SELECT id, document FROM matches WHERE document->>'groupId' = $1 ORDER BY created_at",
        groupId.data()
    );
    tx.commit();

    for (auto row : result) {
        nlohmann::json matchJson = nlohmann::json::parse(row["document"].c_str());
        auto match = std::make_shared<domain::Match>(matchJson);
        match->Id() = row["id"].c_str();
        matches.push_back(match);
    }

    return matches;
}

inline std::vector<std::shared_ptr<domain::Match>> MatchRepository::FindByTournamentIdAndRound(
    const std::string_view& tournamentId, const std::string& round) {
    std::vector<std::shared_ptr<domain::Match>> matches;

    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    pqxx::result result = tx.exec_params(
        "SELECT id, document FROM matches WHERE document->>'tournamentId' = $1 AND document->>'round' = $2 ORDER BY created_at",
        tournamentId.data(), round
    );
    tx.commit();

    for (auto row : result) {
        nlohmann::json matchJson = nlohmann::json::parse(row["document"].c_str());
        auto match = std::make_shared<domain::Match>(matchJson);
        match->Id() = row["id"].c_str();
        matches.push_back(match);
    }

    return matches;
}

inline bool MatchRepository::ExistsByGroupId(const std::string_view& groupId) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    pqxx::result result = tx.exec_params(
        "SELECT COUNT(*) as count FROM matches WHERE document->>'groupId' = $1",
        groupId.data()
    );
    tx.commit();

    return result[0]["count"].as<int>() > 0;
}

#endif // TOURNAMENTS_MATCHREPOSITORY_HPP
