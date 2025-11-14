//
// Created by root on 9/21/25.
//

#ifndef TOURNAMENTS_GROUPREPOSITORY_HPP
#define TOURNAMENTS_GROUPREPOSITORY_HPP

#include <string>
#include <memory>
#include <nlohmann/json.hpp>

#include "persistence/configuration/IDbConnectionProvider.hpp"
#include "persistence/configuration/PostgresConnection.hpp"
#include "IRepository.hpp"
#include "domain/Group.hpp"
#include "domain/Utilities.hpp"

class GroupRepository : public IRepository<domain::Group, std::string>{
    std::shared_ptr<IDbConnectionProvider> connectionProvider;
public:
    GroupRepository(const std::shared_ptr<IDbConnectionProvider>& connectionProvider);
    std::shared_ptr<domain::Group> ReadById(std::string id) override;
    std::string Create (const domain::Group & entity) override;
    std::string Update (const domain::Group & entity) override;
    void Delete(std::string id) override;
    std::vector<std::shared_ptr<domain::Group>> ReadAll() override;

    // Additional methods for group operations
    virtual std::shared_ptr<domain::Group> FindByTournamentIdAndGroupId(const std::string_view& tournamentId, const std::string_view& groupId);
    virtual std::shared_ptr<domain::Group> FindByTournamentIdAndTeamId(const std::string_view& tournamentId, const std::string_view& teamId);
    virtual std::vector<std::shared_ptr<domain::Group>> FindByTournamentId(const std::string_view& tournamentId);
    virtual void UpdateGroupAddTeam(const std::string_view& groupId, const std::shared_ptr<domain::Team>& team);
    virtual bool ExistsByName(const std::string_view& tournamentId, const std::string& name);
};

inline GroupRepository::GroupRepository(const std::shared_ptr<IDbConnectionProvider>& connectionProvider) : connectionProvider(std::move(connectionProvider)) {}

inline std::shared_ptr<domain::Group> GroupRepository::ReadById(std::string id) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    pqxx::result result = tx.exec_params(
        "SELECT id, document FROM groups WHERE id = $1", id
    );
    tx.commit();

    if (result.empty()) {
        return nullptr;
    }

    nlohmann::json groupJson = nlohmann::json::parse(result[0]["document"].c_str());
    auto group = std::make_shared<domain::Group>(groupJson);
    group->Id() = result[0]["id"].c_str();

    return group;
}

inline std::string GroupRepository::Create (const domain::Group & entity) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    nlohmann::json groupBody;
    domain::to_json(groupBody, entity);

    pqxx::work tx(*(connection->connection));
    pqxx::result result = tx.exec(pqxx::prepped{"insert_group"}, groupBody.dump());

    tx.commit();

    return result[0]["id"].c_str();
}

inline std::string GroupRepository::Update (const domain::Group & entity) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    nlohmann::json groupBody;
    domain::to_json(groupBody, entity);

    pqxx::work tx(*(connection->connection));
    pqxx::result result = tx.exec_params(
        "UPDATE groups SET document = $1 WHERE id = $2 RETURNING id",
        groupBody.dump(), entity.Id()
    );
    tx.commit();

    if (result.empty()) {
        return "";
    }
    return result[0]["id"].c_str();
}

inline void GroupRepository::Delete(std::string id) {

}

inline std::vector<std::shared_ptr<domain::Group>> GroupRepository::ReadAll() {
    std::vector<std::shared_ptr<domain::Group>> teams;

    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    pqxx::result result{tx.exec("select id, document->>'name' as name from groups")};
    tx.commit();

    for(auto row : result){
        teams.push_back(std::make_shared<domain::Group>(domain::Group{row["id"].c_str(), row["name"].c_str()}));
    }

    return teams;
}

inline std::shared_ptr<domain::Group> GroupRepository::FindByTournamentIdAndGroupId(const std::string_view& tournamentId, const std::string_view& groupId) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    pqxx::result result = tx.exec_params(
        "SELECT id, document FROM groups WHERE id = $1 AND document->>'tournamentId' = $2",
        groupId.data(), tournamentId.data()
    );
    tx.commit();

    if (result.empty()) {
        return nullptr;
    }

    nlohmann::json groupJson = nlohmann::json::parse(result[0]["document"].c_str());
    auto group = std::make_shared<domain::Group>(groupJson);
    group->Id() = result[0]["id"].c_str();

    return group;
}

inline std::shared_ptr<domain::Group> GroupRepository::FindByTournamentIdAndTeamId(const std::string_view& tournamentId, const std::string_view& teamId) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    pqxx::result result = tx.exec_params(
        "SELECT id, document FROM groups WHERE document->>'tournamentId' = $1 AND document->'teams' @> $2::jsonb",
        tournamentId.data(), nlohmann::json({{"id", teamId.data()}}).dump()
    );
    tx.commit();

    if (result.empty()) {
        return nullptr;
    }

    nlohmann::json groupJson = nlohmann::json::parse(result[0]["document"].c_str());
    auto group = std::make_shared<domain::Group>(groupJson);
    group->Id() = result[0]["id"].c_str();

    return group;
}

inline std::vector<std::shared_ptr<domain::Group>> GroupRepository::FindByTournamentId(const std::string_view& tournamentId) {
    std::vector<std::shared_ptr<domain::Group>> groups;

    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    pqxx::result result = tx.exec_params(
        "SELECT id, document FROM groups WHERE document->>'tournamentId' = $1",
        tournamentId.data()
    );
    tx.commit();

    for(auto row : result){
        nlohmann::json groupJson = nlohmann::json::parse(row["document"].c_str());
        auto group = std::make_shared<domain::Group>(groupJson);
        group->Id() = row["id"].c_str();
        groups.push_back(group);
    }

    return groups;
}

inline void GroupRepository::UpdateGroupAddTeam(const std::string_view& groupId, const std::shared_ptr<domain::Team>& team) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    nlohmann::json teamJson;
    domain::to_json(teamJson, team);

    pqxx::work tx(*(connection->connection));
    tx.exec_params(
        "UPDATE groups SET document = jsonb_set(document, '{teams}', "
        "COALESCE(document->'teams', '[]'::jsonb) || $1::jsonb) WHERE id = $2",
        teamJson.dump(), groupId.data()
    );
    tx.commit();
}

inline bool GroupRepository::ExistsByName(const std::string_view& tournamentId, const std::string& name) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    pqxx::result result = tx.exec_params(
        "SELECT COUNT(*) as count FROM groups WHERE document->>'tournamentId' = $1 AND document->>'name' = $2",
        tournamentId.data(), name
    );
    tx.commit();

    return result[0]["count"].as<int>() > 0;
}

#endif //TOURNAMENTS_GROUPREPOSITORY_HPP