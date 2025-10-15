#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>

#include "domain/Team.hpp"
#include "delegate/ITeamDelegate.hpp"
#include "controller/TeamController.hpp"

class TeamDelegateMock : public ITeamDelegate {
    public:
    MOCK_METHOD(std::shared_ptr<domain::Team>, GetTeam, (const std::string_view id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Team>>, GetAllTeams, (), (override));
    MOCK_METHOD((std::expected<std::string, std::string>), SaveTeam, (const domain::Team&), (override));
    MOCK_METHOD((std::expected<void, std::string>), UpdateTeam, (const domain::Team&), (override));
};

class TeamControllerTest : public ::testing::Test{
protected:
    std::shared_ptr<TeamDelegateMock> teamDelegateMock;
    std::shared_ptr<TeamController> teamController;

    void SetUp() override {
        teamDelegateMock = std::make_shared<TeamDelegateMock>();
        teamController = std::make_shared<TeamController>(TeamController(teamDelegateMock));
    }

    void TearDown() override {
    }
};

// Test 1: Create team success -> HTTP 201
TEST_F(TeamControllerTest, CreateTeam_Success) {
    domain::Team capturedTeam;
    EXPECT_CALL(*teamDelegateMock, SaveTeam(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTeam),
                testing::Return(std::expected<std::string, std::string>("new-id"))
            )
        );

    nlohmann::json teamRequestBody = {{"name", "New Team"}};
    crow::request teamRequest;
    teamRequest.body = teamRequestBody.dump();

    crow::response response = teamController->SaveTeam(teamRequest);

    EXPECT_EQ(crow::CREATED, response.code);
    EXPECT_EQ("New Team", capturedTeam.Name);
}

// Test 2: Create team duplicate -> HTTP 409
TEST_F(TeamControllerTest, CreateTeam_Duplicate) {
    EXPECT_CALL(*teamDelegateMock, SaveTeam(::testing::_))
        .WillOnce(testing::Return(std::unexpected<std::string>("Team with this name already exists")));

    nlohmann::json teamRequestBody = {{"name", "Duplicate Team"}};
    crow::request teamRequest;
    teamRequest.body = teamRequestBody.dump();

    crow::response response = teamController->SaveTeam(teamRequest);

    EXPECT_EQ(409, response.code);
}

// Test 3: Get team by ID found -> HTTP 200
TEST_F(TeamControllerTest, GetTeamById_Found) {
    std::shared_ptr<domain::Team> expectedTeam = std::make_shared<domain::Team>(domain::Team{"my-id",  "Team Name"});

    EXPECT_CALL(*teamDelegateMock, GetTeam(testing::Eq(std::string("my-id"))))
        .WillOnce(testing::Return(expectedTeam));

    crow::response response = teamController->getTeam("my-id");
    auto jsonResponse = crow::json::load(response.body);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ(expectedTeam->Id, jsonResponse["id"]);
    EXPECT_EQ(expectedTeam->Name, jsonResponse["name"]);
}

// Test 4: Get team by ID not found -> HTTP 404
TEST_F(TeamControllerTest, GetTeamById_NotFound) {
    EXPECT_CALL(*teamDelegateMock, GetTeam(testing::Eq(std::string("my-id"))))
        .WillOnce(testing::Return(nullptr));

    crow::response response = teamController->getTeam("my-id");

    EXPECT_EQ(crow::NOT_FOUND, response.code);
}

// Test 5: Get all teams with data -> HTTP 200
TEST_F(TeamControllerTest, GetAllTeams_WithData) {
    std::vector<std::shared_ptr<domain::Team>> teams;
    teams.push_back(std::make_shared<domain::Team>(domain::Team{"id1", "Team 1"}));
    teams.push_back(std::make_shared<domain::Team>(domain::Team{"id2", "Team 2"}));

    EXPECT_CALL(*teamDelegateMock, GetAllTeams())
        .WillOnce(testing::Return(teams));

    crow::response response = teamController->getAllTeams();
    auto jsonResponse = crow::json::load(response.body);

    EXPECT_EQ(200, response.code);
    EXPECT_EQ(2, jsonResponse.size());
}

// Test 6: Get all teams empty -> HTTP 200
TEST_F(TeamControllerTest, GetAllTeams_Empty) {
    std::vector<std::shared_ptr<domain::Team>> teams;

    EXPECT_CALL(*teamDelegateMock, GetAllTeams())
        .WillOnce(testing::Return(teams));

    crow::response response = teamController->getAllTeams();
    auto jsonResponse = crow::json::load(response.body);

    EXPECT_EQ(200, response.code);
    EXPECT_EQ(0, jsonResponse.size());
}

// Test 7: Update team success -> HTTP 204
TEST_F(TeamControllerTest, UpdateTeam_Success) {
    domain::Team capturedTeam;
    EXPECT_CALL(*teamDelegateMock, UpdateTeam(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTeam),
                testing::Return(std::expected<void, std::string>())
            )
        );

    nlohmann::json teamRequestBody = {{"name", "Updated Team"}};
    crow::request teamRequest;
    teamRequest.body = teamRequestBody.dump();

    crow::response response = teamController->UpdateTeam(teamRequest, "existing-id");

    EXPECT_EQ(204, response.code);
    EXPECT_EQ("existing-id", capturedTeam.Id);
    EXPECT_EQ("Updated Team", capturedTeam.Name);
}

// Test 8: Update team not found -> HTTP 404
TEST_F(TeamControllerTest, UpdateTeam_NotFound) {
    EXPECT_CALL(*teamDelegateMock, UpdateTeam(::testing::_))
        .WillOnce(testing::Return(std::unexpected<std::string>("Team not found")));

    nlohmann::json teamRequestBody = {{"name", "Updated Team"}};
    crow::request teamRequest;
    teamRequest.body = teamRequestBody.dump();

    crow::response response = teamController->UpdateTeam(teamRequest, "non-existent-id");

    EXPECT_EQ(crow::NOT_FOUND, response.code);
}
