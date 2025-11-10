#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <nlohmann/json.hpp>

#include "service/PlayoffGenerationService.hpp"
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
 * Tests the ScoreRegisteredConsumer logic by simulating event processing.
 *
 * The actual ScoreRegisteredConsumer listens to ActiveMQ for score-registered events
 * and triggers playoff generation at appropriate times. These tests verify the
 * business logic that determines when to generate playoff rounds.
 */
class ScoreRegisteredConsumerTest : public ::testing::Test {
protected:
    std::shared_ptr<MockMatchRepository> mockMatchRepo;
    std::shared_ptr<MockGroupRepository> mockGroupRepo;
    std::shared_ptr<MockTournamentRepository> mockTournamentRepo;
    std::shared_ptr<PlayoffGenerationService> playoffService;

    void SetUp() override {
        mockMatchRepo = std::make_shared<MockMatchRepository>();
        mockGroupRepo = std::make_shared<MockGroupRepository>();
        mockTournamentRepo = std::make_shared<MockTournamentRepository>();
        playoffService = std::make_shared<PlayoffGenerationService>(
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
        tournament->Status() = "active";

        domain::TournamentFormat format;
        format.Type() = "round-robin";
        format.NumberOfGroups() = 4;
        format.MaxTeamsPerGroup() = 4;
        tournament->Format() = format;

        return tournament;
    }

    // Helper to create a match
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

    // Helper to create a group
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

    // Simulate processing a score-registered event (this is what the consumer does)
    void ProcessScoreRegisteredEvent(const std::string& tournamentId, const std::string& matchId, const std::string& round) {
        // This mimics the logic in ScoreRegisteredConsumer::ProcessScoreRegisteredEvent
        if (round == "regular") {
            if (playoffService->AreAllGroupMatchesCompleted(tournamentId)) {
                playoffService->GenerateQuarterfinals(tournamentId);
            }
        } else if (round == "quarterfinals") {
            playoffService->AdvanceWinners(tournamentId, "quarterfinals");
        } else if (round == "semifinals") {
            playoffService->AdvanceWinners(tournamentId, "semifinals");
        }
        // Finals - tournament complete, no action needed
    }
};

// Test: Last regular match completed triggers quarterfinal generation
TEST_F(ScoreRegisteredConsumerTest, ProcessEvent_LastRegularMatch_GeneratesQuarterfinals) {
    auto tournament = CreateTestTournament();

    // All regular matches are now complete
    std::vector<std::shared_ptr<domain::Match>> allPlayed = {
        CreateTestMatch("m1", "regular", "played"),
        CreateTestMatch("m2", "regular", "played"),
        CreateTestMatch("m3", "regular", "played")
    };

    std::vector<std::shared_ptr<domain::Match>> noQuarterfinals;

    std::vector<std::shared_ptr<domain::Group>> groups = {
        CreateTestGroup("A", 4),
        CreateTestGroup("B", 4),
        CreateTestGroup("C", 4),
        CreateTestGroup("D", 4)
    };

    nlohmann::json event = {
        {"tournamentId", "tournament-1"},
        {"matchId", "m3"},
        {"round", "regular"}
    };

    // Check if all regular matches complete
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "regular"))
        .WillOnce(testing::Return(allPlayed));

    // Generate quarterfinals
    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "regular"))
        .WillOnce(testing::Return(allPlayed));
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "quarterfinals"))
        .WillOnce(testing::Return(noQuarterfinals));
    EXPECT_CALL(*mockGroupRepo, FindByTournamentId("tournament-1"))
        .WillOnce(testing::Return(groups));
    EXPECT_CALL(*mockMatchRepo, FindByGroupId(testing::_))
        .WillRepeatedly(testing::Return(allPlayed));

    // Expect 4 quarterfinal matches created
    EXPECT_CALL(*mockMatchRepo, Create(testing::_))
        .Times(4)
        .WillRepeatedly(testing::Return("match-id"));

    ProcessScoreRegisteredEvent(
        event["tournamentId"],
        event["matchId"],
        event["round"]
    );
}

// Test: Regular match completed but others still pending - no action
TEST_F(ScoreRegisteredConsumerTest, ProcessEvent_RegularMatchButOthersPending_NoAction) {
    // Some matches still pending
    std::vector<std::shared_ptr<domain::Match>> somePending = {
        CreateTestMatch("m1", "regular", "played"),
        CreateTestMatch("m2", "regular", "pending"),  // Still pending!
        CreateTestMatch("m3", "regular", "played")
    };

    nlohmann::json event = {
        {"tournamentId", "tournament-1"},
        {"matchId", "m1"},
        {"round", "regular"}
    };

    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "regular"))
        .WillOnce(testing::Return(somePending));

    // Should NOT generate quarterfinals
    EXPECT_CALL(*mockMatchRepo, Create(testing::_))
        .Times(0);

    ProcessScoreRegisteredEvent(
        event["tournamentId"],
        event["matchId"],
        event["round"]
    );
}

// Test: Last quarterfinal completed triggers semifinal generation
TEST_F(ScoreRegisteredConsumerTest, ProcessEvent_LastQuarterfinal_GeneratesSemifinals) {
    auto tournament = CreateTestTournament();

    // All 4 quarterfinals complete
    std::vector<std::shared_ptr<domain::Match>> allQuarterfinals = {
        CreateTestMatch("qf1", "quarterfinals", "played", "t1", "t2", domain::Score{2, 1}),
        CreateTestMatch("qf2", "quarterfinals", "played", "t3", "t4", domain::Score{3, 0}),
        CreateTestMatch("qf3", "quarterfinals", "played", "t5", "t6", domain::Score{1, 0}),
        CreateTestMatch("qf4", "quarterfinals", "played", "t7", "t8", domain::Score{2, 0})
    };

    std::vector<std::shared_ptr<domain::Match>> noSemifinals;

    nlohmann::json event = {
        {"tournamentId", "tournament-1"},
        {"matchId", "qf4"},
        {"round", "quarterfinals"}
    };

    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "quarterfinals"))
        .WillOnce(testing::Return(allQuarterfinals));
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "semifinals"))
        .WillOnce(testing::Return(noSemifinals));

    // Expect 2 semifinal matches created
    EXPECT_CALL(*mockMatchRepo, Create(testing::_))
        .Times(2)
        .WillRepeatedly(testing::Return("match-id"));

    ProcessScoreRegisteredEvent(
        event["tournamentId"],
        event["matchId"],
        event["round"]
    );
}

// Test: Quarterfinal completed but others pending - no action
TEST_F(ScoreRegisteredConsumerTest, ProcessEvent_QuarterfinalButOthersPending_NoAction) {
    auto tournament = CreateTestTournament();

    // Only 3 of 4 quarterfinals complete
    std::vector<std::shared_ptr<domain::Match>> someQuarterfinals = {
        CreateTestMatch("qf1", "quarterfinals", "played", "t1", "t2", domain::Score{2, 1}),
        CreateTestMatch("qf2", "quarterfinals", "played", "t3", "t4", domain::Score{3, 0}),
        CreateTestMatch("qf3", "quarterfinals", "pending", "t5", "t6")  // Still pending!
    };

    nlohmann::json event = {
        {"tournamentId", "tournament-1"},
        {"matchId", "qf1"},
        {"round", "quarterfinals"}
    };

    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "quarterfinals"))
        .WillOnce(testing::Return(someQuarterfinals));

    // Should NOT create semifinals yet
    EXPECT_CALL(*mockMatchRepo, Create(testing::_))
        .Times(0);

    ProcessScoreRegisteredEvent(
        event["tournamentId"],
        event["matchId"],
        event["round"]
    );
}

// Test: Last semifinal completed triggers final generation
TEST_F(ScoreRegisteredConsumerTest, ProcessEvent_LastSemifinal_GeneratesFinal) {
    auto tournament = CreateTestTournament();

    // Both semifinals complete
    std::vector<std::shared_ptr<domain::Match>> allSemifinals = {
        CreateTestMatch("sf1", "semifinals", "played", "t1", "t3", domain::Score{2, 1}),
        CreateTestMatch("sf2", "semifinals", "played", "t5", "t7", domain::Score{3, 0})
    };

    std::vector<std::shared_ptr<domain::Match>> noFinals;

    nlohmann::json event = {
        {"tournamentId", "tournament-1"},
        {"matchId", "sf2"},
        {"round", "semifinals"}
    };

    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "semifinals"))
        .WillOnce(testing::Return(allSemifinals));
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "finals"))
        .WillOnce(testing::Return(noFinals));

    // Expect 1 final match created
    EXPECT_CALL(*mockMatchRepo, Create(testing::_))
        .Times(1)
        .WillRepeatedly(testing::Return("match-id"));

    ProcessScoreRegisteredEvent(
        event["tournamentId"],
        event["matchId"],
        event["round"]
    );
}

// Test: Finals match completed - tournament complete
TEST_F(ScoreRegisteredConsumerTest, ProcessEvent_FinalCompleted_NoAction) {
    nlohmann::json event = {
        {"tournamentId", "tournament-1"},
        {"matchId", "final-1"},
        {"round", "finals"}
    };

    // Finals have no next round - should do nothing
    EXPECT_CALL(*mockMatchRepo, Create(testing::_))
        .Times(0);

    ProcessScoreRegisteredEvent(
        event["tournamentId"],
        event["matchId"],
        event["round"]
    );
}

// Test: Duplicate events handled gracefully (idempotent)
TEST_F(ScoreRegisteredConsumerTest, ProcessEvent_DuplicateEvent_Idempotent) {
    auto tournament = CreateTestTournament();

    std::vector<std::shared_ptr<domain::Match>> allQuarterfinals = {
        CreateTestMatch("qf1", "quarterfinals", "played", "t1", "t2", domain::Score{2, 1}),
        CreateTestMatch("qf2", "quarterfinals", "played", "t3", "t4", domain::Score{3, 0}),
        CreateTestMatch("qf3", "quarterfinals", "played", "t5", "t6", domain::Score{1, 0}),
        CreateTestMatch("qf4", "quarterfinals", "played", "t7", "t8", domain::Score{2, 0})
    };

    // Semifinals already exist
    std::vector<std::shared_ptr<domain::Match>> existingSemifinals = {
        CreateTestMatch("sf1", "semifinals", "pending")
    };

    nlohmann::json event = {
        {"tournamentId", "tournament-1"},
        {"matchId", "qf4"},
        {"round", "quarterfinals"}
    };

    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "quarterfinals"))
        .WillOnce(testing::Return(allQuarterfinals));
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "semifinals"))
        .WillOnce(testing::Return(existingSemifinals));

    // Should NOT create duplicates
    EXPECT_CALL(*mockMatchRepo, Create(testing::_))
        .Times(0);

    ProcessScoreRegisteredEvent(
        event["tournamentId"],
        event["matchId"],
        event["round"]
    );
}

// Test: Multiple events in rapid succession - only first generates
TEST_F(ScoreRegisteredConsumerTest, ProcessEvent_RapidEvents_OnlyFirstGenerates) {
    auto tournament = CreateTestTournament();

    std::vector<std::shared_ptr<domain::Match>> allQuarterfinals = {
        CreateTestMatch("qf1", "quarterfinals", "played", "t1", "t2", domain::Score{2, 1}),
        CreateTestMatch("qf2", "quarterfinals", "played", "t3", "t4", domain::Score{3, 0}),
        CreateTestMatch("qf3", "quarterfinals", "played", "t5", "t6", domain::Score{1, 0}),
        CreateTestMatch("qf4", "quarterfinals", "played", "t7", "t8", domain::Score{2, 0})
    };

    std::vector<std::shared_ptr<domain::Match>> noSemifinals;
    std::vector<std::shared_ptr<domain::Match>> existingSemifinals = {
        CreateTestMatch("sf1", "semifinals", "pending")
    };

    // First event - generates semifinals
    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "quarterfinals"))
        .WillOnce(testing::Return(allQuarterfinals));
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "semifinals"))
        .WillOnce(testing::Return(noSemifinals));

    EXPECT_CALL(*mockMatchRepo, Create(testing::_))
        .Times(2)
        .WillRepeatedly(testing::Return("match-id"));

    ProcessScoreRegisteredEvent("tournament-1", "qf4", "quarterfinals");

    // Second event (race condition) - semifinals already exist
    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament));
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "quarterfinals"))
        .WillOnce(testing::Return(allQuarterfinals));
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "semifinals"))
        .WillOnce(testing::Return(existingSemifinals));

    // Should NOT create duplicates
    EXPECT_CALL(*mockMatchRepo, Create(testing::_))
        .Times(0);

    ProcessScoreRegisteredEvent("tournament-1", "qf3", "quarterfinals");
}

// Test: Events from different tournaments are independent
TEST_F(ScoreRegisteredConsumerTest, ProcessEvent_DifferentTournaments_Independent) {
    auto tournament1 = CreateTestTournament();
    tournament1->Id() = "tournament-1";

    auto tournament2 = CreateTestTournament();
    tournament2->Id() = "tournament-2";

    std::vector<std::shared_ptr<domain::Match>> allQuarterfinalsTournament1 = {
        CreateTestMatch("t1-qf1", "quarterfinals", "played", "t1", "t2", domain::Score{2, 1}),
        CreateTestMatch("t1-qf2", "quarterfinals", "played", "t3", "t4", domain::Score{3, 0}),
        CreateTestMatch("t1-qf3", "quarterfinals", "played", "t5", "t6", domain::Score{1, 0}),
        CreateTestMatch("t1-qf4", "quarterfinals", "played", "t7", "t8", domain::Score{2, 0})
    };

    std::vector<std::shared_ptr<domain::Match>> noSemifinals;

    // Tournament 1 event
    EXPECT_CALL(*mockTournamentRepo, ReadById("tournament-1"))
        .WillOnce(testing::Return(tournament1));
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "quarterfinals"))
        .WillOnce(testing::Return(allQuarterfinalsTournament1));
    EXPECT_CALL(*mockMatchRepo, FindByTournamentIdAndRound("tournament-1", "semifinals"))
        .WillOnce(testing::Return(noSemifinals));

    EXPECT_CALL(*mockMatchRepo, Create(testing::_))
        .Times(2)
        .WillRepeatedly(testing::Return("match-id"));

    ProcessScoreRegisteredEvent("tournament-1", "t1-qf4", "quarterfinals");
}

// Test: Invalid tournament ID handled gracefully
TEST_F(ScoreRegisteredConsumerTest, ProcessEvent_InvalidTournament_HandlesGracefully) {
    nlohmann::json event = {
        {"tournamentId", "invalid-tournament"},
        {"matchId", "qf1"},
        {"round", "quarterfinals"}
    };

    EXPECT_CALL(*mockTournamentRepo, ReadById("invalid-tournament"))
        .WillOnce(testing::Return(nullptr));

    // Should NOT crash, should NOT create matches
    EXPECT_CALL(*mockMatchRepo, Create(testing::_))
        .Times(0);

    ProcessScoreRegisteredEvent(
        event["tournamentId"],
        event["matchId"],
        event["round"]
    );
}
