//
// Created by root on 9/27/25.
//

#include "configuration/RouteDefinition.hpp"
#include "controller/TeamController.hpp"
#include "domain/Utilities.hpp"


TeamController::TeamController(const std::shared_ptr<ITeamDelegate>& teamDelegate) : teamDelegate(teamDelegate) {}

crow::response TeamController::getTeam(const std::string& teamId) const {
    if(!std::regex_match(teamId, domain::ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }

    if(auto team = teamDelegate->GetTeam(teamId); team != nullptr) {
        nlohmann::json body = team;
        auto response = crow::response{crow::OK, body.dump()};
        response.add_header("Content-Type", "application/json");
        return response;
    }
    return crow::response{crow::NOT_FOUND, "team not found"};
}

crow::response TeamController::getAllTeams() const {

    nlohmann::json body = teamDelegate->GetAllTeams();
    return crow::response{200, body.dump()};
}

crow::response TeamController::SaveTeam(const crow::request& request) const {
    crow::response response;

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

    domain::Team team = requestBody;

    auto createResult = teamDelegate->SaveTeam(team);
    if (createResult.has_value()) {
        response.code = crow::CREATED;
        response.add_header("location", createResult.value());
    } else {
        response.code = 409;  // HTTP 409 Conflict
        response.body = createResult.error();
    }

    return response;
}

crow::response TeamController::UpdateTeam(const crow::request& request, const std::string& teamId) const {
    crow::response response;

    if(!std::regex_match(teamId, domain::ID_VALUE)) {
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
    domain::Team team = requestBody;
    team.Id = teamId;

    auto updateResult = teamDelegate->UpdateTeam(team);
    if (updateResult.has_value()) {
        response.code = 204;  // HTTP 204 No Content
    } else {
        response.code = crow::NOT_FOUND;
        response.body = updateResult.error();
    }

    return response;
}

REGISTER_ROUTE(TeamController, getTeam, "/teams/<string>", "GET"_method)
REGISTER_ROUTE(TeamController, getAllTeams, "/teams", "GET"_method)
REGISTER_ROUTE(TeamController, SaveTeam, "/teams", "POST"_method)
REGISTER_ROUTE(TeamController, UpdateTeam, "/teams/<string>", "PATCH"_method)