//
// Created by tomas on 8/22/25.
//

#ifndef RESTAPI_CONTAINER_SETUP_HPP
#define RESTAPI_CONTAINER_SETUP_HPP

#include <Hypodermic/Hypodermic.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <memory>

#include "persistence/repository/IRepository.hpp"
#include "persistence/repository/TeamRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"
#include "persistence/repository/GroupRepository.hpp"
#include "RunConfiguration.hpp"
#include "delegate/TeamDelegate.hpp"
#include "delegate/TournamentDelegate.hpp"
#include "delegate/GroupDelegate.hpp"
#include "controller/TeamController.hpp"
#include "controller/TournamentController.hpp"
#include "controller/GroupController.hpp"
#include "persistence/configuration/PostgresConnectionProvider.hpp"

namespace config {
    inline std::shared_ptr<Hypodermic::Container> containerSetup() {
        Hypodermic::ContainerBuilder builder;

        std::ifstream file("configuration.json");
        nlohmann::json configuration;
        file >> configuration;
        std::shared_ptr<RunConfiguration> appConfig = std::make_shared<RunConfiguration>(configuration["runConfig"]);
        builder.registerInstance(appConfig);

        std::shared_ptr<PostgresConnectionProvider> postgressConnection = std::make_shared<PostgresConnectionProvider>(
            configuration["databaseConfig"]["connectionString"].get<std::string>(),
            configuration["databaseConfig"]["poolSize"].get<size_t>());
        builder.registerInstance(postgressConnection).as<IDbConnectionProvider>();


        builder.registerType<TeamRepository>().as<IRepository<domain::Team, std::string_view> >().singleInstance();
        builder.registerType<TeamDelegate>().as<ITeamDelegate>().singleInstance();
        builder.registerType<TeamController>().singleInstance();

        builder.registerType<TournamentRepository>().as<IRepository<domain::Tournament, std::string> >().singleInstance();
        builder.registerType<TournamentDelegate>().as<ITournamentDelegate>().singleInstance();
        builder.registerType<TournamentController>().singleInstance();

        builder.registerType<GroupRepository>().singleInstance();
        builder.registerType<GroupDelegate>().as<IGroupDelegate>().singleInstance();
        builder.registerType<GroupController>().singleInstance();

        return builder.build();
    }
}
#endif //RESTAPI_CONTAINER_SETUP_HPP
