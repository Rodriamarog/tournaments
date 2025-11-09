#ifndef CONSUMER_TEAM_ADDED_CONSUMER_HPP
#define CONSUMER_TEAM_ADDED_CONSUMER_HPP

#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include <activemq/library/ActiveMQCPP.h>
#include <cms/Connection.h>
#include <cms/Session.h>
#include <cms/MessageConsumer.h>
#include <cms/TextMessage.h>
#include <cms/MessageListener.h>

#include "service/MatchGenerationService.hpp"

/**
 * ActiveMQ consumer that listens for "tournament.team-add" events
 * and generates round-robin matches when a group becomes full.
 *
 * Event payload:
 * {
 *   "tournamentId": "...",
 *   "groupId": "...",
 *   "teamId": "..."
 * }
 *
 * Flow:
 * 1. Receive team-added event from queue
 * 2. Check if group is now full
 * 3. If full, generate all round-robin matches for the group
 * 4. Acknowledge message
 */
class TeamAddedConsumer : public cms::MessageListener {
    std::shared_ptr<MatchGenerationService> matchGenerationService;
    cms::Connection* connection;
    cms::Session* session;
    cms::MessageConsumer* consumer;

public:
    TeamAddedConsumer(
        const std::shared_ptr<MatchGenerationService>& matchGenerationService,
        cms::Connection* connection
    );

    ~TeamAddedConsumer();

    /**
     * Start listening for messages on the queue.
     * This method blocks and processes messages asynchronously.
     */
    void Start();

    /**
     * Stop listening and clean up resources.
     */
    void Stop();

    /**
     * ActiveMQ callback when a message is received.
     * Implements cms::MessageListener::onMessage
     */
    void onMessage(const cms::Message* message) override;

private:
    /**
     * Process the team-added event and trigger match generation if needed.
     */
    void ProcessTeamAddedEvent(const nlohmann::json& eventData);
};

inline TeamAddedConsumer::TeamAddedConsumer(
    const std::shared_ptr<MatchGenerationService>& matchGenerationService,
    cms::Connection* connection
)
    : matchGenerationService(matchGenerationService),
      connection(connection),
      session(nullptr),
      consumer(nullptr) {}

inline TeamAddedConsumer::~TeamAddedConsumer() {
    Stop();
}

inline void TeamAddedConsumer::Start() {
    try {
        // Create session (non-transacted, auto-acknowledge)
        session = connection->createSession(cms::Session::AUTO_ACKNOWLEDGE);

        // Create destination (queue)
        cms::Destination* destination = session->createQueue("tournament.team-add");

        // Create consumer
        consumer = session->createConsumer(destination);

        // Register this as message listener
        consumer->setMessageListener(this);

        delete destination;

        std::cout << "[TeamAddedConsumer] Started listening on 'tournament.team-add' queue" << std::endl;
    } catch (cms::CMSException& e) {
        std::cerr << "[TeamAddedConsumer] Error starting consumer: " << e.getMessage() << std::endl;
        throw;
    }
}

inline void TeamAddedConsumer::Stop() {
    try {
        if (consumer != nullptr) {
            consumer->close();
            delete consumer;
            consumer = nullptr;
        }

        if (session != nullptr) {
            session->close();
            delete session;
            session = nullptr;
        }

        std::cout << "[TeamAddedConsumer] Stopped" << std::endl;
    } catch (cms::CMSException& e) {
        std::cerr << "[TeamAddedConsumer] Error stopping consumer: " << e.getMessage() << std::endl;
    }
}

inline void TeamAddedConsumer::onMessage(const cms::Message* message) {
    try {
        const cms::TextMessage* textMessage = dynamic_cast<const cms::TextMessage*>(message);
        if (textMessage == nullptr) {
            std::cerr << "[TeamAddedConsumer] Received non-text message, ignoring" << std::endl;
            return;
        }

        std::string messageText = textMessage->getText();
        std::cout << "[TeamAddedConsumer] Received message: " << messageText << std::endl;

        // Parse JSON payload
        nlohmann::json eventData = nlohmann::json::parse(messageText);

        // Process the event
        ProcessTeamAddedEvent(eventData);

    } catch (cms::CMSException& e) {
        std::cerr << "[TeamAddedConsumer] CMS Error processing message: " << e.getMessage() << std::endl;
    } catch (nlohmann::json::exception& e) {
        std::cerr << "[TeamAddedConsumer] JSON parsing error: " << e.what() << std::endl;
    } catch (std::exception& e) {
        std::cerr << "[TeamAddedConsumer] Error processing message: " << e.what() << std::endl;
    }
}

inline void TeamAddedConsumer::ProcessTeamAddedEvent(const nlohmann::json& eventData) {
    // Extract event data
    std::string tournamentId = eventData["tournamentId"];
    std::string groupId = eventData["groupId"];
    std::string teamId = eventData["teamId"];

    std::cout << "[TeamAddedConsumer] Processing team-added event: "
              << "Tournament=" << tournamentId << ", "
              << "Group=" << groupId << ", "
              << "Team=" << teamId << std::endl;

    // Check if group is ready for match generation
    if (matchGenerationService->IsGroupReadyForMatches(tournamentId, groupId)) {
        std::cout << "[TeamAddedConsumer] Group is full! Generating round-robin matches..." << std::endl;

        auto result = matchGenerationService->GenerateRoundRobinMatches(tournamentId, groupId);

        if (result.has_value()) {
            std::cout << "[TeamAddedConsumer] Successfully generated matches for group " << groupId << std::endl;
        } else {
            std::cerr << "[TeamAddedConsumer] Failed to generate matches: " << result.error() << std::endl;
        }
    } else {
        std::cout << "[TeamAddedConsumer] Group not yet full or matches already exist. Skipping match generation." << std::endl;
    }
}

#endif /* CONSUMER_TEAM_ADDED_CONSUMER_HPP */
