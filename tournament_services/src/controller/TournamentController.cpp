//
// Created by tsuny on 8/31/25.
//

#include "configuration/RouteDefinition.hpp"
#include "controller/TournamentController.hpp"

#include <string>
#include <utility>
#include  "domain/Tournament.hpp"
#include "domain/Utilities.hpp"

TournamentController::TournamentController(std::shared_ptr<ITournamentDelegate> delegate) : tournamentDelegate(std::move(delegate)) {}

crow::response TournamentController::CreateTournament(const crow::request &request) const {
    crow::response response;

    if(!nlohmann::json::accept(request.body)) {
        response.code = crow::BAD_REQUEST;
        return response;
    }

    nlohmann::json body = nlohmann::json::parse(request.body);

    // Validate required fields
    if (!body.contains("name")) {
        response.code = crow::BAD_REQUEST;
        response.body = "Missing required field: name";
        return response;
    }

    const std::shared_ptr<domain::Tournament> tournament = std::make_shared<domain::Tournament>(body);

    auto createResult = tournamentDelegate->CreateTournament(tournament);
    if (createResult.has_value()) {
        response.code = crow::CREATED;
        response.add_header("location", createResult.value());
    } else {
        response.code = 409;  // HTTP 409 Conflict
        response.body = createResult.error();
    }

    return response;
}

crow::response TournamentController::GetTournament(const std::string& tournamentId) const {
    if(!std::regex_match(tournamentId, domain::ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }

    if(auto tournament = tournamentDelegate->GetTournament(tournamentId); tournament != nullptr) {
        nlohmann::json body = tournament;
        auto response = crow::response{crow::OK, body.dump()};
        response.add_header("Content-Type", "application/json");
        return response;
    }
    return crow::response{crow::NOT_FOUND, "tournament not found"};
}

crow::response TournamentController::ReadAll() const {
    nlohmann::json body = tournamentDelegate->ReadAll();
    crow::response response;
    response.code = crow::OK;
    response.body = body.dump();

    return response;
}

crow::response TournamentController::UpdateTournament(const crow::request &request, const std::string& tournamentId) const {
    crow::response response;

    if(!std::regex_match(tournamentId, domain::ID_VALUE)) {
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
    domain::Tournament tournament = requestBody;
    tournament.Id() = tournamentId;

    auto updateResult = tournamentDelegate->UpdateTournament(tournament);
    if (updateResult.has_value()) {
        response.code = 204;  // HTTP 204 No Content
    } else {
        response.code = crow::NOT_FOUND;
        response.body = updateResult.error();
    }

    return response;
}


REGISTER_ROUTE(TournamentController, CreateTournament, "/tournaments", "POST"_method)
REGISTER_ROUTE(TournamentController, GetTournament, "/tournaments/<string>", "GET"_method)
REGISTER_ROUTE(TournamentController, ReadAll, "/tournaments", "GET"_method)
REGISTER_ROUTE(TournamentController, UpdateTournament, "/tournaments/<string>", "PATCH"_method)