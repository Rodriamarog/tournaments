//
// Created by tomas on 9/7/25.
//

#ifndef TOURNAMENTS_CONSUMER_CONTAINER_SETUP_HPP
#define TOURNAMENTS_CONSUMER_CONTAINER_SETUP_HPP

#include <Hypodermic/Hypodermic.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <memory>

#include "configuration/DatabaseConfiguration.hpp"
#include "persistence/repository/IRepository.hpp"
#include "persistence/repository/TeamRepository.hpp"
#include "persistence/configuration/PostgresConnectionProvider.hpp"
#include "persistence/repository/TournamentRepository.hpp"

namespace config {
    inline std::shared_ptr<Hypodermic::Container> containerSetup() {
        Hypodermic::ContainerBuilder builder;

        std::ifstream file("configuration.json");
        nlohmann::json configuration;
        file >> configuration;

        std::shared_ptr<PostgresConnectionProvider> postgressConnection = std::make_shared<PostgresConnectionProvider>(configuration["databaseConfig"]["connectionString"].get<std::string>(), configuration["databaseConfig"]["poolSize"].get<size_t>());
        builder.registerInstance(postgressConnection).as<IDbConnectionProvider>();

        // ActiveMQ dependencies removed - to be replaced with alternative message queue
        // builder.registerType<ConnectionManager>()...
        // builder.registerType<QueueMessageConsumer>()...

        builder.registerType<TeamRepository>().as<IRepository<domain::Team, std::string_view>>().singleInstance();

        builder.registerType<TournamentRepository>().as<IRepository<domain::Tournament, std::string>>().singleInstance();

        return builder.build();
    }
}
#endif //TOURNAMENTS_CONSUMER_CONTAINER_SETUP_HPP