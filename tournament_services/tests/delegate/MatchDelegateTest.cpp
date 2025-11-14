#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

#include "domain/Match.hpp"
#include "domain/Tournament.hpp"
#include "persistence/repository/MatchRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"
#include "cms/IQueueMessageProducer.hpp"
#include "delegate/MatchDelegate.hpp"

// Mock MatchRepository
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

// Mock TournamentRepository
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

// Mock IQueueMessageProducer
class QueueMessageProducerMock : public IQueueMessageProducer {
public:
    MOCK_METHOD(void, SendMessage, (const std::string_view& message, const std::string_view& queue), (override));
};

class MatchDelegateTest : public ::testing::Test {
protected:
    std::shared_ptr<MatchRepositoryMock> matchRepositoryMock;
    std::shared_ptr<TournamentRepositoryMock> tournamentRepositoryMock;
    std::shared_ptr<QueueMessageProducerMock> messageProducerMock;
    std::shared_ptr<MatchDelegate> matchDelegate;

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

    std::shared_ptr<domain::Tournament> CreateTestTournament(const std::string& id) {
        auto tournament = std::make_shared<domain::Tournament>();
        tournament->Id() = id;
        tournament->Name() = "Test Tournament";
        tournament->Format().Type() = domain::TournamentType::ROUND_ROBIN;
        tournament->Format().MaxTeamsPerGroup() = 4;
        return tournament;
    }

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

// Test 1: GetMatches returns all matches when no filter is applied
TEST_F(MatchDelegateTest, GetMatches_NoFilter_ReturnsAllMatches) {
    // Arrange
    std::string tournamentId = "tournament-123";
    auto tournament = CreateTestTournament(tournamentId);
    auto match1 = CreateTestMatch("match1", tournamentId);
    auto match2 = CreateTestMatch("match2", tournamentId);

    std::vector<std::shared_ptr<domain::Match>> matches = {match1, match2};

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(tournamentId))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*matchRepositoryMock, FindByTournamentId(tournamentId))
        .WillOnce(testing::Return(matches));

    // Act
    auto result = matchDelegate->GetMatches(tournamentId, std::nullopt);

    // Assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(2, result.value().size());
    EXPECT_EQ("match1", result.value()[0].Id());
    EXPECT_EQ("match2", result.value()[1].Id());
}

// Test 2: GetMatches filters by "played" status
TEST_F(MatchDelegateTest, GetMatches_FilteredByPlayed_ReturnsPlayedMatches) {
    // Arrange
    std::string tournamentId = "tournament-123";
    auto tournament = CreateTestTournament(tournamentId);
    auto match1 = CreateTestMatch("match1", tournamentId);
    match1->SetScore(domain::Score(2, 1));

    std::vector<std::shared_ptr<domain::Match>> playedMatches = {match1};

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(tournamentId))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*matchRepositoryMock, FindByTournamentIdAndStatus(tournamentId, "played"))
        .WillOnce(testing::Return(playedMatches));

    // Act
    auto result = matchDelegate->GetMatches(tournamentId, std::optional<std::string>("played"));

    // Assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(1, result.value().size());
    EXPECT_EQ("played", result.value()[0].Status());
}

// Test 3: GetMatches filters by "pending" status
TEST_F(MatchDelegateTest, GetMatches_FilteredByPending_ReturnsPendingMatches) {
    // Arrange
    std::string tournamentId = "tournament-123";
    auto tournament = CreateTestTournament(tournamentId);
    auto match1 = CreateTestMatch("match1", tournamentId);

    std::vector<std::shared_ptr<domain::Match>> pendingMatches = {match1};

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(tournamentId))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*matchRepositoryMock, FindByTournamentIdAndStatus(tournamentId, "pending"))
        .WillOnce(testing::Return(pendingMatches));

    // Act
    auto result = matchDelegate->GetMatches(tournamentId, std::optional<std::string>("pending"));

    // Assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(1, result.value().size());
    EXPECT_EQ("pending", result.value()[0].Status());
}

// Test 4: GetMatches returns error when tournament doesn't exist
TEST_F(MatchDelegateTest, GetMatches_TournamentNotFound_ReturnsError) {
    // Arrange
    std::string tournamentId = "nonexistent";

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(tournamentId))
        .WillOnce(testing::Return(nullptr));

    // Act
    auto result = matchDelegate->GetMatches(tournamentId, std::nullopt);

    // Assert
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ("Tournament doesn't exist", result.error());
}

// Test 5: GetMatch returns single match successfully
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

// Test 6: GetMatch returns error when match not found
TEST_F(MatchDelegateTest, GetMatch_MatchNotFound_ReturnsError) {
    // Arrange
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

// Test 7: GetMatch returns error when match doesn't belong to tournament
TEST_F(MatchDelegateTest, GetMatch_WrongTournament_ReturnsError) {
    // Arrange
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

// Test 8: UpdateScore updates match successfully and publishes event
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
        .WillOnce(testing::Invoke([&matchId](const domain::Match& updatedMatch) {
            // Validate the match was updated correctly
            EXPECT_EQ(updatedMatch.Status(), "played");
            EXPECT_TRUE(updatedMatch.GetScore().has_value());
            EXPECT_EQ(updatedMatch.GetScore().value().home, 2);
            EXPECT_EQ(updatedMatch.GetScore().value().visitor, 1);
            return matchId;
        }));
    EXPECT_CALL(*messageProducerMock, SendMessage(testing::_, "tournament.score-registered"))
        .WillOnce(testing::Invoke([&tournamentId, &matchId](const std::string_view& message, const std::string_view& queue) {
            // Validate the event payload contains all required fields
            auto json = nlohmann::json::parse(message);
            EXPECT_EQ(json["tournamentId"], tournamentId);
            EXPECT_EQ(json["matchId"], matchId);
            EXPECT_EQ(json["round"], "regular");
            EXPECT_EQ(json["score"]["home"], 2);
            EXPECT_EQ(json["score"]["visitor"], 1);
            EXPECT_TRUE(json.contains("winnerId"));
            EXPECT_EQ(json["winnerId"], "team1");  // team1 won 2-1
        }));

    // Act
    auto result = matchDelegate->UpdateScore(tournamentId, matchId, score);

    // Assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ("played", match->Status());
    EXPECT_EQ(2, match->GetScore().value().home);
    EXPECT_EQ(1, match->GetScore().value().visitor);
}

// Test 9: UpdateScore rejects ties in playoff matches but allows in regular season
TEST_F(MatchDelegateTest, UpdateScore_TieScore_ReturnsError) {
    // Arrange - playoff match
    std::string tournamentId = "tournament-123";
    std::string matchId = "match1";
    domain::Score tieScore(1, 1);

    auto tournament = CreateTestTournament(tournamentId);
    auto match = CreateTestMatch(matchId, tournamentId);
    match->Round() = "quarterfinals";  // Playoff match

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(tournamentId))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*matchRepositoryMock, ReadById(matchId))
        .WillOnce(testing::Return(match));

    // Act
    auto result = matchDelegate->UpdateScore(tournamentId, matchId, tieScore);

    // Assert
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ("Ties are not allowed in playoff rounds", result.error());
}

// Test 10: UpdateScore rejects negative scores
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

// Test 11: UpdateScore allows ties in regular season matches
TEST_F(MatchDelegateTest, UpdateScore_TieInRegularSeason_Succeeds) {
    // Arrange
    std::string tournamentId = "tournament-123";
    std::string matchId = "match1";
    domain::Score tieScore(2, 2);

    auto tournament = CreateTestTournament(tournamentId);
    auto match = CreateTestMatch(matchId, tournamentId);  // Regular round match

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(tournamentId))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*matchRepositoryMock, ReadById(matchId))
        .WillOnce(testing::Return(match));
    EXPECT_CALL(*matchRepositoryMock, Update(testing::_))
        .WillOnce(testing::Return("match1"));
    EXPECT_CALL(*messageProducerMock, SendMessage(testing::_, "tournament.score-registered"))
        .Times(1);

    // Act
    auto result = matchDelegate->UpdateScore(tournamentId, matchId, tieScore);

    // Assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ("played", match->Status());
    EXPECT_EQ(2, match->GetScore().value().home);
    EXPECT_EQ(2, match->GetScore().value().visitor);
}

// Test 12: UpdateScore rejects scores greater than 10
TEST_F(MatchDelegateTest, UpdateScore_ScoreGreaterThan10_ReturnsError) {
    // Arrange
    std::string tournamentId = "tournament-123";
    std::string matchId = "match1";
    domain::Score invalidScore(11, 5);

    auto tournament = CreateTestTournament(tournamentId);
    auto match = CreateTestMatch(matchId, tournamentId);

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(tournamentId))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*matchRepositoryMock, ReadById(matchId))
        .WillOnce(testing::Return(match));

    // Act
    auto result = matchDelegate->UpdateScore(tournamentId, matchId, invalidScore);

    // Assert
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ("Score must be between 0 and 10", result.error());
}

// Test 13: UpdateScore returns error when tournament doesn't exist
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

// Test 12: UpdateScore returns error when match doesn't exist
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
