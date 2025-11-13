#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include "controller/MatchController.hpp"
#include "delegate/IMatchDelegate.hpp"
#include "domain/Match.hpp"

// Mock MatchDelegate
class MatchDelegateMock : public IMatchDelegate {
public:
    MOCK_METHOD((std::expected<std::vector<domain::Match>, std::string>), GetMatches,
                (const std::string_view&, const std::optional<std::string>&), (override));

    MOCK_METHOD((std::expected<domain::Match, std::string>), GetMatch,
                (const std::string_view&, const std::string_view&), (override));

    MOCK_METHOD((std::expected<void, std::string>), UpdateScore,
                (const std::string_view&, const std::string_view&, const domain::Score&), (override));
};

class MatchControllerTest : public ::testing::Test {
protected:
    std::shared_ptr<MatchDelegateMock> matchDelegateMock;
    std::shared_ptr<MatchController> matchController;

    void SetUp() override {
        matchDelegateMock = std::make_shared<MatchDelegateMock>();
        matchController = std::make_shared<MatchController>(matchDelegateMock);
    }

    domain::Match CreateTestMatch(const std::string& id) {
        domain::Match match;
        match.Id() = id;
        match.TournamentId() = "tournament-123";
        match.GroupId() = "group-456";
        match.Home() = domain::MatchTeam("team1", "Team One");
        match.Visitor() = domain::MatchTeam("team2", "Team Two");
        match.Round() = "regular";
        match.Status() = "pending";
        return match;
    }
};

// Test 1: GET all matches - success
TEST_F(MatchControllerTest, GetMatches_Success_ReturnsMatches) {
    // Arrange
    std::vector<domain::Match> matches;
    matches.push_back(CreateTestMatch("match1"));
    matches.push_back(CreateTestMatch("match2"));

    EXPECT_CALL(*matchDelegateMock, GetMatches("tournament-123", testing::Eq(std::nullopt)))
        .WillOnce(testing::Return(matches));

    crow::request request;
    request.url = "/tournaments/tournament-123/matches";

    // Act
    auto response = matchController->GetMatches(request, "tournament-123");

    // Assert
    EXPECT_EQ(crow::OK, response.code);

    // Debug output to see actual response
    if (response.body.find("match1") == std::string::npos) {
        std::cout << "Response body: " << response.body << std::endl;
    }

    EXPECT_NE(response.body.find("match1"), std::string::npos) << "Response: " << response.body;
    EXPECT_NE(response.body.find("match2"), std::string::npos) << "Response: " << response.body;
}

// Test 2: GET matches filtered by "played" - success
TEST_F(MatchControllerTest, GetMatches_FilteredByPlayed_ReturnsPlayedMatches) {
    // Arrange
    domain::Match playedMatch = CreateTestMatch("match1");
    playedMatch.SetScore(domain::Score(2, 1));

    std::vector<domain::Match> matches;
    matches.push_back(playedMatch);

    EXPECT_CALL(*matchDelegateMock, GetMatches(testing::_, testing::Eq(std::optional<std::string>("played"))))
        .WillOnce(testing::Return(matches));

    crow::request request;
    request.url = "/tournaments/tournament-123/matches?showMatches=played";
    request.url_params = crow::query_string("?showMatches=played");

    // Act
    auto response = matchController->GetMatches(request, "tournament-123");

    // Assert
    EXPECT_EQ(crow::OK, response.code);
    EXPECT_NE(response.body.find("played"), std::string::npos);
}

// Test 3: GET matches filtered by "pending" - success
TEST_F(MatchControllerTest, GetMatches_FilteredByPending_ReturnsPendingMatches) {
    // Arrange
    std::vector<domain::Match> matches;
    matches.push_back(CreateTestMatch("match1"));

    EXPECT_CALL(*matchDelegateMock, GetMatches(testing::_, testing::Eq(std::optional<std::string>("pending"))))
        .WillOnce(testing::Return(matches));

    crow::request request;
    request.url = "/tournaments/tournament-123/matches?showMatches=pending";
    request.url_params = crow::query_string("?showMatches=pending");

    // Act
    auto response = matchController->GetMatches(request, "tournament-123");

    // Assert
    EXPECT_EQ(crow::OK, response.code);
    EXPECT_NE(response.body.find("pending"), std::string::npos);
}

// Test 4: GET matches with invalid filter - returns bad request
TEST_F(MatchControllerTest, GetMatches_InvalidFilter_ReturnsBadRequest) {
    // Arrange
    crow::request request;
    request.url = "/tournaments/tournament-123/matches?showMatches=invalid";
    request.url_params = crow::query_string("?showMatches=invalid");

    // Act
    auto response = matchController->GetMatches(request, "tournament-123");

    // Assert
    EXPECT_EQ(crow::BAD_REQUEST, response.code);
    EXPECT_NE(response.body.find("Invalid showMatches parameter"), std::string::npos);
}

// Test 5: GET matches - tournament not found
TEST_F(MatchControllerTest, GetMatches_TournamentNotFound_ReturnsNotFound) {
    // Arrange
    EXPECT_CALL(*matchDelegateMock, GetMatches(testing::_, testing::_))
        .WillOnce(testing::Return(std::unexpected("Tournament doesn't exist")));

    crow::request request;
    request.url = "/tournaments/nonexistent/matches";

    // Act
    auto response = matchController->GetMatches(request, "nonexistent");

    // Assert
    EXPECT_EQ(crow::NOT_FOUND, response.code);
}

// Test 6: GET matches - empty tournament returns empty array
TEST_F(MatchControllerTest, GetMatches_EmptyTournament_ReturnsEmptyArray) {
    // Arrange
    std::vector<domain::Match> emptyMatches;

    EXPECT_CALL(*matchDelegateMock, GetMatches(testing::_, testing::_))
        .WillOnce(testing::Return(emptyMatches));

    crow::request request;
    request.url = "/tournaments/tournament-123/matches";

    // Act
    auto response = matchController->GetMatches(request, "tournament-123");

    // Assert
    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ("[]", response.body);
}

// Test 7: GET single match - success
TEST_F(MatchControllerTest, GetMatch_Success_ReturnsMatch) {
    // Arrange
    domain::Match match = CreateTestMatch("match1");

    EXPECT_CALL(*matchDelegateMock, GetMatch("tournament-123", "match1"))
        .WillOnce(testing::Return(match));

    crow::request request;
    request.url = "/tournaments/tournament-123/matches/match1";

    // Act
    auto response = matchController->GetMatch(request, "tournament-123", "match1");

    // Assert
    EXPECT_EQ(crow::OK, response.code);
    EXPECT_NE(response.body.find("match1"), std::string::npos);
    EXPECT_NE(response.body.find("Team One"), std::string::npos);
}

// Test 8: GET single match - match not found
TEST_F(MatchControllerTest, GetMatch_NotFound_ReturnsNotFound) {
    // Arrange
    EXPECT_CALL(*matchDelegateMock, GetMatch("tournament-123", "nonexistent"))
        .WillOnce(testing::Return(std::unexpected("Match doesn't exist")));

    crow::request request;
    request.url = "/tournaments/tournament-123/matches/nonexistent";

    // Act
    auto response = matchController->GetMatch(request, "tournament-123", "nonexistent");

    // Assert
    EXPECT_EQ(crow::NOT_FOUND, response.code);
}

// Test 9: PATCH update score - success
TEST_F(MatchControllerTest, UpdateScore_Success_ReturnsNoContent) {
    // Arrange
    EXPECT_CALL(*matchDelegateMock, UpdateScore("tournament-123", "match1", testing::_))
        .WillOnce(testing::Invoke([](const std::string_view& tournamentId, const std::string_view& matchId, const domain::Score& score) {
            // Validate the score object
            EXPECT_EQ(score.home, 2);
            EXPECT_EQ(score.visitor, 1);
            return std::expected<void, std::string>();
        }));

    crow::request request;
    request.url = "/tournaments/tournament-123/matches/match1";
    request.body = R"({"score": {"home": 2, "visitor": 1}})";

    // Act
    auto response = matchController->UpdateScore(request, "tournament-123", "match1");

    // Assert
    EXPECT_EQ(crow::NO_CONTENT, response.code);
}

// Test 10: PATCH update score - invalid JSON
TEST_F(MatchControllerTest, UpdateScore_InvalidJson_ReturnsBadRequest) {
    // Arrange
    crow::request request;
    request.url = "/tournaments/tournament-123/matches/match1";
    request.body = "invalid json";

    // Act
    auto response = matchController->UpdateScore(request, "tournament-123", "match1");

    // Assert
    EXPECT_EQ(crow::BAD_REQUEST, response.code);
    EXPECT_NE(response.body.find("Invalid JSON"), std::string::npos);
}

// Test 11: PATCH update score - missing fields
TEST_F(MatchControllerTest, UpdateScore_MissingFields_ReturnsBadRequest) {
    // Arrange
    crow::request request;
    request.url = "/tournaments/tournament-123/matches/match1";
    request.body = R"({"score": {"home": 2}})";  // Missing visitor

    // Act
    auto response = matchController->UpdateScore(request, "tournament-123", "match1");

    // Assert
    EXPECT_EQ(crow::BAD_REQUEST, response.code);
    EXPECT_NE(response.body.find("Missing required fields"), std::string::npos);
}

// Test 12: PATCH update score - tie not allowed (returns 422/500)
TEST_F(MatchControllerTest, UpdateScore_TieNotAllowed_ReturnsUnprocessableEntity) {
    // Arrange
    EXPECT_CALL(*matchDelegateMock, UpdateScore(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(std::unexpected("Ties are not allowed")));

    crow::request request;
    request.url = "/tournaments/tournament-123/matches/match1";
    request.body = R"({"score": {"home": 1, "visitor": 1}})";

    // Act
    auto response = matchController->UpdateScore(request, "tournament-123", "match1");

    // Assert
    // HTTP 422 will return as 500 due to Crow limitation
    EXPECT_EQ(422, response.code);
    EXPECT_NE(response.body.find("not allowed"), std::string::npos);
}

// Test 13: PATCH update score - match not found
TEST_F(MatchControllerTest, UpdateScore_MatchNotFound_ReturnsNotFound) {
    // Arrange
    EXPECT_CALL(*matchDelegateMock, UpdateScore(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(std::unexpected("Match doesn't exist")));

    crow::request request;
    request.url = "/tournaments/tournament-123/matches/nonexistent";
    request.body = R"({"score": {"home": 2, "visitor": 1}})";

    // Act
    auto response = matchController->UpdateScore(request, "tournament-123", "nonexistent");

    // Assert
    EXPECT_EQ(crow::NOT_FOUND, response.code);
}

// Test 14: PATCH update score - negative score
TEST_F(MatchControllerTest, UpdateScore_NegativeScore_ReturnsUnprocessableEntity) {
    // Arrange
    EXPECT_CALL(*matchDelegateMock, UpdateScore(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(std::unexpected("Score cannot be negative")));

    crow::request request;
    request.url = "/tournaments/tournament-123/matches/match1";
    request.body = R"({"score": {"home": -1, "visitor": 2}})";

    // Act
    auto response = matchController->UpdateScore(request, "tournament-123", "match1");

    // Assert
    EXPECT_EQ(422, response.code);
    EXPECT_NE(response.body.find("cannot be negative"), std::string::npos);
}

// Test 15: PATCH update score - invalid score format
TEST_F(MatchControllerTest, UpdateScore_InvalidScoreFormat_ReturnsBadRequest) {
    // Arrange
    crow::request request;
    request.url = "/tournaments/tournament-123/matches/match1";
    request.body = R"({"score": {"home": "two", "visitor": 1}})";  // String instead of int

    // Act
    auto response = matchController->UpdateScore(request, "tournament-123", "match1");

    // Assert
    EXPECT_EQ(crow::BAD_REQUEST, response.code);
    EXPECT_NE(response.body.find("Invalid score format"), std::string::npos);
}
