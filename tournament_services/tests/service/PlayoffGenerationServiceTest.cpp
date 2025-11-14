#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <vector>

#include "service/PlayoffGenerationService.hpp"
#include "domain/Match.hpp"
#include "domain/Group.hpp"
#include "domain/Tournament.hpp"
#include "domain/Team.hpp"

// Mock repositories (reusing from MatchGenerationServiceTest)
class MockMatchRepository : public MatchRepository {
public:
    MockMatchRepository() : MatchRepository(nullptr) {}
    MOCK_METHOD(std::string, Create, (const domain::Match& match), (override));
    MOCK_METHOD(std::shared_ptr<domain::Match>, ReadById, (std::string id), (override));
    MOCK_METHOD(std::string, Update, (const domain::Match& match), (override));
    MOCK_METHOD(void, Delete, (std::string id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Match>>, FindByTournamentId, (const std::string_view& tournamentId), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Match>>, FindByTournamentIdAndStatus, (const std::string_view& tournamentId, const std::string& status), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Match>>, FindByGroupId, (const std::string_view& groupId), (override));
    MOCK_METHOD(bool, ExistsByGroupId, (const std::string_view& groupId), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Match>>, FindByTournamentIdAndRound, (const std::string_view& tournamentId, const std::string& round), (override));
};

class MockGroupRepository : public GroupRepository {
public:
    MockGroupRepository() : GroupRepository(nullptr) {}
    MOCK_METHOD(std::string, Create, (const domain::Group& group), (override));
    MOCK_METHOD(std::shared_ptr<domain::Group>, FindByTournamentIdAndGroupId, (const std::string_view& tournamentId, const std::string_view& groupId), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Group>>, FindByTournamentId, (const std::string_view& tournamentId), (override));
    MOCK_METHOD(std::shared_ptr<domain::Group>, FindByTournamentIdAndTeamId, (const std::string_view& tournamentId, const std::string_view& teamId), (override));
    MOCK_METHOD(std::string, Update, (const domain::Group& group), (override));
    MOCK_METHOD(void, Delete, (std::string groupId), (override));
    MOCK_METHOD(bool, ExistsByName, (const std::string_view& tournamentId, const std::string& name), (override));
    MOCK_METHOD(void, UpdateGroupAddTeam, (const std::string_view& groupId, const std::shared_ptr<domain::Team>& team), (override));
};

class MockTournamentRepository : public TournamentRepository {
public:
    MockTournamentRepository() : TournamentRepository(nullptr) {}
    MOCK_METHOD(std::string, Create, (const domain::Tournament& tournament), (override));
    MOCK_METHOD(std::shared_ptr<domain::Tournament>, ReadById, (std::string id), (override));
    MOCK_METHOD(std::string, Update, (const domain::Tournament& tournament), (override));
    MOCK_METHOD(void, Delete, (std::string id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Tournament>>, ReadAll, (), (override));
    MOCK_METHOD(bool, ExistsByName, (const std::string& name), (override));
};

class PlayoffGenerationServiceTest : public ::testing::Test {
protected:
    std::shared_ptr<MockMatchRepository> mockMatchRepo;
    std::shared_ptr<MockGroupRepository> mockGroupRepo;
    std::shared_ptr<MockTournamentRepository> mockTournamentRepo;
    std::shared_ptr<PlayoffGenerationService> service;

    void SetUp() override {
        mockMatchRepo = std::make_shared<MockMatchRepository>();
        mockGroupRepo = std::make_shared<MockGroupRepository>();
        mockTournamentRepo = std::make_shared<MockTournamentRepository>();
        service = std::make_shared<PlayoffGenerationService>(
            mockMatchRepo,
            mockGroupRepo,
            mockTournamentRepo
        );
    }

    // Helper to create a test tournament
    std::shared_ptr<domain::Tournament> CreateTestTournament() {
        auto tournament = std::make_shared<domain::Tournament>();
        tournament->Id() = "tournament-1";
        tournament->Name() = "Test Tournament";

        domain::TournamentFormat format;
        format.Type() = domain::TournamentType::ROUND_ROBIN;
        format.NumberOfGroups() = 4;
        format.MaxTeamsPerGroup() = 4;
        tournament->Format() = format;

        return tournament;
    }

    // Helper to create a match with specified properties
    std::shared_ptr<domain::Match> CreateTestMatch(
        const std::string& id,
        const std::string& round,
        const std::string& status,
        const std::string& homeId = "team-1",
        const std::string& visitorId = "team-2",
        std::optional<domain::Score> score = std::nullopt
    ) {
        auto match = std::make_shared<domain::Match>();
        match->Id() = id;
        match->TournamentId() = "tournament-1";
        match->Round() = round;
        match->Status() = status;
        match->Home() = domain::MatchTeam(homeId, "Team " + homeId);
        match->Visitor() = domain::MatchTeam(visitorId, "Team " + visitorId);
        if (score.has_value()) {
            match->SetScore(score.value());
        }
        return match;
    }

    // Helper to create a group with teams
    std::shared_ptr<domain::Group> CreateTestGroup(const std::string& groupId, int teamCount) {
        auto group = std::make_shared<domain::Group>();
        group->Id() = groupId;
        group->TournamentId() = "tournament-1";
        group->Name() = "Group " + groupId;

        std::vector<domain::Team> teams;
        for (int i = 0; i < teamCount; i++) {
            domain::Team team;
            team.Id = groupId + "-team-" + std::to_string(i + 1);
            team.Name = "Team " + std::to_string(i + 1);
            teams.push_back(team);
        }
        group->Teams() = teams;

        return group;
    }
};

// Test: AreAllGroupMatchesCompleted - All matches are played
TEST_F(PlayoffGenerationServiceTest, AreAllGroupMatchesCompleted_AllPlayed_ReturnsTrue) {
    std::vector<std::shared_ptr<domain::Match>> matches = {
        CreateTestMatch("match-1", "regular", "played"),
        CreateTestMatch("match-2", "regular", "played"),
        CreateTestMatch("match-3", "regular", "played")
    };

    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "regular"))
        .WillOnce(testing::Return(matches));

    bool result = service->AreAllGroupMatchesCompleted("tournament-1");

    EXPECT_TRUE(result);
}

// Test: AreAllGroupMatchesCompleted - Some matches still pending
TEST_F(PlayoffGenerationServiceTest, AreAllGroupMatchesCompleted_SomePending_ReturnsFalse) {
    std::vector<std::shared_ptr<domain::Match>> matches = {
        CreateTestMatch("match-1", "regular", "played"),
        CreateTestMatch("match-2", "regular", "pending"),
        CreateTestMatch("match-3", "regular", "played")
    };

    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "regular"))
        .WillOnce(testing::Return(matches));

    bool result = service->AreAllGroupMatchesCompleted("tournament-1");

    EXPECT_FALSE(result);
}

// Test: AreAllGroupMatchesCompleted - No matches exist yet
TEST_F(PlayoffGenerationServiceTest, AreAllGroupMatchesCompleted_NoMatches_ReturnsFalse) {
    std::vector<std::shared_ptr<domain::Match>> matches;

    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "regular"))
        .WillOnce(testing::Return(matches));

    bool result = service->AreAllGroupMatchesCompleted("tournament-1");

    EXPECT_FALSE(result);
}

// Test: GenerateQuarterfinals - Success generates 4 matches with proper 1v8, 4v5, 2v7, 3v6 seeding
TEST_F(PlayoffGenerationServiceTest, GenerateQuarterfinals_Success_Generates4Matches) {
    auto tournament = CreateTestTournament();

    // Create 1 group with 16 teams (Round Robin format)
    auto group = std::make_shared<domain::Group>();
    group->Id() = "main-group";
    group->TournamentId() = "tournament-1";
    group->Name() = "Group A";

    // Create 16 teams with predictable IDs
    std::vector<domain::Team> teams;
    for (int i = 1; i <= 16; i++) {
        domain::Team team;
        team.Id = "team-" + std::to_string(i);
        team.Name = "Team " + std::to_string(i);
        teams.push_back(team);
    }
    group->Teams() = teams;

    std::vector<std::shared_ptr<domain::Group>> groups = {group};

    // Create matches with clear win records to establish ranking 1-16
    // Team 1: 15 wins (plays teams 2-16, wins all)
    // Team 2: 14 wins (loses to 1, beats 3-16)
    // Team 3: 13 wins (loses to 1,2, beats 4-16)
    // ... and so on
    std::vector<std::shared_ptr<domain::Match>> allMatches;
    int matchId = 1;

    // Generate round-robin matches with predictable outcomes
    for (int i = 1; i <= 16; i++) {
        for (int j = i + 1; j <= 16; j++) {
            std::string home = "team-" + std::to_string(i);
            std::string visitor = "team-" + std::to_string(j);

            // Lower numbered team always wins (team-1 beats everyone, team-2 beats 3-16, etc.)
            auto match = CreateTestMatch(
                "m" + std::to_string(matchId++),
                "regular",
                "played",
                home,
                visitor,
                domain::Score{2, 1}  // Home team wins
            );
            match->GroupId() = "main-group";
            allMatches.push_back(match);
        }
    }

    // No existing quarterfinals
    std::vector<std::shared_ptr<domain::Match>> noQuarterfinals;

    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "regular"))
        .WillOnce(testing::Return(allMatches));
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "quarterfinals"))
        .WillOnce(testing::Return(noQuarterfinals));
    EXPECT_CALL(*mockGroupRepo, FindByTournamentId("tournament-1"))
        .WillOnce(testing::Return(groups));

    // Mock FindByGroupId to return all matches for the single group
    EXPECT_CALL(*mockMatchRepo, FindByGroupId("main-group"))
        .WillOnce(testing::Return(allMatches));

    // Mock FindByTournamentIdAndGroupId to return the group
    EXPECT_CALL(*mockGroupRepo, FindByTournamentIdAndGroupId("tournament-1", "main-group"))
        .WillOnce(testing::Return(group));

    // Validate quarterfinal matchups with exact seeding: 1v8, 4v5, 2v7, 3v6
    // QF1: 1 vs 8
    EXPECT_CALL(*mockMatchRepo, Create(testing::_))
        .WillOnce(testing::Invoke([](const domain::Match& match) {
            EXPECT_EQ(match.Round(), "quarterfinals");
            EXPECT_EQ(match.Status(), "pending");
            EXPECT_EQ(match.TournamentId(), "tournament-1");
            EXPECT_EQ(match.Home().id, "team-1");
            EXPECT_EQ(match.Visitor().id, "team-8");
            return "qf1";
        }))
    // QF2: 4 vs 5
        .WillOnce(testing::Invoke([](const domain::Match& match) {
            EXPECT_EQ(match.Round(), "quarterfinals");
            EXPECT_EQ(match.Status(), "pending");
            EXPECT_EQ(match.Home().id, "team-4");
            EXPECT_EQ(match.Visitor().id, "team-5");
            return "qf2";
        }))
    // QF3: 2 vs 7
        .WillOnce(testing::Invoke([](const domain::Match& match) {
            EXPECT_EQ(match.Round(), "quarterfinals");
            EXPECT_EQ(match.Status(), "pending");
            EXPECT_EQ(match.Home().id, "team-2");
            EXPECT_EQ(match.Visitor().id, "team-7");
            return "qf3";
        }))
    // QF4: 3 vs 6
        .WillOnce(testing::Invoke([](const domain::Match& match) {
            EXPECT_EQ(match.Round(), "quarterfinals");
            EXPECT_EQ(match.Status(), "pending");
            EXPECT_EQ(match.Home().id, "team-3");
            EXPECT_EQ(match.Visitor().id, "team-6");
            return "qf4";
        }));

    auto result = service->GenerateQuarterfinals("tournament-1");

    EXPECT_TRUE(result.has_value());
}

// Test: GenerateQuarterfinals - Tournament doesn't exist
TEST_F(PlayoffGenerationServiceTest, GenerateQuarterfinals_TournamentNotFound_ReturnsError) {
    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(nullptr));

    auto result = service->GenerateQuarterfinals("tournament-1");

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Tournament doesn't exist");
}

// Test: GenerateQuarterfinals - Not all group matches completed
TEST_F(PlayoffGenerationServiceTest, GenerateQuarterfinals_MatchesNotComplete_ReturnsError) {
    auto tournament = CreateTestTournament();

    // Some matches still pending
    std::vector<std::shared_ptr<domain::Match>> regularMatches = {
        CreateTestMatch("match-1", "regular", "played"),
        CreateTestMatch("match-2", "regular", "pending")
    };

    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "regular"))
        .WillOnce(testing::Return(regularMatches));

    auto result = service->GenerateQuarterfinals("tournament-1");

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Not all group stage matches are completed");
}

// Test: GenerateQuarterfinals - Quarterfinals already exist
TEST_F(PlayoffGenerationServiceTest, GenerateQuarterfinals_AlreadyExist_ReturnsError) {
    auto tournament = CreateTestTournament();

    std::vector<std::shared_ptr<domain::Match>> regularMatches = {
        CreateTestMatch("m1", "regular", "played")
    };

    std::vector<std::shared_ptr<domain::Match>> existingQuarterfinals = {
        CreateTestMatch("qf1", "quarterfinals", "pending")
    };

    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "regular"))
        .WillOnce(testing::Return(regularMatches));
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "quarterfinals"))
        .WillOnce(testing::Return(existingQuarterfinals));

    auto result = service->GenerateQuarterfinals("tournament-1");

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Quarterfinals already generated");
}

// Test: GenerateQuarterfinals - Not enough teams for playoffs
TEST_F(PlayoffGenerationServiceTest, GenerateQuarterfinals_NotEnoughGroups_ReturnsError) {
    auto tournament = CreateTestTournament();

    std::vector<std::shared_ptr<domain::Match>> regularMatches = {
        CreateTestMatch("m1", "regular", "played")
    };

    // Create 1 group with only 6 teams (need 8 for playoffs)
    auto group = std::make_shared<domain::Group>();
    group->Id() = "main-group";
    group->TournamentId() = "tournament-1";
    group->Name() = "Group A";

    std::vector<domain::Team> teams;
    for (int i = 1; i <= 6; i++) {
        domain::Team team;
        team.Id = "team-" + std::to_string(i);
        team.Name = "Team " + std::to_string(i);
        teams.push_back(team);
    }
    group->Teams() = teams;

    std::vector<std::shared_ptr<domain::Group>> groups = {group};
    std::vector<std::shared_ptr<domain::Match>> noQuarterfinals;

    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "regular"))
        .WillOnce(testing::Return(regularMatches));
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "quarterfinals"))
        .WillOnce(testing::Return(noQuarterfinals));
    EXPECT_CALL(*mockGroupRepo, FindByTournamentId("tournament-1"))
        .WillOnce(testing::Return(groups));
    EXPECT_CALL(*mockMatchRepo, FindByGroupId("main-group"))
        .WillOnce(testing::Return(regularMatches));
    EXPECT_CALL(*mockGroupRepo, FindByTournamentIdAndGroupId("tournament-1", "main-group"))
        .WillOnce(testing::Return(group));

    auto result = service->GenerateQuarterfinals("tournament-1");

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Not enough teams for playoffs (need 8)");
}

// Test: AdvanceWinners - Quarterfinals to Semifinals
TEST_F(PlayoffGenerationServiceTest, AdvanceWinners_QuarterfinalsToSemifinals_Success) {
    auto tournament = CreateTestTournament();

    // All 4 quarterfinal matches completed
    std::vector<std::shared_ptr<domain::Match>> quarterfinals = {
        CreateTestMatch("qf1", "quarterfinals", "played", "team-1", "team-2", domain::Score{2, 1}),
        CreateTestMatch("qf2", "quarterfinals", "played", "team-3", "team-4", domain::Score{3, 0}),
        CreateTestMatch("qf3", "quarterfinals", "played", "team-5", "team-6", domain::Score{1, 0}),
        CreateTestMatch("qf4", "quarterfinals", "played", "team-7", "team-8", domain::Score{2, 0})
    };

    // No existing semifinals
    std::vector<std::shared_ptr<domain::Match>> noSemifinals;

    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "quarterfinals"))
        .WillOnce(testing::Return(quarterfinals));
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "semifinals"))
        .WillOnce(testing::Return(noSemifinals));

    // Expect 2 semifinal matches to be created
    EXPECT_CALL(*mockMatchRepo, Create(testing::_))
        .Times(2)
        .WillRepeatedly(testing::Return("match-id"));

    auto result = service->AdvanceWinners("tournament-1", "quarterfinals");

    EXPECT_TRUE(result.has_value());
}

// Test: AdvanceWinners - Semifinals to Finals
TEST_F(PlayoffGenerationServiceTest, AdvanceWinners_SemifinalsToFinals_Success) {
    auto tournament = CreateTestTournament();

    // Both semifinal matches completed
    std::vector<std::shared_ptr<domain::Match>> semifinals = {
        CreateTestMatch("sf1", "semifinals", "played", "team-1", "team-3", domain::Score{2, 1}),
        CreateTestMatch("sf2", "semifinals", "played", "team-5", "team-7", domain::Score{3, 0})
    };

    // No existing finals
    std::vector<std::shared_ptr<domain::Match>> noFinals;

    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "semifinals"))
        .WillOnce(testing::Return(semifinals));
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "finals"))
        .WillOnce(testing::Return(noFinals));

    // Expect 1 final match to be created
    EXPECT_CALL(*mockMatchRepo, Create(testing::_))
        .Times(1)
        .WillRepeatedly(testing::Return("match-id"));

    auto result = service->AdvanceWinners("tournament-1", "semifinals");

    EXPECT_TRUE(result.has_value());
}

// Test: AdvanceWinners - Tournament doesn't exist
TEST_F(PlayoffGenerationServiceTest, AdvanceWinners_TournamentNotFound_ReturnsError) {
    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(nullptr));

    auto result = service->AdvanceWinners("tournament-1", "quarterfinals");

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Tournament doesn't exist");
}

// Test: AdvanceWinners - Invalid round
TEST_F(PlayoffGenerationServiceTest, AdvanceWinners_InvalidRound_ReturnsError) {
    auto tournament = CreateTestTournament();

    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));

    auto result = service->AdvanceWinners("tournament-1", "invalid-round");

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Invalid round for advancement");
}

// Test: AdvanceWinners - Not all matches in round completed
TEST_F(PlayoffGenerationServiceTest, AdvanceWinners_NotAllMatchesComplete_ReturnsSuccess) {
    auto tournament = CreateTestTournament();

    // All 4 quarterfinals exist but only 3 are played, 1 is still pending
    std::vector<std::shared_ptr<domain::Match>> quarterfinals = {
        CreateTestMatch("qf1", "quarterfinals", "played", "team-1", "team-2", domain::Score{2, 1}),
        CreateTestMatch("qf2", "quarterfinals", "played", "team-3", "team-4", domain::Score{3, 0}),
        CreateTestMatch("qf3", "quarterfinals", "played", "team-5", "team-6", domain::Score{1, 0}),
        CreateTestMatch("qf4", "quarterfinals", "pending", "team-7", "team-8")  // Still pending
    };

    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "quarterfinals"))
        .WillOnce(testing::Return(quarterfinals));

    auto result = service->AdvanceWinners("tournament-1", "quarterfinals");

    // Should return success but not create next round yet (line 313: returns {} when not all played)
    EXPECT_TRUE(result.has_value());
}

// Test: AdvanceWinners - Next round already exists
TEST_F(PlayoffGenerationServiceTest, AdvanceWinners_NextRoundExists_ReturnsSuccess) {
    auto tournament = CreateTestTournament();

    std::vector<std::shared_ptr<domain::Match>> quarterfinals = {
        CreateTestMatch("qf1", "quarterfinals", "played", "team-1", "team-2", domain::Score{2, 1}),
        CreateTestMatch("qf2", "quarterfinals", "played", "team-3", "team-4", domain::Score{3, 0}),
        CreateTestMatch("qf3", "quarterfinals", "played", "team-5", "team-6", domain::Score{1, 0}),
        CreateTestMatch("qf4", "quarterfinals", "played", "team-7", "team-8", domain::Score{2, 0})
    };

    // Semifinals already exist
    std::vector<std::shared_ptr<domain::Match>> existingSemifinals = {
        CreateTestMatch("sf1", "semifinals", "pending")
    };

    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "quarterfinals"))
        .WillOnce(testing::Return(quarterfinals));
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "semifinals"))
        .WillOnce(testing::Return(existingSemifinals));

    auto result = service->AdvanceWinners("tournament-1", "quarterfinals");

    // Should return success without creating duplicates
    EXPECT_TRUE(result.has_value());
}
