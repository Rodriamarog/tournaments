#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <nlohmann/json.hpp>

// We can't directly test the consumer since it requires ActiveMQ infrastructure,
// but we can test the logic by creating a test harness that simulates the consumer behavior

#include "service/MatchGenerationService.hpp"
#include "domain/Match.hpp"
#include "domain/Group.hpp"
#include "domain/Tournament.hpp"

// Mock repositories for testing
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

/**
 * Tests the TeamAddedConsumer logic by simulating event processing.
 *
 * The actual TeamAddedConsumer is a thin wrapper around MatchGenerationService
 * that listens to ActiveMQ messages. These tests verify the business logic
 * that would be triggered when team-added events are received.
 */
class TeamAddedConsumerTest : public ::testing::Test {
protected:
    std::shared_ptr<MockMatchRepository> mockMatchRepo;
    std::shared_ptr<MockGroupRepository> mockGroupRepo;
    std::shared_ptr<MockTournamentRepository> mockTournamentRepo;
    std::shared_ptr<MatchGenerationService> matchGenerationService;

    void SetUp() override {
        mockMatchRepo = std::make_shared<MockMatchRepository>();
        mockGroupRepo = std::make_shared<MockGroupRepository>();
        mockTournamentRepo = std::make_shared<MockTournamentRepository>();
        matchGenerationService = std::make_shared<MatchGenerationService>(
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

    // Simulate processing a team-added event (this is what the consumer does)
    void ProcessTeamAddedEvent(const std::string& tournamentId, const std::string& groupId, const std::string& teamId) {
        // This mimics the logic in TeamAddedConsumer::ProcessTeamAddedEvent
        if (matchGenerationService->IsGroupReadyForMatches(tournamentId, groupId)) {
            auto result = matchGenerationService->GenerateRoundRobinMatches(tournamentId, groupId);
            // In the real consumer, errors would be logged
            // For testing, we can check the result if needed
        }
    }
};

// Test: When group becomes full after team added, generate matches
TEST_F(TeamAddedConsumerTest, ProcessEvent_GroupBecomesFull_GeneratesMatches) {
    auto tournament = CreateTestTournament(4);
    auto group = CreateTestGroup("group-1", 4);  // Group now has 4 teams (full)

    nlohmann::json event = {
        {"tournamentId", "tournament-1"},
        {"groupId", "group-1"},
        {"teamId", "team-4"}  // This is the 4th team being added
    };

    // Setup expectations
    EXPECT_CALL(*mockMatchRepo, ExistsByGroupId("group-1"))
        .WillOnce(testing::Return(false));
    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockGroupRepo, FindByTournamentIdAndGroupId("tournament-1", "group-1"))
        .WillOnce(testing::Return(group));

    // When group is ready, should create matches
    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockGroupRepo, FindByTournamentIdAndGroupId("tournament-1", "group-1"))
        .WillOnce(testing::Return(group));
    EXPECT_CALL(*mockMatchRepo, ExistsByGroupId("group-1"))
        .WillOnce(testing::Return(false));

    // Expect 6 matches created for 4 teams
    EXPECT_CALL(*mockMatchRepo, Create(testing::_))
        .Times(6)
        .WillRepeatedly(testing::Return("match-id"));

    // Process the event
    ProcessTeamAddedEvent(
        event["tournamentId"],
        event["groupId"],
        event["teamId"]
    );
}

// Test: When group not yet full, do nothing
TEST_F(TeamAddedConsumerTest, ProcessEvent_GroupNotFull_NoMatchesGenerated) {
    auto tournament = CreateTestTournament(4);
    auto group = CreateTestGroup("group-1", 3);  // Only 3 teams, needs 4

    nlohmann::json event = {
        {"tournamentId", "tournament-1"},
        {"groupId", "group-1"},
        {"teamId", "team-3"}
    };

    // Setup expectations for checking if group is ready
    EXPECT_CALL(*mockMatchRepo, ExistsByGroupId("group-1"))
        .WillOnce(testing::Return(false));
    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockGroupRepo, FindByTournamentIdAndGroupId("tournament-1", "group-1"))
        .WillOnce(testing::Return(group));

    // Should NOT call Create since group is not full
    EXPECT_CALL(*mockMatchRepo, Create(testing::_))
        .Times(0);

    // Process the event
    ProcessTeamAddedEvent(
        event["tournamentId"],
        event["groupId"],
        event["teamId"]
    );
}

// Test: When matches already exist, do nothing (idempotent)
TEST_F(TeamAddedConsumerTest, ProcessEvent_MatchesAlreadyExist_NoAction) {
    nlohmann::json event = {
        {"tournamentId", "tournament-1"},
        {"groupId", "group-1"},
        {"teamId", "team-4"}
    };

    // Matches already exist for this group
    EXPECT_CALL(*mockMatchRepo, ExistsByGroupId("group-1"))
        .WillOnce(testing::Return(true));

    // Should NOT check tournament or group, should NOT create matches
    EXPECT_CALL(*mockTournamentRepo, ReadById(testing::_))
        .Times(0);
    EXPECT_CALL(*mockGroupRepo, FindByTournamentIdAndGroupId(testing::_, testing::_))
        .Times(0);
    EXPECT_CALL(*mockMatchRepo, Create(testing::_))
        .Times(0);

    // Process the event
    ProcessTeamAddedEvent(
        event["tournamentId"],
        event["groupId"],
        event["teamId"]
    );
}

// Test: Multiple events for same group - only first generates matches
TEST_F(TeamAddedConsumerTest, ProcessEvent_MultipleEventsForSameGroup_OnlyFirstGeneratesMatches) {
    auto tournament = CreateTestTournament(4);
    auto group = CreateTestGroup("group-1", 4);

    // First event - group becomes full
    EXPECT_CALL(*mockMatchRepo, ExistsByGroupId("group-1"))
        .WillOnce(testing::Return(false));  // No matches yet
    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockGroupRepo, FindByTournamentIdAndGroupId("tournament-1", "group-1"))
        .WillOnce(testing::Return(group));

    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockGroupRepo, FindByTournamentIdAndGroupId("tournament-1", "group-1"))
        .WillOnce(testing::Return(group));
    EXPECT_CALL(*mockMatchRepo, ExistsByGroupId("group-1"))
        .WillOnce(testing::Return(false));

    EXPECT_CALL(*mockMatchRepo, Create(testing::_))
        .Times(6)
        .WillRepeatedly(testing::Return("match-id"));

    ProcessTeamAddedEvent("tournament-1", "group-1", "team-4");

    // Second event - matches now exist (duplicate message or race condition)
    EXPECT_CALL(*mockMatchRepo, ExistsByGroupId("group-1"))
        .WillOnce(testing::Return(true));  // Matches exist now

    // Should NOT create matches again
    EXPECT_CALL(*mockMatchRepo, Create(testing::_))
        .Times(0);

    ProcessTeamAddedEvent("tournament-1", "group-1", "team-4");
}

// Test: Events for different groups are independent
TEST_F(TeamAddedConsumerTest, ProcessEvent_DifferentGroups_IndependentGeneration) {
    auto tournament = CreateTestTournament(4);
    auto groupA = CreateTestGroup("group-a", 4);
    auto groupB = CreateTestGroup("group-b", 4);

    // Event for group A
    EXPECT_CALL(*mockMatchRepo, ExistsByGroupId("group-a"))
        .WillOnce(testing::Return(false));
    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockGroupRepo, FindByTournamentIdAndGroupId("tournament-1", "group-a"))
        .WillOnce(testing::Return(groupA));

    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockGroupRepo, FindByTournamentIdAndGroupId("tournament-1", "group-a"))
        .WillOnce(testing::Return(groupA));
    EXPECT_CALL(*mockMatchRepo, ExistsByGroupId("group-a"))
        .WillOnce(testing::Return(false));

    EXPECT_CALL(*mockMatchRepo, Create(testing::_))
        .Times(6)
        .WillRepeatedly(testing::Return("match-id"));

    ProcessTeamAddedEvent("tournament-1", "group-a", "team-4");

    // Event for group B - should also generate matches independently
    EXPECT_CALL(*mockMatchRepo, ExistsByGroupId("group-b"))
        .WillOnce(testing::Return(false));
    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockGroupRepo, FindByTournamentIdAndGroupId("tournament-1", "group-b"))
        .WillOnce(testing::Return(groupB));

    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockGroupRepo, FindByTournamentIdAndGroupId("tournament-1", "group-b"))
        .WillOnce(testing::Return(groupB));
    EXPECT_CALL(*mockMatchRepo, ExistsByGroupId("group-b"))
        .WillOnce(testing::Return(false));

    EXPECT_CALL(*mockMatchRepo, Create(testing::_))
        .Times(6)
        .WillRepeatedly(testing::Return("match-id"));

    ProcessTeamAddedEvent("tournament-1", "group-b", "team-4");
}

// Test: Tournament with different max teams per group
TEST_F(TeamAddedConsumerTest, ProcessEvent_DifferentMaxTeams_GeneratesCorrectMatchCount) {
    // Tournament requires 3 teams per group
    auto tournament = CreateTestTournament(3);
    auto group = CreateTestGroup("group-1", 3);

    EXPECT_CALL(*mockMatchRepo, ExistsByGroupId("group-1"))
        .WillOnce(testing::Return(false));
    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockGroupRepo, FindByTournamentIdAndGroupId("tournament-1", "group-1"))
        .WillOnce(testing::Return(group));

    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockGroupRepo, FindByTournamentIdAndGroupId("tournament-1", "group-1"))
        .WillOnce(testing::Return(group));
    EXPECT_CALL(*mockMatchRepo, ExistsByGroupId("group-1"))
        .WillOnce(testing::Return(false));

    // For 3 teams: (3 * 2) / 2 = 3 matches
    EXPECT_CALL(*mockMatchRepo, Create(testing::_))
        .Times(3)
        .WillRepeatedly(testing::Return("match-id"));

    ProcessTeamAddedEvent("tournament-1", "group-1", "team-3");
}

// Test: Handle invalid tournament ID gracefully
TEST_F(TeamAddedConsumerTest, ProcessEvent_InvalidTournament_HandlesGracefully) {
    nlohmann::json event = {
        {"tournamentId", "invalid-tournament"},
        {"groupId", "group-1"},
        {"teamId", "team-1"}
    };

    // Tournament doesn't exist
    EXPECT_CALL(*mockMatchRepo, ExistsByGroupId("group-1"))
        .WillOnce(testing::Return(false));
    EXPECT_CALL(*mockTournamentRepo, ReadById("invalid-tournament"))
        .WillOnce(testing::Return(nullptr));

    // Should NOT crash, should NOT create matches
    EXPECT_CALL(*mockMatchRepo, Create(testing::_))
        .Times(0);

    ProcessTeamAddedEvent(
        event["tournamentId"],
        event["groupId"],
        event["teamId"]
    );
}

// Test: Handle invalid group ID gracefully
TEST_F(TeamAddedConsumerTest, ProcessEvent_InvalidGroup_HandlesGracefully) {
    auto tournament = CreateTestTournament(4);

    nlohmann::json event = {
        {"tournamentId", "tournament-1"},
        {"groupId", "invalid-group"},
        {"teamId", "team-1"}
    };

    // Group doesn't exist
    EXPECT_CALL(*mockMatchRepo, ExistsByGroupId("invalid-group"))
        .WillOnce(testing::Return(false));
    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockGroupRepo, FindByTournamentIdAndGroupId("tournament-1", "invalid-group"))
        .WillOnce(testing::Return(nullptr));

    // Should NOT crash, should NOT create matches
    EXPECT_CALL(*mockMatchRepo, Create(testing::_))
        .Times(0);

    ProcessTeamAddedEvent(
        event["tournamentId"],
        event["groupId"],
        event["teamId"]
    );
}
