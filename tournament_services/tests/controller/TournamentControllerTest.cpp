#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>
#include <nlohmann/json.hpp>

#include "domain/Tournament.hpp"
#include "delegate/ITournamentDelegate.hpp"
#include "controller/TournamentController.hpp"

class TournamentDelegateMock : public ITournamentDelegate {
    public:
    MOCK_METHOD((std::expected<std::string, std::string>), CreateTournament, (std::shared_ptr<domain::Tournament>), (override));
    MOCK_METHOD(std::shared_ptr<domain::Tournament>, GetTournament, (const std::string&), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Tournament>>, ReadAll, (), (override));
    MOCK_METHOD((std::expected<void, std::string>), UpdateTournament, (const domain::Tournament&), (override));
};

class TournamentControllerTest : public ::testing::Test{
protected:
    std::shared_ptr<TournamentDelegateMock> tournamentDelegateMock;
    std::shared_ptr<TournamentController> tournamentController;

    void SetUp() override {
        tournamentDelegateMock = std::make_shared<TournamentDelegateMock>();
        tournamentController = std::make_shared<TournamentController>(TournamentController(tournamentDelegateMock));
    }

    void TearDown() override {
    }
};

// Test 1: Create tournament success -> HTTP 201
TEST_F(TournamentControllerTest, CreateTournament_Success) {
    EXPECT_CALL(*tournamentDelegateMock, CreateTournament(::testing::_))
        .WillOnce(testing::Return(std::expected<std::string, std::string>("new-id")));

    nlohmann::json tournamentRequestBody = {
        {"name", "New Tournament"},
        {"format", {{"numberOfGroups", 1}, {"teamsPerGroup", 8}}}
    };
    crow::request tournamentRequest;
    tournamentRequest.body = tournamentRequestBody.dump();

    crow::response response = tournamentController->CreateTournament(tournamentRequest);

    EXPECT_EQ(crow::CREATED, response.code);
}

// Test 2: Create tournament duplicate -> HTTP 409
TEST_F(TournamentControllerTest, CreateTournament_Duplicate) {
    EXPECT_CALL(*tournamentDelegateMock, CreateTournament(::testing::_))
        .WillOnce(testing::Return(std::unexpected<std::string>("Tournament with this name already exists")));

    nlohmann::json tournamentRequestBody = {
        {"name", "Duplicate Tournament"},
        {"format", {{"numberOfGroups", 1}, {"teamsPerGroup", 8}}}
    };
    crow::request tournamentRequest;
    tournamentRequest.body = tournamentRequestBody.dump();

    crow::response response = tournamentController->CreateTournament(tournamentRequest);

    EXPECT_EQ(409, response.code);
}

// Test 3: Get tournament by ID found -> HTTP 200
TEST_F(TournamentControllerTest, GetTournamentById_Found) {
    auto expectedTournament = std::make_shared<domain::Tournament>();
    expectedTournament->Id() = "my-id";
    expectedTournament->Name() = "Tournament Name";

    EXPECT_CALL(*tournamentDelegateMock, GetTournament(testing::Eq(std::string("my-id"))))
        .WillOnce(testing::Return(expectedTournament));

    crow::response response = tournamentController->GetTournament("my-id");
    auto jsonResponse = crow::json::load(response.body);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ(expectedTournament->Id(), jsonResponse["id"]);
    EXPECT_EQ(expectedTournament->Name(), jsonResponse["name"]);
}

// Test 4: Get tournament by ID not found -> HTTP 404
TEST_F(TournamentControllerTest, GetTournamentById_NotFound) {
    EXPECT_CALL(*tournamentDelegateMock, GetTournament(testing::Eq(std::string("my-id"))))
        .WillOnce(testing::Return(nullptr));

    crow::response response = tournamentController->GetTournament("my-id");

    EXPECT_EQ(crow::NOT_FOUND, response.code);
}

// Test 5: Get all tournaments with data -> HTTP 200
TEST_F(TournamentControllerTest, GetAllTournaments_WithData) {
    std::vector<std::shared_ptr<domain::Tournament>> tournaments;

    auto tournament1 = std::make_shared<domain::Tournament>();
    tournament1->Id() = "id1";
    tournament1->Name() = "Tournament 1";

    auto tournament2 = std::make_shared<domain::Tournament>();
    tournament2->Id() = "id2";
    tournament2->Name() = "Tournament 2";

    tournaments.push_back(tournament1);
    tournaments.push_back(tournament2);

    EXPECT_CALL(*tournamentDelegateMock, ReadAll())
        .WillOnce(testing::Return(tournaments));

    crow::response response = tournamentController->ReadAll();
    auto jsonResponse = crow::json::load(response.body);

    EXPECT_EQ(200, response.code);
    EXPECT_EQ(2, jsonResponse.size());
}

// Test 6: Get all tournaments empty -> HTTP 200
TEST_F(TournamentControllerTest, GetAllTournaments_Empty) {
    std::vector<std::shared_ptr<domain::Tournament>> tournaments;

    EXPECT_CALL(*tournamentDelegateMock, ReadAll())
        .WillOnce(testing::Return(tournaments));

    crow::response response = tournamentController->ReadAll();
    auto jsonResponse = crow::json::load(response.body);

    EXPECT_EQ(200, response.code);
    EXPECT_EQ(0, jsonResponse.size());
}

// Test 7: Update tournament success -> HTTP 204
TEST_F(TournamentControllerTest, UpdateTournament_Success) {
    EXPECT_CALL(*tournamentDelegateMock, UpdateTournament(::testing::_))
        .WillOnce(testing::Return(std::expected<void, std::string>()));

    nlohmann::json tournamentRequestBody = {
        {"name", "Updated Tournament"},
        {"format", {{"numberOfGroups", 1}, {"teamsPerGroup", 8}}}
    };
    crow::request tournamentRequest;
    tournamentRequest.body = tournamentRequestBody.dump();

    crow::response response = tournamentController->UpdateTournament(tournamentRequest, "existing-id");

    EXPECT_EQ(204, response.code);
}

// Test 8: Update tournament not found -> HTTP 404
TEST_F(TournamentControllerTest, UpdateTournament_NotFound) {
    EXPECT_CALL(*tournamentDelegateMock, UpdateTournament(::testing::_))
        .WillOnce(testing::Return(std::unexpected<std::string>("Tournament not found")));

    nlohmann::json tournamentRequestBody = {
        {"name", "Updated Tournament"},
        {"format", {{"numberOfGroups", 1}, {"teamsPerGroup", 8}}}
    };
    crow::request tournamentRequest;
    tournamentRequest.body = tournamentRequestBody.dump();

    crow::response response = tournamentController->UpdateTournament(tournamentRequest, "non-existent-id");

    EXPECT_EQ(crow::NOT_FOUND, response.code);
}
