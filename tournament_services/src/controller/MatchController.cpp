//
// Match Controller Route Registrations
//

#include "controller/MatchController.hpp"
#include "configuration/RouteDefinition.hpp"

// Register Match routes
REGISTER_ROUTE(MatchController, GetMatches, "/tournaments/<string>/matches", crow::HTTPMethod::GET)
REGISTER_ROUTE(MatchController, GetMatch, "/tournaments/<string>/matches/<string>", crow::HTTPMethod::GET)
REGISTER_ROUTE(MatchController, UpdateScore, "/tournaments/<string>/matches/<string>", crow::HTTPMethod::PATCH)
