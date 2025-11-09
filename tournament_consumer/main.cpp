#include <iostream>
#include <memory>
#include <csignal>
#include <fstream>
#include <nlohmann/json.hpp>

#include <activemq/library/ActiveMQCPP.h>
#include <activemq/core/ActiveMQConnectionFactory.h>
#include <cms/Connection.h>

#include "consumer/TeamAddedConsumer.hpp"
#include "consumer/ScoreRegisteredConsumer.hpp"
#include "service/MatchGenerationService.hpp"
#include "service/PlayoffGenerationService.hpp"
#include "persistence/repository/MatchRepository.hpp"
#include "persistence/repository/GroupRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"
#include "persistence/configuration/PostgresConnection.hpp"
#include "persistence/configuration/PostgresConnectionProvider.hpp"

// Global flag for graceful shutdown
static bool running = true;

void signalHandler(int signal) {
    std::cout << "\n[Consumer] Received signal " << signal << ", shutting down gracefully..." << std::endl;
    running = false;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "   Tournament Consumer - ActiveMQ" << std::endl;
    std::cout << "========================================" << std::endl;

    try {
        // Register signal handlers for graceful shutdown
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);

        // Load configuration
        std::cout << "[Consumer] Loading configuration..." << std::endl;
        std::ifstream configFile("configuration.json");
        if (!configFile.is_open()) {
            std::cerr << "[Consumer] ERROR: Could not open configuration.json" << std::endl;
            return 1;
        }

        nlohmann::json config;
        configFile >> config;
        configFile.close();

        // Extract configuration
        std::string brokerURI = config["messagingConfig"]["brokerURI"];
        std::string dbConnectionString = config["databaseConfig"]["connectionString"];

        std::cout << "[Consumer] Broker URI: " << brokerURI << std::endl;
        std::cout << "[Consumer] Database: " << dbConnectionString.substr(0, 30) << "..." << std::endl;

        // Initialize ActiveMQ library
        activemq::library::ActiveMQCPP::initializeLibrary();
        std::cout << "[Consumer] ActiveMQ library initialized" << std::endl;

        // Create database connection provider (pool size = 5)
        auto connectionProvider = std::make_shared<PostgresConnectionProvider>(dbConnectionString, 5);
        std::cout << "[Consumer] Database connection provider created (pool size: 5)" << std::endl;

        // Create repositories
        auto matchRepository = std::make_shared<MatchRepository>(connectionProvider);
        auto groupRepository = std::make_shared<GroupRepository>(connectionProvider);
        auto tournamentRepository = std::make_shared<TournamentRepository>(connectionProvider);
        std::cout << "[Consumer] Repositories initialized" << std::endl;

        // Create match generation service
        auto matchGenerationService = std::make_shared<MatchGenerationService>(
            matchRepository,
            groupRepository,
            tournamentRepository
        );
        std::cout << "[Consumer] MatchGenerationService created" << std::endl;

        // Create playoff generation service
        auto playoffGenerationService = std::make_shared<PlayoffGenerationService>(
            matchRepository,
            groupRepository,
            tournamentRepository
        );
        std::cout << "[Consumer] PlayoffGenerationService created" << std::endl;

        // Create ActiveMQ connection
        activemq::core::ActiveMQConnectionFactory connectionFactory(brokerURI);
        cms::Connection* connection = connectionFactory.createConnection();
        connection->start();
        std::cout << "[Consumer] Connected to ActiveMQ broker at " << brokerURI << std::endl;

        // Create and start both consumers
        TeamAddedConsumer teamAddedConsumer(matchGenerationService, connection);
        teamAddedConsumer.Start();

        ScoreRegisteredConsumer scoreRegisteredConsumer(playoffGenerationService, connection);
        scoreRegisteredConsumer.Start();

        std::cout << "\n[Consumer] Listening for events... Press Ctrl+C to stop." << std::endl;
        std::cout << "========================================\n" << std::endl;

        // Keep the consumer running
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Graceful shutdown
        std::cout << "\n[Consumer] Shutting down..." << std::endl;
        teamAddedConsumer.Stop();
        scoreRegisteredConsumer.Stop();

        connection->close();
        delete connection;

        activemq::library::ActiveMQCPP::shutdownLibrary();

        std::cout << "[Consumer] Shutdown complete" << std::endl;
        return 0;

    } catch (cms::CMSException& e) {
        std::cerr << "[Consumer] CMS Exception: " << e.getMessage() << std::endl;
        activemq::library::ActiveMQCPP::shutdownLibrary();
        return 1;
    } catch (std::exception& e) {
        std::cerr << "[Consumer] Exception: " << e.what() << std::endl;
        activemq::library::ActiveMQCPP::shutdownLibrary();
        return 1;
    }
}
