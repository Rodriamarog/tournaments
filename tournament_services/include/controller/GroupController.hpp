#ifndef A7B3517D_1DC1_4B59_A78C_D3E03D29710C
#define A7B3517D_1DC1_4B59_A78C_D3E03D29710C

#include <vector>
#include <string>
#include <memory>
#include <crow.h>
#include <nlohmann/json.hpp>

#include "configuration/RouteDefinition.hpp"
#include "delegate/IGroupDelegate.hpp"
#include "domain/Group.hpp"
#include "domain/Utilities.hpp"


class GroupController
{
    std::shared_ptr<IGroupDelegate> groupDelegate;
public:
    GroupController(const std::shared_ptr<IGroupDelegate>& delegate);
    ~GroupController();
    crow::response GetGroups(const std::string& tournamentId);
    crow::response GetGroup(const std::string& tournamentId, const std::string& groupId);
    crow::response CreateGroup(const crow::request& request, const std::string& tournamentId);
    crow::response UpdateGroup(const crow::request& request, const std::string& tournamentId, const std::string& groupId);
    crow::response AddTeamToGroup(const crow::request& request, const std::string& tournamentId, const std::string& groupId);
};

GroupController::GroupController(const std::shared_ptr<IGroupDelegate>& delegate) : groupDelegate(std::move(delegate)) {}

GroupController::~GroupController()
{
}

crow::response GroupController::GetGroups(const std::string& tournamentId){
    if(!std::regex_match(tournamentId, domain::ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid tournament ID format"};
    }

    auto groupsResult = groupDelegate->GetGroups(tournamentId);
    crow::response response;

    if (groupsResult.has_value()) {
        nlohmann::json body = groupsResult.value();
        response.code = crow::OK;
        response.body = body.dump();
        response.add_header("Content-Type", "application/json");
    } else {
        response.code = crow::NOT_FOUND;
        response.body = groupsResult.error();
    }

    return response;
}

crow::response GroupController::GetGroup(const std::string& tournamentId, const std::string& groupId){
    if(!std::regex_match(tournamentId, domain::ID_VALUE) || !std::regex_match(groupId, domain::ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }

    auto groupResult = groupDelegate->GetGroup(tournamentId, groupId);
    crow::response response;

    if (groupResult.has_value()) {
        nlohmann::json body = groupResult.value();
        response.code = crow::OK;
        response.body = body.dump();
        response.add_header("Content-Type", "application/json");
    } else {
        response.code = crow::NOT_FOUND;
        response.body = groupResult.error();
    }

    return response;
}

crow::response GroupController::CreateGroup(const crow::request& request, const std::string& tournamentId){
    crow::response response;

    if(!std::regex_match(tournamentId, domain::ID_VALUE)) {
        response.code = crow::BAD_REQUEST;
        response.body = "Invalid tournament ID format";
        return response;
    }

    if(!nlohmann::json::accept(request.body)) {
        response.code = crow::BAD_REQUEST;
        response.body = "Invalid JSON";
        return response;
    }

    auto requestBody = nlohmann::json::parse(request.body);

    // Validate required fields
    if (!requestBody.contains("name")) {
        response.code = crow::BAD_REQUEST;
        response.body = "Missing required field: name";
        return response;
    }

    domain::Group group = requestBody;

    auto groupId = groupDelegate->CreateGroup(tournamentId, group);
    if (groupId.has_value()) {
        response.add_header("location", groupId.value());
        response.code = crow::CREATED;
    } else {
        // Check if it's a duplicate error
        if (groupId.error().find("already exists") != std::string::npos) {
            response.code = 409;  // HTTP 409 Conflict
        } else {
            response.code = 422;  // HTTP 422 Unprocessable Entity
        }
        response.body = groupId.error();
    }

    return response;
}

crow::response GroupController::UpdateGroup(const crow::request& request, const std::string& tournamentId, const std::string& groupId){
    crow::response response;

    if(!std::regex_match(tournamentId, domain::ID_VALUE) || !std::regex_match(groupId, domain::ID_VALUE)) {
        response.code = crow::BAD_REQUEST;
        response.body = "Invalid ID format";
        return response;
    }

    if(!nlohmann::json::accept(request.body)) {
        response.code = crow::BAD_REQUEST;
        response.body = "Invalid JSON";
        return response;
    }

    auto requestBody = nlohmann::json::parse(request.body);
    domain::Group group = requestBody;
    group.Id() = groupId;
    group.TournamentId() = tournamentId;

    auto updateResult = groupDelegate->UpdateGroup(tournamentId, group);
    if (updateResult.has_value()) {
        response.code = 204;  // HTTP 204 No Content
    } else {
        response.code = crow::NOT_FOUND;
        response.body = updateResult.error();
    }

    return response;
}

crow::response GroupController::AddTeamToGroup(const crow::request& request, const std::string& tournamentId, const std::string& groupId){
    crow::response response;

    if(!std::regex_match(tournamentId, domain::ID_VALUE) || !std::regex_match(groupId, domain::ID_VALUE)) {
        response.code = crow::BAD_REQUEST;
        response.body = "Invalid ID format";
        return response;
    }

    if(!nlohmann::json::accept(request.body)) {
        response.code = crow::BAD_REQUEST;
        response.body = "Invalid JSON";
        return response;
    }

    auto requestBody = nlohmann::json::parse(request.body);

    // Validate required fields for team
    if (!requestBody.contains("id")) {
        response.code = crow::BAD_REQUEST;
        response.body = "Missing required field: id";
        return response;
    }

    domain::Team team = requestBody;

    // Call UpdateTeams with a single-element vector
    std::vector<domain::Team> teams = {team};
    auto updateResult = groupDelegate->UpdateTeams(tournamentId, groupId, teams);

    if (updateResult.has_value()) {
        response.code = 204;  // HTTP 204 No Content
    } else {
        // Check error type for proper HTTP status code
        std::string error = updateResult.error();
        if (error.find("doesn't exist") != std::string::npos ||
            error.find("max capacity") != std::string::npos ||
            error.find("already exist") != std::string::npos) {
            response.code = 422;  // HTTP 422 Unprocessable Entity
        } else {
            response.code = crow::NOT_FOUND;  // HTTP 404 for group/tournament not found
        }
        response.body = error;
    }

    return response;
}

REGISTER_ROUTE(GroupController, GetGroups, "/tournaments/<string>/groups", "GET"_method)
REGISTER_ROUTE(GroupController, GetGroup, "/tournaments/<string>/groups/<string>", "GET"_method)
REGISTER_ROUTE(GroupController, CreateGroup, "/tournaments/<string>/groups", "POST"_method)
REGISTER_ROUTE(GroupController, UpdateGroup, "/tournaments/<string>/groups/<string>", "PATCH"_method)
REGISTER_ROUTE(GroupController, AddTeamToGroup, "/tournaments/<string>/groups/<string>", "POST"_method)

#endif /* A7B3517D_1DC1_4B59_A78C_D3E03D29710C */
