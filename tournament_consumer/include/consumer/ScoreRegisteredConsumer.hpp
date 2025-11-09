#ifndef CONSUMER_SCORE_REGISTERED_CONSUMER_HPP
#define CONSUMER_SCORE_REGISTERED_CONSUMER_HPP

#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include <activemq/library/ActiveMQCPP.h>
#include <cms/Connection.h>
#include <cms/Session.h>
#include <cms/MessageConsumer.h>
#include <cms/TextMessage.h>
#include <cms/MessageListener.h>

#include "service/PlayoffGenerationService.hpp"

/**
 * ActiveMQ consumer that listens for "tournament.score-registered" events
 * and triggers playoff generation when appropriate.
 *
 * Event payload:
 * {
 *   "tournamentId": "...",
 *   "matchId": "...",
 *   "round": "regular|quarterfinals|semifinals|finals",
 *   "winnerId": "..." (optional - can be null for draws)
 * }
 *
 * Flow:
 * 1. Receive score-registered event from queue
 * 2. If round is "regular" -> Check if all group matches complete -> Generate quarterfinals
 * 3. If round is "quarterfinals" -> Check if all QF complete -> Generate semifinals
 * 4. If round is "semifinals" -> Check if all SF complete -> Generate finals
 * 5. Acknowledge message
 */
class ScoreRegisteredConsumer : public cms::MessageListener {
    std::shared_ptr<PlayoffGenerationService> playoffGenerationService;
    cms::Connection* connection;
    cms::Session* session;
    cms::MessageConsumer* consumer;

public:
    ScoreRegisteredConsumer(
        const std::shared_ptr<PlayoffGenerationService>& playoffGenerationService,
        cms::Connection* connection
    );

    ~ScoreRegisteredConsumer();

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
     * Process the score-registered event and trigger playoff generation if needed.
     */
    void ProcessScoreRegisteredEvent(const nlohmann::json& eventData);
};

inline ScoreRegisteredConsumer::ScoreRegisteredConsumer(
    const std::shared_ptr<PlayoffGenerationService>& playoffGenerationService,
    cms::Connection* connection
)
    : playoffGenerationService(playoffGenerationService),
      connection(connection),
      session(nullptr),
      consumer(nullptr) {}

inline ScoreRegisteredConsumer::~ScoreRegisteredConsumer() {
    Stop();
}

inline void ScoreRegisteredConsumer::Start() {
    try {
        // Create session (non-transacted, auto-acknowledge)
        session = connection->createSession(cms::Session::AUTO_ACKNOWLEDGE);

        // Create destination (queue)
        cms::Destination* destination = session->createQueue("tournament.score-registered");

        // Create consumer
        consumer = session->createConsumer(destination);

        // Register this as message listener
        consumer->setMessageListener(this);

        delete destination;

        std::cout << "[ScoreRegisteredConsumer] Started listening on 'tournament.score-registered' queue" << std::endl;
    } catch (cms::CMSException& e) {
        std::cerr << "[ScoreRegisteredConsumer] Error starting consumer: " << e.getMessage() << std::endl;
        throw;
    }
}

inline void ScoreRegisteredConsumer::Stop() {
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

        std::cout << "[ScoreRegisteredConsumer] Stopped" << std::endl;
    } catch (cms::CMSException& e) {
        std::cerr << "[ScoreRegisteredConsumer] Error stopping consumer: " << e.getMessage() << std::endl;
    }
}

inline void ScoreRegisteredConsumer::onMessage(const cms::Message* message) {
    try {
        const cms::TextMessage* textMessage = dynamic_cast<const cms::TextMessage*>(message);
        if (textMessage == nullptr) {
            std::cerr << "[ScoreRegisteredConsumer] Received non-text message, ignoring" << std::endl;
            return;
        }

        std::string messageText = textMessage->getText();
        std::cout << "[ScoreRegisteredConsumer] Received message: " << messageText << std::endl;

        // Parse JSON payload
        nlohmann::json eventData = nlohmann::json::parse(messageText);

        // Process the event
        ProcessScoreRegisteredEvent(eventData);

    } catch (cms::CMSException& e) {
        std::cerr << "[ScoreRegisteredConsumer] CMS Error processing message: " << e.getMessage() << std::endl;
    } catch (nlohmann::json::exception& e) {
        std::cerr << "[ScoreRegisteredConsumer] JSON parsing error: " << e.what() << std::endl;
    } catch (std::exception& e) {
        std::cerr << "[ScoreRegisteredConsumer] Error processing message: " << e.what() << std::endl;
    }
}

inline void ScoreRegisteredConsumer::ProcessScoreRegisteredEvent(const nlohmann::json& eventData) {
    // Extract event data
    std::string tournamentId = eventData["tournamentId"];
    std::string matchId = eventData["matchId"];
    std::string round = eventData["round"];

    std::cout << "[ScoreRegisteredConsumer] Processing score-registered event: "
              << "Tournament=" << tournamentId << ", "
              << "Match=" << matchId << ", "
              << "Round=" << round << std::endl;

    // Handle based on round type
    if (round == "regular") {
        // Check if all group stage matches are completed
        if (playoffGenerationService->AreAllGroupMatchesCompleted(tournamentId)) {
            std::cout << "[ScoreRegisteredConsumer] All group matches completed! Generating quarterfinals..." << std::endl;

            auto result = playoffGenerationService->GenerateQuarterfinals(tournamentId);

            if (result.has_value()) {
                std::cout << "[ScoreRegisteredConsumer] Successfully generated quarterfinals for tournament " << tournamentId << std::endl;
            } else {
                std::cerr << "[ScoreRegisteredConsumer] Failed to generate quarterfinals: " << result.error() << std::endl;
            }
        } else {
            std::cout << "[ScoreRegisteredConsumer] Not all group matches completed yet. Waiting..." << std::endl;
        }
    } else if (round == "quarterfinals") {
        // Advance winners to semifinals
        std::cout << "[ScoreRegisteredConsumer] Quarterfinal match completed. Attempting to advance winners..." << std::endl;

        auto result = playoffGenerationService->AdvanceWinners(tournamentId, "quarterfinals");

        if (result.has_value()) {
            std::cout << "[ScoreRegisteredConsumer] Successfully advanced winners to semifinals" << std::endl;
        } else {
            // Not an error - might be waiting for other QF matches
            std::cout << "[ScoreRegisteredConsumer] " << result.error() << std::endl;
        }
    } else if (round == "semifinals") {
        // Advance winners to finals
        std::cout << "[ScoreRegisteredConsumer] Semifinal match completed. Attempting to advance winners..." << std::endl;

        auto result = playoffGenerationService->AdvanceWinners(tournamentId, "semifinals");

        if (result.has_value()) {
            std::cout << "[ScoreRegisteredConsumer] Successfully advanced winners to finals" << std::endl;
        } else {
            std::cout << "[ScoreRegisteredConsumer] " << result.error() << std::endl;
        }
    } else if (round == "finals") {
        std::cout << "[ScoreRegisteredConsumer] Finals match completed! Tournament is complete." << std::endl;
    } else {
        std::cerr << "[ScoreRegisteredConsumer] Unknown round type: " << round << std::endl;
    }
}

#endif /* CONSUMER_SCORE_REGISTERED_CONSUMER_HPP */
