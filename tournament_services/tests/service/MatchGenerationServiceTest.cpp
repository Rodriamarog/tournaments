#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <vector>

#include "service/MatchGenerationService.hpp"
#include "domain/Match.hpp"
#include "domain/Group.hpp"
#include "domain/Tournament.hpp"
#include "domain/Team.hpp"

// Mock repositories
class MockMatchRepository : public MatchRepository {
public:
    MOCK_METHOD(std::string, Create, (const domain::Match& match), (override));
    MOCK_METHOD(std::shared_ptr<domain::Match>, ReadById, (const std::string& id), (override));
    MOCK_METHOD(std::string, Update, (const domain::Match& match), (override));
    MOCK_METHOD(void, Delete, (const std::string& id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Match>>, FindByTournamentId, (const std::string_view& tournamentId), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Match>>, FindByTournamentIdAndStatus, (const std::string_view& tournamentId, const std::string& status), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Match>>, FindByGroupId, (const std::string_view& groupId), (override));
    MOCK_METHOD(bool, ExistsByGroupId, (const std::string_view& groupId), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Match>>, FindByTournamentIdAndRound, (const std::string_view& tournamentId, const std::string& round), (override));
};

class MockGroupRepository : public GroupRepository {
public:
    MOCK_METHOD(std::string, Create, (const domain::Group& group), (override));
    MOCK_METHOD(std::shared_ptr<domain::Group>, FindByTournamentIdAndGroupId, (const std::string_view& tournamentId, const std::string_view& groupId), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Group>>, FindByTournamentId, (const std::string_view& tournamentId), (override));
    MOCK_METHOD(std::shared_ptr<domain::Group>, FindByTournamentIdAndTeamId, (const std::string_view& tournamentId, const std::string& teamId), (override));
    MOCK_METHOD(std::string, Update, (const domain::Group& group), (override));
    MOCK_METHOD(void, Delete, (const std::string& groupId), (override));
    MOCK_METHOD(bool, ExistsByName, (const std::string_view& tournamentId, const std::string& name), (override));
    MOCK_METHOD(void, UpdateGroupAddTeam, (const std::string_view& groupId, const std::shared_ptr<domain::Team>& team), (override));
};

class MockTournamentRepository : public TournamentRepository {
public:
    MOCK_METHOD(std::string, Create, (const domain::Tournament& tournament), (override));
    MOCK_METHOD(std::shared_ptr<domain::Tournament>, ReadById, (const std::string& id), (override));
    MOCK_METHOD(std::string, Update, (const domain::Tournament& tournament), (override));
    MOCK_METHOD(void, Delete, (const std::string& id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Tournament>>, FindAll, (), (override));
    MOCK_METHOD(bool, ExistsByName, (const std::string& name), (override));
};

class MatchGenerationServiceTest : public ::testing::Test {
protected:
    std::shared_ptr<MockMatchRepository> mockMatchRepo;
    std::shared_ptr<MockGroupRepository> mockGroupRepo;
    std::shared_ptr<MockTournamentRepository> mockTournamentRepo;
    std::shared_ptr<MatchGenerationService> service;

    void SetUp() override {
        mockMatchRepo = std::make_shared<MockMatchRepository>();
        mockGroupRepo = std::make_shared<MockGroupRepository>();
        mockTournamentRepo = std::make_shared<MockTournamentRepository>();
        service = std::make_shared<MatchGenerationService>(
            mockMatchRepo,
            mockGroupRepo,
            mockTournamentRepo
        );
    }

    // Helper to create a tournament with specific format
    std::shared_ptr<domain::Tournament> CreateTestTournament(int maxTeamsPerGroup) {
        auto tournament = std::make_shared<domain::Tournament>();
        tournament->Id() = "tournament-1";
        tournament->Name() = "Test Tournament";
        tournament->Status() = "active";

        domain::TournamentFormat format;
        format.Type() = "round-robin";
        format.NumberOfGroups() = 4;
        format.MaxTeamsPerGroup() = maxTeamsPerGroup;
        tournament->Format() = format;

        return tournament;
    }

    // Helper to create a group with specific number of teams
    std::shared_ptr<domain::Group> CreateTestGroup(const std::string& groupId, int teamCount) {
        auto group = std::make_shared<domain::Group>();
        group->Id() = groupId;
        group->TournamentId() = "tournament-1";
        group->Name() = "Group A";

        std::vector<domain::Team> teams;
        for (int i = 0; i < teamCount; i++) {
            domain::Team team;
            team.Id = "team-" + std::to_string(i + 1);
            team.Name = "Team " + std::to_string(i + 1);
            teams.push_back(team);
        }
        group->Teams() = teams;

        return group;
    }
};

// Test: IsGroupReadyForMatches - Group is full and no matches exist
TEST_F(MatchGenerationServiceTest, IsGroupReadyForMatches_GroupFull_ReturnsTrue) {
    auto tournament = CreateTestTournament(4);
    auto group = CreateTestGroup("group-1", 4);

    EXPECT_CALL(*mockMatchRepo, ExistsByGroupId("group-1"))
        .WillOnce(testing::Return(false));
    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockGroupRepo, FindByTournamentIdAndGroupId("tournament-1", "group-1"))
        .WillOnce(testing::Return(group));

    bool result = service->IsGroupReadyForMatches("tournament-1", "group-1");

    EXPECT_TRUE(result);
}

// Test: IsGroupReadyForMatches - Group not yet full
TEST_F(MatchGenerationServiceTest, IsGroupReadyForMatches_GroupNotFull_ReturnsFalse) {
    auto tournament = CreateTestTournament(4);
    auto group = CreateTestGroup("group-1", 2);  // Only 2 teams, needs 4

    EXPECT_CALL(*mockMatchRepo, ExistsByGroupId("group-1"))
        .WillOnce(testing::Return(false));
    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockGroupRepo, FindByTournamentIdAndGroupId("tournament-1", "group-1"))
        .WillOnce(testing::Return(group));

    bool result = service->IsGroupReadyForMatches("tournament-1", "group-1");

    EXPECT_FALSE(result);
}

// Test: IsGroupReadyForMatches - Matches already exist
TEST_F(MatchGenerationServiceTest, IsGroupReadyForMatches_MatchesExist_ReturnsFalse) {
    EXPECT_CALL(*mockMatchRepo, ExistsByGroupId("group-1"))
        .WillOnce(testing::Return(true));

    bool result = service->IsGroupReadyForMatches("tournament-1", "group-1");

    EXPECT_FALSE(result);
}

// Test: IsGroupReadyForMatches - Tournament doesn't exist
TEST_F(MatchGenerationServiceTest, IsGroupReadyForMatches_TournamentNotFound_ReturnsFalse) {
    EXPECT_CALL(*mockMatchRepo, ExistsByGroupId("group-1"))
        .WillOnce(testing::Return(false));
    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(nullptr));

    bool result = service->IsGroupReadyForMatches("tournament-1", "group-1");

    EXPECT_FALSE(result);
}

// Test: IsGroupReadyForMatches - Group doesn't exist
TEST_F(MatchGenerationServiceTest, IsGroupReadyForMatches_GroupNotFound_ReturnsFalse) {
    auto tournament = CreateTestTournament(4);

    EXPECT_CALL(*mockMatchRepo, ExistsByGroupId("group-1"))
        .WillOnce(testing::Return(false));
    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockGroupRepo, FindByTournamentIdAndGroupId("tournament-1", "group-1"))
        .WillOnce(testing::Return(nullptr));

    bool result = service->IsGroupReadyForMatches("tournament-1", "group-1");

    EXPECT_FALSE(result);
}

// Test: GenerateRoundRobinMatches - Success for 4 teams (generates 6 matches)
TEST_F(MatchGenerationServiceTest, GenerateRoundRobinMatches_FourTeams_Generates6Matches) {
    auto tournament = CreateTestTournament(4);
    auto group = CreateTestGroup("group-1", 4);

    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockGroupRepo, FindByTournamentIdAndGroupId("tournament-1", "group-1"))
        .WillOnce(testing::Return(group));
    EXPECT_CALL(*mockMatchRepo, ExistsByGroupId("group-1"))
        .WillOnce(testing::Return(false));

    // Expect 6 matches created: (4 * 3) / 2 = 6
    EXPECT_CALL(*mockMatchRepo, Create(testing::_))
        .Times(6)
        .WillRepeatedly(testing::Return("match-id"));

    auto result = service->GenerateRoundRobinMatches("tournament-1", "group-1");

    EXPECT_TRUE(result.has_value());
}

// Test: GenerateRoundRobinMatches - Success for 3 teams (generates 3 matches)
TEST_F(MatchGenerationServiceTest, GenerateRoundRobinMatches_ThreeTeams_Generates3Matches) {
    auto tournament = CreateTestTournament(3);
    auto group = CreateTestGroup("group-1", 3);

    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockGroupRepo, FindByTournamentIdAndGroupId("tournament-1", "group-1"))
        .WillOnce(testing::Return(group));
    EXPECT_CALL(*mockMatchRepo, ExistsByGroupId("group-1"))
        .WillOnce(testing::Return(false));

    // Expect 3 matches: (3 * 2) / 2 = 3
    EXPECT_CALL(*mockMatchRepo, Create(testing::_))
        .Times(3)
        .WillRepeatedly(testing::Return("match-id"));

    auto result = service->GenerateRoundRobinMatches("tournament-1", "group-1");

    EXPECT_TRUE(result.has_value());
}

// Test: GenerateRoundRobinMatches - Tournament doesn't exist
TEST_F(MatchGenerationServiceTest, GenerateRoundRobinMatches_TournamentNotFound_ReturnsError) {
    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(nullptr));

    auto result = service->GenerateRoundRobinMatches("tournament-1", "group-1");

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Tournament doesn't exist");
}

// Test: GenerateRoundRobinMatches - Group doesn't exist
TEST_F(MatchGenerationServiceTest, GenerateRoundRobinMatches_GroupNotFound_ReturnsError) {
    auto tournament = CreateTestTournament(4);

    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockGroupRepo, FindByTournamentIdAndGroupId("tournament-1", "group-1"))
        .WillOnce(testing::Return(nullptr));

    auto result = service->GenerateRoundRobinMatches("tournament-1", "group-1");

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Group doesn't exist");
}

// Test: GenerateRoundRobinMatches - Group belongs to different tournament
TEST_F(MatchGenerationServiceTest, GenerateRoundRobinMatches_GroupWrongTournament_ReturnsError) {
    auto tournament = CreateTestTournament(4);
    auto group = CreateTestGroup("group-1", 4);
    group->TournamentId() = "different-tournament";

    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockGroupRepo, FindByTournamentIdAndGroupId("tournament-1", "group-1"))
        .WillOnce(testing::Return(group));

    auto result = service->GenerateRoundRobinMatches("tournament-1", "group-1");

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Group doesn't belong to this tournament");
}

// Test: GenerateRoundRobinMatches - Matches already exist
TEST_F(MatchGenerationServiceTest, GenerateRoundRobinMatches_MatchesAlreadyExist_ReturnsError) {
    auto tournament = CreateTestTournament(4);
    auto group = CreateTestGroup("group-1", 4);

    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockGroupRepo, FindByTournamentIdAndGroupId("tournament-1", "group-1"))
        .WillOnce(testing::Return(group));
    EXPECT_CALL(*mockMatchRepo, ExistsByGroupId("group-1"))
        .WillOnce(testing::Return(true));

    auto result = service->GenerateRoundRobinMatches("tournament-1", "group-1");

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Matches already generated for this group");
}

// Test: GenerateRoundRobinMatches - Group not full yet
TEST_F(MatchGenerationServiceTest, GenerateRoundRobinMatches_GroupNotFull_ReturnsError) {
    auto tournament = CreateTestTournament(4);
    auto group = CreateTestGroup("group-1", 2);  // Only 2 teams, needs 4

    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockGroupRepo, FindByTournamentIdAndGroupId("tournament-1", "group-1"))
        .WillOnce(testing::Return(group));
    EXPECT_CALL(*mockMatchRepo, ExistsByGroupId("group-1"))
        .WillOnce(testing::Return(false));

    auto result = service->GenerateRoundRobinMatches("tournament-1", "group-1");

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Group is not full yet. Cannot generate matches.");
}

// Test: GenerateRoundRobinMatches - Validates match structure
TEST_F(MatchGenerationServiceTest, GenerateRoundRobinMatches_ValidatesMatchStructure) {
    auto tournament = CreateTestTournament(4);
    auto group = CreateTestGroup("group-1", 4);

    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockGroupRepo, FindByTournamentIdAndGroupId("tournament-1", "group-1"))
        .WillOnce(testing::Return(group));
    EXPECT_CALL(*mockMatchRepo, ExistsByGroupId("group-1"))
        .WillOnce(testing::Return(false));

    // Capture and validate created matches
    std::vector<domain::Match> createdMatches;
    EXPECT_CALL(*mockMatchRepo, Create(testing::_))
        .Times(6)
        .WillRepeatedly([&createdMatches](const domain::Match& match) {
            createdMatches.push_back(match);
            return "match-id";
        });

    auto result = service->GenerateRoundRobinMatches("tournament-1", "group-1");

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(createdMatches.size(), 6);

    // Validate all matches have correct properties
    for (const auto& match : createdMatches) {
        EXPECT_EQ(match.TournamentId(), "tournament-1");
        EXPECT_EQ(match.GroupId(), "group-1");
        EXPECT_EQ(match.Round(), "regular");
        EXPECT_EQ(match.Status(), "pending");
        EXPECT_FALSE(match.GetScore().has_value());  // No score yet
    }
}
