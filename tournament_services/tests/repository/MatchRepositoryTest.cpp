#include <gtest/gtest.h>
#include <memory>
#include <fstream>
#include <ctime>
#include <nlohmann/json.hpp>
#include "persistence/repository/MatchRepository.hpp"
#include "persistence/configuration/PostgresConnectionProvider.hpp"
#include "domain/Match.hpp"

// Note: These are integration tests that require a real database connection
// They read the connection string from configuration.json

class MatchRepositoryTest : public ::testing::Test {
protected:
    std::shared_ptr<PostgresConnectionProvider> connectionProvider;
    std::shared_ptr<MatchRepository> repository;

    void SetUp() override {
        // Read connection string from configuration file
        // Try multiple possible paths
        std::ifstream configFile("../../tournament_services/configuration.json");
        if (!configFile.is_open()) {
            configFile.open("../configuration.json");
        }
        if (!configFile.is_open()) {
            configFile.open("configuration.json");
        }
        if (!configFile.is_open()) {
            GTEST_SKIP() << "Configuration file not found. Skipping integration tests.";
            return;
        }

        nlohmann::json config;
        configFile >> config;
        std::string connectionString = config["databaseConfig"]["connectionString"];
        int poolSize = config["databaseConfig"]["poolSize"];

        connectionProvider = std::make_shared<PostgresConnectionProvider>(connectionString, poolSize);
        repository = std::make_shared<MatchRepository>(connectionProvider);
    }

    void TearDown() override {
        // Clean up test data
        if (repository && !testMatchIds.empty()) {
            for (const auto& matchId : testMatchIds) {
                try {
                    repository->Delete(matchId);
                } catch (...) {
                    // Ignore cleanup errors
                }
            }
        }
    }

    std::string CreateTestTournament() {
        // Create a test tournament with a unique name and return its ID
        auto pooled = connectionProvider->Connection();
        auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
        pqxx::work tx(*(connection->connection));

        // Generate unique tournament name using timestamp and counter
        static int counter = 0;
        std::string uniqueName = "Test Tournament " + std::to_string(std::time(nullptr)) + "-" + std::to_string(++counter);

        nlohmann::json tournamentDoc = {
            {"name", uniqueName},
            {"status", "active"},
            {"format", {
                {"type", "round-robin"},
                {"numberOfGroups", 4},
                {"maxTeamsPerGroup", 4}
            }}
        };

        pqxx::result result = tx.exec_params(
            "INSERT INTO tournaments (document) VALUES ($1) RETURNING id",
            tournamentDoc.dump()
        );
        tx.commit();

        return result[0]["id"].c_str();
    }

    domain::Match CreateTestMatch(const std::string& tournamentId, const std::optional<std::string>& groupId = std::nullopt) {
        domain::Match match;
        match.TournamentId() = tournamentId;
        match.GroupId() = groupId;
        match.Home() = domain::MatchTeam("team1", "Team One");
        match.Visitor() = domain::MatchTeam("team2", "Team Two");
        match.Round() = "regular";
        match.Status() = "pending";
        return match;
    }

    std::vector<std::string> testMatchIds;
};

TEST_F(MatchRepositoryTest, Create_ValidMatch_ReturnsId) {
    // Arrange
    std::string tournamentId = CreateTestTournament();
    auto match = CreateTestMatch(tournamentId, std::nullopt);

    // Act
    std::string matchId = repository->Create(match);
    testMatchIds.push_back(matchId);

    // Assert
    ASSERT_FALSE(matchId.empty());
    EXPECT_EQ(36, matchId.length()); // UUID length with dashes
}

TEST_F(MatchRepositoryTest, ReadById_ExistingMatch_ReturnsMatch) {
    // Arrange
    std::string tournamentId = CreateTestTournament();
    auto match = CreateTestMatch(tournamentId, "group-1");
    std::string matchId = repository->Create(match);
    testMatchIds.push_back(matchId);

    // Act
    auto retrievedMatch = repository->ReadById(matchId);

    // Assert
    ASSERT_NE(nullptr, retrievedMatch);
    EXPECT_EQ(matchId, retrievedMatch->Id());
    EXPECT_EQ(tournamentId, retrievedMatch->TournamentId());
    EXPECT_EQ("group-1", retrievedMatch->GroupId().value());
    EXPECT_EQ("team1", retrievedMatch->Home().id);
    EXPECT_EQ("Team One", retrievedMatch->Home().name);
    EXPECT_EQ("team2", retrievedMatch->Visitor().id);
    EXPECT_EQ("Team Two", retrievedMatch->Visitor().name);
    EXPECT_EQ("regular", retrievedMatch->Round());
    EXPECT_EQ("pending", retrievedMatch->Status());
}

TEST_F(MatchRepositoryTest, ReadById_NonExistentMatch_ReturnsNull) {
    // Act
    auto match = repository->ReadById("00000000-0000-0000-0000-000000000000");

    // Assert
    EXPECT_EQ(nullptr, match);
}

TEST_F(MatchRepositoryTest, Update_ExistingMatch_UpdatesSuccessfully) {
    // Arrange
    std::string tournamentId = CreateTestTournament();
    auto match = CreateTestMatch(tournamentId, "group-1");
    std::string matchId = repository->Create(match);
    testMatchIds.push_back(matchId);

    auto retrievedMatch = repository->ReadById(matchId);
    retrievedMatch->SetScore(domain::Score(2, 1));

    // Act
    std::string updateResult = repository->Update(*retrievedMatch);

    // Assert
    EXPECT_EQ(matchId, updateResult);

    auto updatedMatch = repository->ReadById(matchId);
    ASSERT_TRUE(updatedMatch->GetScore().has_value());
    EXPECT_EQ(2, updatedMatch->GetScore()->home);
    EXPECT_EQ(1, updatedMatch->GetScore()->visitor);
    EXPECT_EQ("played", updatedMatch->Status());
}

TEST_F(MatchRepositoryTest, FindByTournamentId_MultipleMatches_ReturnsAll) {
    // Arrange
    std::string tournamentId = CreateTestTournament();
    testMatchIds.push_back(repository->Create(CreateTestMatch(tournamentId, "group-1")));
    testMatchIds.push_back(repository->Create(CreateTestMatch(tournamentId, "group-1")));
    testMatchIds.push_back(repository->Create(CreateTestMatch(tournamentId, "group-2")));

    // Act
    auto matches = repository->FindByTournamentId(tournamentId);

    // Assert
    EXPECT_GE(matches.size(), 3);
    for (const auto& match : matches) {
        EXPECT_EQ(tournamentId, match->TournamentId());
    }
}

TEST_F(MatchRepositoryTest, FindByTournamentIdAndStatus_FiltersPending_ReturnsOnlyPending) {
    // Arrange
    std::string tournamentId = CreateTestTournament();
    auto match1 = CreateTestMatch(tournamentId, "group-1");
    std::string id1 = repository->Create(match1);
    testMatchIds.push_back(id1);

    auto match2 = CreateTestMatch(tournamentId, "group-1");
    std::string id2 = repository->Create(match2);
    testMatchIds.push_back(id2);

    // Mark one as played
    auto playedMatch = repository->ReadById(id2);
    playedMatch->SetScore(domain::Score(1, 0));
    repository->Update(*playedMatch);

    // Act
    auto pendingMatches = repository->FindByTournamentIdAndStatus(tournamentId, "pending");

    // Assert
    EXPECT_GE(pendingMatches.size(), 1);
    for (const auto& match : pendingMatches) {
        EXPECT_EQ("pending", match->Status());
    }
}

TEST_F(MatchRepositoryTest, FindByTournamentIdAndStatus_FiltersPlayed_ReturnsOnlyPlayed) {
    // Arrange
    std::string tournamentId = CreateTestTournament();
    auto match1 = CreateTestMatch(tournamentId, "group-1");
    std::string id1 = repository->Create(match1);
    testMatchIds.push_back(id1);

    auto retrieved = repository->ReadById(id1);
    retrieved->SetScore(domain::Score(3, 2));
    repository->Update(*retrieved);

    // Act
    auto playedMatches = repository->FindByTournamentIdAndStatus(tournamentId, "played");

    // Assert
    EXPECT_GE(playedMatches.size(), 1);
    for (const auto& match : playedMatches) {
        EXPECT_EQ("played", match->Status());
    }
}

TEST_F(MatchRepositoryTest, FindByGroupId_MultipleMatchesInGroup_ReturnsAll) {
    // Arrange
    std::string tournamentId = CreateTestTournament();
    std::string groupId = "group-find-test";
    testMatchIds.push_back(repository->Create(CreateTestMatch(tournamentId, groupId)));
    testMatchIds.push_back(repository->Create(CreateTestMatch(tournamentId, groupId)));

    // Act
    auto matches = repository->FindByGroupId(groupId);

    // Assert
    EXPECT_GE(matches.size(), 2);
    for (const auto& match : matches) {
        EXPECT_EQ(groupId, match->GroupId().value());
    }
}

TEST_F(MatchRepositoryTest, ExistsByGroupId_MatchesExist_ReturnsTrue) {
    // Arrange
    std::string tournamentId = CreateTestTournament();
    std::string groupId = "group-exists-test";
    testMatchIds.push_back(repository->Create(CreateTestMatch(tournamentId, groupId)));

    // Act
    bool exists = repository->ExistsByGroupId(groupId);

    // Assert
    EXPECT_TRUE(exists);
}

TEST_F(MatchRepositoryTest, ExistsByGroupId_NoMatches_ReturnsFalse) {
    // Act
    bool exists = repository->ExistsByGroupId("non-existent-group");

    // Assert
    EXPECT_FALSE(exists);
}

TEST_F(MatchRepositoryTest, FindByTournamentIdAndRound_FiltersRegular_ReturnsOnlyRegular) {
    // Arrange
    std::string tournamentId = CreateTestTournament();
    auto regularMatch = CreateTestMatch(tournamentId, "group-1");
    regularMatch.Round() = "regular";
    testMatchIds.push_back(repository->Create(regularMatch));

    auto playoffMatch = CreateTestMatch(tournamentId, "group-2");
    playoffMatch.GroupId() = std::nullopt;  // Clear groupId for playoff match
    playoffMatch.Round() = "quarterfinals";
    testMatchIds.push_back(repository->Create(playoffMatch));

    // Act
    auto regularMatches = repository->FindByTournamentIdAndRound(tournamentId, "regular");

    // Assert
    EXPECT_GE(regularMatches.size(), 1);
    for (const auto& match : regularMatches) {
        EXPECT_EQ("regular", match->Round());
    }
}
