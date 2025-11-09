#ifndef SERVICE_MATCH_CONTROLLER_HPP
#define SERVICE_MATCH_CONTROLLER_HPP

#include <crow.h>
#include <memory>
#include <nlohmann/json.hpp>

#include "delegate/IMatchDelegate.hpp"
#include "domain/Match.hpp"

class MatchController {
    std::shared_ptr<IMatchDelegate> matchDelegate;

public:
    explicit MatchController(const std::shared_ptr<IMatchDelegate>& matchDelegate)
        : matchDelegate(matchDelegate) {}

    crow::response GetMatches(const crow::request& request, const std::string& tournamentId) {
        crow::response response;

        try {
            // Parse query parameters for filtering
            std::optional<std::string> statusFilter = std::nullopt;
            char* showMatchesParam = request.url_params.get("showMatches");
            if (showMatchesParam != nullptr) {
                std::string filterValue(showMatchesParam);
                if (filterValue == "played" || filterValue == "pending") {
                    statusFilter = filterValue;
                } else {
                    response.code = crow::BAD_REQUEST;
                    response.body = "Invalid showMatches parameter. Use 'played' or 'pending'";
                    return response;
                }
            }

            auto result = matchDelegate->GetMatches(tournamentId, statusFilter);

            if (result.has_value()) {
                nlohmann::json jsonArray = nlohmann::json::array();
                for (const auto& match : result.value()) {
                    nlohmann::json matchJson;
                    domain::to_json(matchJson, match);
                    jsonArray.push_back(matchJson);
                }
                response.code = crow::OK;
                response.body = jsonArray.dump();
                response.add_header("Content-Type", "application/json");
            } else {
                if (result.error().find("doesn't exist") != std::string::npos) {
                    response.code = crow::NOT_FOUND;
                } else {
                    response.code = crow::INTERNAL_SERVER_ERROR;
                }
                response.body = result.error();
            }
        } catch (const std::exception& e) {
            response.code = crow::INTERNAL_SERVER_ERROR;
            response.body = std::string("Internal server error: ") + e.what();
        }

        return response;
    }

    crow::response GetMatch(const crow::request& request, const std::string& tournamentId, const std::string& matchId) {
        crow::response response;

        try {
            auto result = matchDelegate->GetMatch(tournamentId, matchId);

            if (result.has_value()) {
                nlohmann::json matchJson;
                domain::to_json(matchJson, result.value());
                response.code = crow::OK;
                response.body = matchJson.dump();
                response.add_header("Content-Type", "application/json");
            } else {
                if (result.error().find("doesn't exist") != std::string::npos) {
                    response.code = crow::NOT_FOUND;
                } else {
                    response.code = crow::INTERNAL_SERVER_ERROR;
                }
                response.body = result.error();
            }
        } catch (const std::exception& e) {
            response.code = crow::INTERNAL_SERVER_ERROR;
            response.body = std::string("Internal server error: ") + e.what();
        }

        return response;
    }

    crow::response UpdateScore(const crow::request& request, const std::string& tournamentId, const std::string& matchId) {
        crow::response response;

        try {
            // Parse JSON body
            nlohmann::json requestBody;
            try {
                requestBody = nlohmann::json::parse(request.body);
            } catch (const nlohmann::json::parse_error& e) {
                response.code = crow::BAD_REQUEST;
                response.body = "Invalid JSON format";
                return response;
            }

            // Validate required fields
            if (!requestBody.contains("score") ||
                !requestBody["score"].contains("home") ||
                !requestBody["score"].contains("visitor")) {
                response.code = crow::BAD_REQUEST;
                response.body = "Missing required fields: score.home, score.visitor";
                return response;
            }

            // Extract score
            domain::Score score;
            try {
                score.home = requestBody["score"]["home"].get<int>();
                score.visitor = requestBody["score"]["visitor"].get<int>();
            } catch (const std::exception& e) {
                response.code = crow::BAD_REQUEST;
                response.body = "Invalid score format. Expected integers.";
                return response;
            }

            // Update score via delegate
            auto result = matchDelegate->UpdateScore(tournamentId, matchId, score);

            if (result.has_value()) {
                response.code = crow::NO_CONTENT;
            } else {
                // Map error to appropriate HTTP status
                if (result.error().find("doesn't exist") != std::string::npos) {
                    response.code = crow::NOT_FOUND;
                } else if (result.error().find("not allowed") != std::string::npos ||
                           result.error().find("cannot be negative") != std::string::npos) {
                    // HTTP 422 would be ideal, but Crow doesn't support it (returns 500)
                    response.code = 422;
                } else {
                    response.code = crow::INTERNAL_SERVER_ERROR;
                }
                response.body = result.error();
            }
        } catch (const std::exception& e) {
            response.code = crow::INTERNAL_SERVER_ERROR;
            response.body = std::string("Internal server error: ") + e.what();
        }

        return response;
    }
};

#endif /* SERVICE_MATCH_CONTROLLER_HPP */
