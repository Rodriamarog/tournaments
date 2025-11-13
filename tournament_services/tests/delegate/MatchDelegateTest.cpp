// This test suite uses GoogleTest + GoogleMock to validate the behavior of MatchDelegate.
// We mock the repositories and the message producer to isolate business logic and avoid touching real databases or queues.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

#include "domain/Match.hpp"
#include "domain/Tournament.hpp"
#include "persistence/repository/MatchRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"
#include "cms/IQueueMessageProducer.hpp"
#include "delegate/MatchDelegate.hpp"

// ---------------------------
// Mock MatchRepository
// ---------------------------
// These mocks simulate the data layer. By replacing real repositories,
// we can control the behavior of each method during the tests.
// This helps us test MatchDelegate without depending on external storage.
class MatchRepositoryMock : public MatchRepository {
public:
    explicit MatchRepositoryMock() : MatchRepository(nullptr) {}

    MOCK_METHOD(std::shared_ptr<domain::Match>, ReadById, (std::string id), (override));
    MOCK_METHOD(std::string, Create, (const domain::Match& entity), (override));
    MOCK_METHOD(std::string, Update, (const domain::Match& entity), (override));
    MOCK_METHOD(void, Delete, (std::string id), (override));
    MOCK_METHOD((std::vector<std::shared_ptr<domain::Match>>), ReadAll, (), (override));
    MOCK_METHOD((std::vector<std::shared_ptr<domain::Match>>), FindByTournamentId, (const std::string_view&), (override));
    MOCK_METHOD((std::vector<std::shared_ptr<domain::Match>>), FindByTournamentIdAndStatus, (const std::string_view&, const std::string&), (override));
    MOCK_METHOD((std::vector<std::shared_ptr<domain::Match>>), FindByGroupId, (const std::string_view&), (override));
    MOCK_METHOD((std::vector<std::shared_ptr<domain::Match>>), FindByTournamentIdAndRound, (const std::string_view&, const std::string&), (override));
    MOCK_METHOD(bool, ExistsByGroupId, (const std::string_view&), (override));
};

// ---------------------------
// Mock TournamentRepository
// ---------------------------
// Same purpose as above: we fake repository behavior so we can test logic
// without reaching a database.
class TournamentRepositoryMock : public TournamentRepository {
public:
    explicit TournamentRepositoryMock() : TournamentRepository(nullptr) {}

    MOCK_METHOD(std::shared_ptr<domain::Tournament>, ReadById, (std::string id), (override));
    MOCK_METHOD(std::string, Create, (const domain::Tournament& entity), (override));
    MOCK_METHOD(std::string, Update, (const domain::Tournament& entity), (override));
    MOCK_METHOD(void, Delete, (std::string id), (override));
    MOCK_METHOD((std::vector<std::shared_ptr<domain::Tournament>>), ReadAll, (), (override));
    MOCK_METHOD(bool, ExistsByName, (const std::string& name), (override));
};

// ---------------------------
// Mock Queue Producer
// ---------------------------
// Simulates the message broker used to notify other services.
// By mocking this, we confirm that the event is sent at the right moment.
class QueueMessageProducerMock : public IQueueMessageProducer {
public:
    MOCK_METHOD(void, SendMessage, (const std::string_view& message, const std::string_view& queue), (override));
};

// ---------------------------
// Test Fixture
// ---------------------------
// This class prepares a clean environment for each test run.
class MatchDelegateTest : public ::testing::Test {
protected:
    std::shared_ptr<MatchRepositoryMock> matchRepositoryMock;
    std::shared_ptr<TournamentRepositoryMock> tournamentRepositoryMock;
    std::shared_ptr<QueueMessageProducerMock> messageProducerMock;
    std::shared_ptr<MatchDelegate> matchDelegate;

    // This method runs before each test case.
    // It initializes the mocks and injects them into MatchDelegate.
    void SetUp() override {
        matchRepositoryMock = std::make_shared<MatchRepositoryMock>();
        tournamentRepositoryMock = std::make_shared<TournamentRepositoryMock>();
        messageProducerMock = std::make_shared<QueueMessageProducerMock>();
        matchDelegate = std::make_shared<MatchDelegate>(
            matchRepositoryMock,
            tournamentRepositoryMock,
            messageProducerMock
        );
    }

    // Helper function to create a fake tournament used by several tests.
    std::shared_ptr<domain::Tournament> CreateTestTournament(const std::string& id) {
        auto tournament = std::make_shared<domain::Tournament>();
        tournament->Id() = id;
        tournament->Name() = "Test Tournament";
        tournament->Format().Type() = domain::TournamentType::ROUND_ROBIN;
        tournament->Format().MaxTeamsPerGroup() = 4;
        return tournament;
    }

    // Helper function to create a fake match connected to a tournament.
    std::shared_ptr<domain::Match> CreateTestMatch(const std::string& id, const std::string& tournamentId) {
        auto match = std::make_shared<domain::Match>();
        match->Id() = id;
        match->TournamentId() = tournamentId;
        match->GroupId() = "group-1";
        match->Home() = domain::MatchTeam("team1", "Team One");
        match->Visitor() = domain::MatchTeam("team2", "Team Two");
        match->Round() = "regular";
        match->Status() = "pending";
        return match;
    }
};

// ---------------------------
// Test 1
// ---------------------------
// Validates that GetMatches returns all matches when no filter is provided.
TEST_F(MatchDelegateTest, GetMatches_NoFilter_ReturnsAllMatches) {
    // Arrange: fake tournament + two matches
    std::string tournamentId = "tournament-123";
    auto tournament = CreateTestTournament(tournamentId);
    auto match1 = CreateTestMatch("match1", tournamentId);
    auto match2 = CreateTestMatch("match2", tournamentId);

    std::vector<std::shared_ptr<domain::Match>> matches = {match1, match2};

    // Mock expectations: repository returns tournament and matches list
    EXPECT_CALL(*tournamentRepositoryMock, ReadById(tournamentId))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*matchRepositoryMock, FindByTournamentId(testing::_))
        .WillOnce(testing::Return(matches));

    // Act: call the method we want to verify
    auto result = matchDelegate->GetMatches(tournamentId, std::nullopt);

    // Assert: confirm behavior and returned data
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(2, result.value().size());
    EXPECT_EQ("match1", result.value()[0].Id());
    EXPECT_EQ("match2", result.value()[1].Id());
}

// ---------------------------
// Test 2
// ---------------------------
// Checks that only "played" matches are returned when using a filter.
TEST_F(MatchDelegateTest, GetMatches_FilteredByPlayed_ReturnsPlayedMatches) {
    // Arrange
    std::string tournamentId = "tournament-123";
    auto tournament = CreateTestTournament(tournamentId);
    auto match1 = CreateTestMatch("match1", tournamentId);
    match1->SetScore(domain::Score(2, 1));  // Score sets status to "played"

    std::vector<std::shared_ptr<domain::Match>> playedMatches = {match1};

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(tournamentId))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*matchRepositoryMock, FindByTournamentIdAndStatus(testing::_, "played"))
        .WillOnce(testing::Return(playedMatches));

    // Act
    auto result = matchDelegate->GetMatches(tournamentId, std::optional<std::string>("played"));

    // Assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(1, result.value().size());
    EXPECT_EQ("played", result.value()[0].Status());
}

// ---------------------------
// Test 3
// ---------------------------
// Same as above but for "pending" matches.
TEST_F(MatchDelegateTest, GetMatches_FilteredByPending_ReturnsPendingMatches) {
    // Arrange
    std::string tournamentId = "tournament-123";
    auto tournament = CreateTestTournament(tournamentId);
    auto match1 = CreateTestMatch("match1", tournamentId);

    std::vector<std::shared_ptr<domain::Match>> pendingMatches = {match1};

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(tournamentId))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*matchRepositoryMock, FindByTournamentIdAndStatus(testing::_, "pending"))
        .WillOnce(testing::Return(pendingMatches));

    // Act
    auto result = matchDelegate->GetMatches(tournamentId, std::optional<std::string>("pending"));

    // Assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(1, result.value().size());
    EXPECT_EQ("pending", result.value()[0].Status());
}

// ---------------------------
// Test 4
// ---------------------------
// Ensures an error is returned when the tournament doesn't exist.
TEST_F(MatchDelegateTest, GetMatches_TournamentNotFound_ReturnsError) {
    // Arrange: repository returns null instead of a valid tournament
    std::string tournamentId = "nonexistent";

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(tournamentId))
        .WillOnce(testing::Return(nullptr));

    // Act
    auto result = matchDelegate->GetMatches(tournamentId, std::nullopt);

    // Assert
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ("Tournament doesn't exist", result.error());
}

// ---------------------------
// Test 5
// ---------------------------
// Confirms that GetMatch returns a valid match when everything is correct.
TEST_F(MatchDelegateTest, GetMatch_Success_ReturnsMatch) {
    // Arrange
    std::string tournamentId = "tournament-123";
    std::string matchId = "match1";
    auto tournament = CreateTestTournament(tournamentId);
    auto match = CreateTestMatch(matchId, tournamentId);

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(tournamentId))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*matchRepositoryMock, ReadById(matchId))
        .WillOnce(testing::Return(match));

    // Act
    auto result = matchDelegate->GetMatch(tournamentId, matchId);

    // Assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(matchId, result.value().Id());
    EXPECT_EQ(tournamentId, result.value().TournamentId());
}

// ---------------------------
// Test 6
// ---------------------------
// Returns an error when the match ID doesn't exist.
TEST_F(MatchDelegateTest, GetMatch_MatchNotFound_ReturnsError) {
    // Arrange: match not found
    std::string tournamentId = "tournament-123";
    std::string matchId = "nonexistent";
    auto tournament = CreateTestTournament(tournamentId);

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(tournamentId))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*matchRepositoryMock, ReadById(matchId))
        .WillOnce(testing::Return(nullptr));

    // Act
    auto result = matchDelegate->GetMatch(tournamentId, matchId);

    // Assert
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ("Match doesn't exist", result.error());
}

// ---------------------------
// Test 7
// ---------------------------
// Validates that a match cannot belong to a different tournament.
TEST_F(MatchDelegateTest, GetMatch_WrongTournament_ReturnsError) {
    // Arrange: match belongs to another tournament
    std::string tournamentId = "tournament-123";
    std::string matchId = "match1";
    auto tournament = CreateTestTournament(tournamentId);
    auto match = CreateTestMatch(matchId, "different-tournament");

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(tournamentId))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*matchRepositoryMock, ReadById(matchId))
        .WillOnce(testing::Return(match));

    // Act
    auto result = matchDelegate->GetMatch(tournamentId, matchId);

    // Assert
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ("Match doesn't belong to this tournament", result.error());
}

// ---------------------------
// Test 8
// ---------------------------
// This test validates the complete score update flow:
// 1. Match is found
// 2. Tournament exists
// 3. Score is valid
// 4. Status becomes "played"
// 5. Repository updates the match
// 6. A message is published
TEST_F(MatchDelegateTest, UpdateScore_Success_UpdatesAndPublishesEvent) {
    // Arrange
    std::string tournamentId = "tournament-123";
    std::string matchId = "match1";
    domain::Score score(2, 1);

    auto tournament = CreateTestTournament(tournamentId);
    auto match = CreateTestMatch(matchId, tournamentId);

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(tournamentId))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*matchRepositoryMock, ReadById(matchId))
        .WillOnce(testing::Return(match));
    EXPECT_CALL(*matchRepositoryMock, Update(testing::_))
        .WillOnce(testing::Return(matchId));

    // .Times(1) ensures the event is published once—important for avoiding duplicate messages.
    EXPECT_CALL(*messageProducerMock, SendMessage(testing::_, "tournament.score-registered"))
        .Times(1);

    // Act
    auto result = matchDelegate->UpdateScore(tournamentId, matchId, score);

    // Assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ("played", match->Status());
    EXPECT_EQ(2, match->GetScore().value().home);
    EXPECT_EQ(1, match->GetScore().value().visitor);
}

// ---------------------------
// Test 9
// ---------------------------
// Ensures ties are not accepted by business rules.
TEST_F(MatchDelegateTest, UpdateScore_TieScore_ReturnsError) {
    // Arrange
    std::string tournamentId = "tournament-123";
    std::string matchId = "match1";
    domain::Score tieScore(1, 1);

    auto tournament = CreateTestTournament(tournamentId);
    auto match = CreateTestMatch(matchId, tournamentId);

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(tournamentId))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*matchRepositoryMock, ReadById(matchId))
        .WillOnce(testing::Return(match));

    // Act
    auto result = matchDelegate->UpdateScore(tournamentId, matchId, tieScore);

    // Assert
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ("Ties are not allowed", result.error());
}

// ---------------------------
// Test 10
// ---------------------------
// Validates that scores cannot contain negative numbers.
TEST_F(MatchDelegateTest, UpdateScore_NegativeScore_ReturnsError) {
    // Arrange
    std::string tournamentId = "tournament-123";
    std::string matchId = "match1";
    domain::Score negativeScore(-1, 2);

    auto tournament = CreateTestTournament(tournamentId);
    auto match = CreateTestMatch(matchId, tournamentId);

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(tournamentId))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*matchRepositoryMock, ReadById(matchId))
        .WillOnce(testing::Return(match));

    // Act
    auto result = matchDelegate->UpdateScore(tournamentId, matchId, negativeScore);

    // Assert
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ("Score cannot be negative", result.error());
}

// ---------------------------
// Test 11
// ---------------------------
// Error returned when tournament doesn't exist.
TEST_F(MatchDelegateTest, UpdateScore_TournamentNotFound_ReturnsError) {
    // Arrange
    std::string tournamentId = "nonexistent";
    std::string matchId = "match1";
    domain::Score score(2, 1);

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(tournamentId))
        .WillOnce(testing::Return(nullptr));

    // Act
    auto result = matchDelegate->UpdateScore(tournamentId, matchId, score);

    // Assert
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ("Tournament doesn't exist", result.error());
}

// ---------------------------
// Test 12
// ---------------------------
// Confirms that the match must exist before updating its score.
TEST_F(MatchDelegateTest, UpdateScore_MatchNotFound_ReturnsError) {
    // Arrange
    std::string tournamentId = "tournament-123";
    std::string matchId = "nonexistent";
    domain::Score score(2, 1);

    auto tournament = CreateTestTournament(tournamentId);

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(tournamentId))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*matchRepositoryMock, ReadById(matchId))
        .WillOnce(testing::Return(nullptr));

    // Act
    auto result = matchDelegate->UpdateScore(tournamentId, matchId, score);

    // Assert
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ("Match doesn't exist", result.error());
}
