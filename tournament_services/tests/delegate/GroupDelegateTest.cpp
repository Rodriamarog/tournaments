#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "domain/Group.hpp"
#include "domain/Tournament.hpp"
#include "domain/Team.hpp"
#include "persistence/repository/GroupRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"
#include "persistence/repository/TeamRepository.hpp"
#include "delegate/GroupDelegate.hpp"

class GroupRepositoryMock : public GroupRepository {
public:
    explicit GroupRepositoryMock() : GroupRepository(nullptr) {}

    MOCK_METHOD(std::shared_ptr<domain::Group>, ReadById, (std::string id), (override));
    MOCK_METHOD(std::string, Create, (const domain::Group& entity), (override));
    MOCK_METHOD(std::string, Update, (const domain::Group& entity), (override));
    MOCK_METHOD(void, Delete, (std::string id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Group>>, ReadAll, (), (override));
    MOCK_METHOD(std::shared_ptr<domain::Group>, FindByTournamentIdAndGroupId, (const std::string_view&, const std::string_view&), (override));
    MOCK_METHOD(std::shared_ptr<domain::Group>, FindByTournamentIdAndTeamId, (const std::string_view&, const std::string_view&), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Group>>, FindByTournamentId, (const std::string_view&), (override));
    MOCK_METHOD(void, UpdateGroupAddTeam, (const std::string_view&, const std::shared_ptr<domain::Team>&), (override));
    MOCK_METHOD(bool, ExistsByName, (const std::string_view&, const std::string&), (override));
};

class TournamentRepositoryMock : public TournamentRepository {
public:
    explicit TournamentRepositoryMock() : TournamentRepository(nullptr) {}

    MOCK_METHOD(std::shared_ptr<domain::Tournament>, ReadById, (std::string id), (override));
    MOCK_METHOD(std::string, Create, (const domain::Tournament& entity), (override));
    MOCK_METHOD(std::string, Update, (const domain::Tournament& entity), (override));
    MOCK_METHOD(void, Delete, (std::string id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Tournament>>, ReadAll, (), (override));
    MOCK_METHOD(bool, ExistsByName, (const std::string& name), (override));
};

class TeamRepositoryMock : public TeamRepository {
public:
    explicit TeamRepositoryMock() : TeamRepository(nullptr) {}

    MOCK_METHOD(std::shared_ptr<domain::Team>, ReadById, (std::string_view id), (override));
    MOCK_METHOD(std::string_view, Create, (const domain::Team& entity), (override));
    MOCK_METHOD(std::string_view, Update, (const domain::Team& entity), (override));
    MOCK_METHOD(void, Delete, (std::string_view id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Team>>, ReadAll, (), (override));
    MOCK_METHOD(bool, ExistsByName, (const std::string& name), (override));
};

class GroupDelegateTest : public ::testing::Test {
protected:
    std::shared_ptr<TournamentRepositoryMock> tournamentRepositoryMock;
    std::shared_ptr<GroupRepositoryMock> groupRepositoryMock;
    std::shared_ptr<TeamRepositoryMock> teamRepositoryMock;
    std::shared_ptr<GroupDelegate> groupDelegate;

    void SetUp() override {
        tournamentRepositoryMock = std::make_shared<TournamentRepositoryMock>();
        groupRepositoryMock = std::make_shared<GroupRepositoryMock>();
        teamRepositoryMock = std::make_shared<TeamRepositoryMock>();
        groupDelegate = std::make_shared<GroupDelegate>(
            tournamentRepositoryMock,
            groupRepositoryMock,
            teamRepositoryMock
        );
    }

    void TearDown() override {
    }
};

// Test 1: Create group valid -> returns ID
TEST_F(GroupDelegateTest, CreateGroup_Valid_ReturnsID) {
    auto tournament = std::make_shared<domain::Tournament>();
    tournament->Id() = "tournament-id";

    domain::Group group;
    group.Name() = "New Group";

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(testing::Eq("tournament-id")))
        .WillOnce(testing::Return(tournament));

    EXPECT_CALL(*groupRepositoryMock, ExistsByName(testing::Eq("tournament-id"), testing::Eq("New Group")))
        .WillOnce(testing::Return(false));

    EXPECT_CALL(*groupRepositoryMock, Create(::testing::_))
        .WillOnce(testing::Return("generated-group-id"));

    auto result = groupDelegate->CreateGroup("tournament-id", group);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ("generated-group-id", result.value());
}

// Test 2: Create group duplicate -> returns error
TEST_F(GroupDelegateTest, CreateGroup_Duplicate_ReturnsError) {
    auto tournament = std::make_shared<domain::Tournament>();
    tournament->Id() = "tournament-id";

    domain::Group group;
    group.Name() = "Duplicate Group";

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(testing::Eq("tournament-id")))
        .WillOnce(testing::Return(tournament));

    EXPECT_CALL(*groupRepositoryMock, ExistsByName(testing::Eq("tournament-id"), testing::Eq("Duplicate Group")))
        .WillOnce(testing::Return(true));

    auto result = groupDelegate->CreateGroup("tournament-id", group);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ("Group with this name already exists in this tournament", result.error());
}

// Test 3: Get by ID found -> returns object
TEST_F(GroupDelegateTest, GetGroupById_Found_ReturnsObject) {
    auto tournament = std::make_shared<domain::Tournament>();
    tournament->Id() = "tournament-id";

    auto expectedGroup = std::make_shared<domain::Group>();
    expectedGroup->Id() = "group-id";
    expectedGroup->Name() = "Existing Group";

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(testing::Eq("tournament-id")))
        .WillOnce(testing::Return(tournament));

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId(testing::Eq("tournament-id"), testing::Eq("group-id")))
        .WillOnce(testing::Return(expectedGroup));

    auto result = groupDelegate->GetGroup("tournament-id", "group-id");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ("group-id", result.value().Id());
    EXPECT_EQ("Existing Group", result.value().Name());
}

// Test 4: Get by ID not found -> returns error
TEST_F(GroupDelegateTest, GetGroupById_NotFound_ReturnsError) {
    auto tournament = std::make_shared<domain::Tournament>();
    tournament->Id() = "tournament-id";

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(testing::Eq("tournament-id")))
        .WillOnce(testing::Return(tournament));

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId(testing::Eq("tournament-id"), testing::Eq("non-existent-id")))
        .WillOnce(testing::Return(nullptr));

    auto result = groupDelegate->GetGroup("tournament-id", "non-existent-id");

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ("Group not found", result.error());
}

// Test 5: Get all groups with data -> returns list
TEST_F(GroupDelegateTest, GetAllGroups_WithData_ReturnsList) {
    auto tournament = std::make_shared<domain::Tournament>();
    tournament->Id() = "tournament-id";

    std::vector<std::shared_ptr<domain::Group>> groups;

    auto group1 = std::make_shared<domain::Group>();
    group1->Id() = "id1";
    group1->Name() = "Group A";

    auto group2 = std::make_shared<domain::Group>();
    group2->Id() = "id2";
    group2->Name() = "Group B";

    groups.push_back(group1);
    groups.push_back(group2);

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(testing::Eq("tournament-id")))
        .WillOnce(testing::Return(tournament));

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentId(testing::Eq("tournament-id")))
        .WillOnce(testing::Return(groups));

    auto result = groupDelegate->GetGroups("tournament-id");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(2, result.value().size());
    EXPECT_EQ("Group A", result.value()[0].Name());
    EXPECT_EQ("Group B", result.value()[1].Name());
}

// Test 6: Get all groups empty -> returns empty list
TEST_F(GroupDelegateTest, GetAllGroups_Empty_ReturnsEmptyList) {
    auto tournament = std::make_shared<domain::Tournament>();
    tournament->Id() = "tournament-id";

    std::vector<std::shared_ptr<domain::Group>> groups;

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(testing::Eq("tournament-id")))
        .WillOnce(testing::Return(tournament));

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentId(testing::Eq("tournament-id")))
        .WillOnce(testing::Return(groups));

    auto result = groupDelegate->GetGroups("tournament-id");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(0, result.value().size());
}

// Test 7: Update group success -> returns success
TEST_F(GroupDelegateTest, UpdateGroup_Success_ReturnsSuccess) {
    auto tournament = std::make_shared<domain::Tournament>();
    tournament->Id() = "tournament-id";

    domain::Group group;
    group.Id() = "group-id";
    group.Name() = "Updated Group";

    auto existingGroup = std::make_shared<domain::Group>();
    existingGroup->Id() = "group-id";
    existingGroup->Name() = "Old Name";

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(testing::Eq("tournament-id")))
        .WillOnce(testing::Return(tournament));

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId(testing::Eq("tournament-id"), testing::Eq("group-id")))
        .WillOnce(testing::Return(existingGroup));

    EXPECT_CALL(*groupRepositoryMock, Update(::testing::_))
        .WillOnce(testing::Return("group-id"));

    auto result = groupDelegate->UpdateGroup("tournament-id", group);

    ASSERT_TRUE(result.has_value());
}

// Test 8: Update group not found -> returns error
TEST_F(GroupDelegateTest, UpdateGroup_NotFound_ReturnsError) {
    auto tournament = std::make_shared<domain::Tournament>();
    tournament->Id() = "tournament-id";

    domain::Group group;
    group.Id() = "non-existent-id";
    group.Name() = "Updated Group";

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(testing::Eq("tournament-id")))
        .WillOnce(testing::Return(tournament));

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId(testing::Eq("tournament-id"), testing::Eq("non-existent-id")))
        .WillOnce(testing::Return(nullptr));

    auto result = groupDelegate->UpdateGroup("tournament-id", group);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ("Group not found", result.error());
}

// Test 9: Create group - max groups reached -> returns error
TEST_F(GroupDelegateTest, CreateGroup_MaxGroupsReached_ReturnsError) {
    auto tournament = std::make_shared<domain::Tournament>();
    tournament->Id() = "tournament-id";
    tournament->Format().NumberOfGroups() = 1;  // Max 1 group for this tournament

    domain::Group group;
    group.Name() = "Group B";

    // Simulate that 1 group already exists (max reached)
    std::vector<std::shared_ptr<domain::Group>> existingGroups;
    auto existingGroup = std::make_shared<domain::Group>();
    existingGroup->Id() = "group-a-id";
    existingGroup->Name() = "Group A";
    existingGroups.push_back(existingGroup);

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(testing::Eq("tournament-id")))
        .WillOnce(testing::Return(tournament));

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentId(testing::Eq("tournament-id")))
        .WillOnce(testing::Return(existingGroups));

    auto result = groupDelegate->CreateGroup("tournament-id", group);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ("Maximum number of groups reached for this tournament format", result.error());
}

// Test 10: Add team to group success -> returns success
TEST_F(GroupDelegateTest, UpdateTeams_Success_ReturnsSuccess) {
    auto existingGroup = std::make_shared<domain::Group>();
    existingGroup->Id() = "group-id";
    existingGroup->Name() = "Group A";
    existingGroup->Teams().clear();  // Empty group

    auto team = std::make_shared<domain::Team>();
    team->Id = "team-id";
    team->Name = "Team A";

    std::vector<domain::Team> teamsToAdd;
    domain::Team teamToAdd;
    teamToAdd.Id = "team-id";
    teamToAdd.Name = "Team A";
    teamsToAdd.push_back(teamToAdd);

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId(testing::Eq("tournament-id"), testing::Eq("group-id")))
        .WillOnce(testing::Return(existingGroup));

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndTeamId(testing::Eq("tournament-id"), testing::Eq("team-id")))
        .WillOnce(testing::Return(nullptr));  // Team not in any group

    EXPECT_CALL(*teamRepositoryMock, ReadById(testing::Eq("team-id")))
        .WillOnce(testing::Return(team));

    EXPECT_CALL(*groupRepositoryMock, UpdateGroupAddTeam(testing::Eq("group-id"), ::testing::_))
        .Times(1);

    auto result = groupDelegate->UpdateTeams("tournament-id", "group-id", teamsToAdd);

    ASSERT_TRUE(result.has_value());
}

// Test 11: Add team to group - team doesn't exist -> returns error
TEST_F(GroupDelegateTest, UpdateTeams_TeamNotFound_ReturnsError) {
    auto existingGroup = std::make_shared<domain::Group>();
    existingGroup->Id() = "group-id";
    existingGroup->Name() = "Group A";
    existingGroup->Teams().clear();

    std::vector<domain::Team> teamsToAdd;
    domain::Team teamToAdd;
    teamToAdd.Id = "non-existent-team-id";
    teamToAdd.Name = "Team A";
    teamsToAdd.push_back(teamToAdd);

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId(testing::Eq("tournament-id"), testing::Eq("group-id")))
        .WillOnce(testing::Return(existingGroup));

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndTeamId(testing::Eq("tournament-id"), testing::Eq("non-existent-team-id")))
        .WillOnce(testing::Return(nullptr));

    EXPECT_CALL(*teamRepositoryMock, ReadById(testing::Eq("non-existent-team-id")))
        .WillOnce(testing::Return(nullptr));  // Team doesn't exist in DB

    auto result = groupDelegate->UpdateTeams("tournament-id", "group-id", teamsToAdd);

    ASSERT_FALSE(result.has_value());
    EXPECT_TRUE(result.error().find("doesn't exist") != std::string::npos);
}

// Test 12: Add team to group - group is full -> returns error
TEST_F(GroupDelegateTest, UpdateTeams_GroupFull_ReturnsError) {
    auto existingGroup = std::make_shared<domain::Group>();
    existingGroup->Id() = "group-id";
    existingGroup->Name() = "Group A";

    // Fill the group with 16 teams (max capacity)
    for (int i = 0; i < 16; i++) {
        domain::Team t;
        t.Id = "team-" + std::to_string(i);
        t.Name = "Team " + std::to_string(i);
        existingGroup->Teams().push_back(t);
    }

    std::vector<domain::Team> teamsToAdd;
    domain::Team teamToAdd;
    teamToAdd.Id = "new-team-id";
    teamToAdd.Name = "New Team";
    teamsToAdd.push_back(teamToAdd);

    EXPECT_CALL(*groupRepositoryMock, FindByTournamentIdAndGroupId(testing::Eq("tournament-id"), testing::Eq("group-id")))
        .WillOnce(testing::Return(existingGroup));

    auto result = groupDelegate->UpdateTeams("tournament-id", "group-id", teamsToAdd);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ("Group at max capacity", result.error());
}
