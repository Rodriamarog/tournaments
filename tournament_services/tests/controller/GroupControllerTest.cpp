#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>

#include "domain/Group.hpp"
#include "delegate/IGroupDelegate.hpp"
#include "controller/GroupController.hpp"

class GroupDelegateMock : public IGroupDelegate {
public:
    MOCK_METHOD((std::expected<std::string, std::string>), CreateGroup, (const std::string_view&, const domain::Group&), (override));
    MOCK_METHOD((std::expected<std::vector<domain::Group>, std::string>), GetGroups, (const std::string_view&), (override));
    MOCK_METHOD((std::expected<domain::Group, std::string>), GetGroup, (const std::string_view&, const std::string_view&), (override));
    MOCK_METHOD((std::expected<void, std::string>), UpdateGroup, (const std::string_view&, const domain::Group&), (override));
    MOCK_METHOD((std::expected<void, std::string>), RemoveGroup, (const std::string_view&, const std::string_view&), (override));
    MOCK_METHOD((std::expected<void, std::string>), UpdateTeams, (const std::string_view&, const std::string_view&, const std::vector<domain::Team>&), (override));
};

class GroupControllerTest : public ::testing::Test {
protected:
    std::shared_ptr<GroupDelegateMock> groupDelegateMock;
    std::shared_ptr<GroupController> groupController;

    void SetUp() override {
        groupDelegateMock = std::make_shared<GroupDelegateMock>();
        groupController = std::make_shared<GroupController>(groupDelegateMock);
    }

    void TearDown() override {
    }
};

// Test 1: Create group success -> HTTP 201
TEST_F(GroupControllerTest, CreateGroup_Success) {
    EXPECT_CALL(*groupDelegateMock, CreateGroup(testing::Eq("tournament-id"), ::testing::_))
        .WillOnce(testing::Return(std::expected<std::string, std::string>("new-group-id")));

    nlohmann::json groupRequestBody = {{"name", "Group A"}};
    crow::request groupRequest;
    groupRequest.body = groupRequestBody.dump();

    crow::response response = groupController->CreateGroup(groupRequest, "tournament-id");

    EXPECT_EQ(crow::CREATED, response.code);
}

// Test 2: Create group duplicate -> HTTP 409
TEST_F(GroupControllerTest, CreateGroup_Duplicate) {
    EXPECT_CALL(*groupDelegateMock, CreateGroup(testing::Eq("tournament-id"), ::testing::_))
        .WillOnce(testing::Return(std::unexpected<std::string>("Group with this name already exists in this tournament")));

    nlohmann::json groupRequestBody = {{"name", "Duplicate Group"}};
    crow::request groupRequest;
    groupRequest.body = groupRequestBody.dump();

    crow::response response = groupController->CreateGroup(groupRequest, "tournament-id");

    EXPECT_EQ(409, response.code);
}

// Test 3: Get group by ID found -> HTTP 200
TEST_F(GroupControllerTest, GetGroupById_Found) {
    domain::Group expectedGroup;
    expectedGroup.Id() = "group-id";
    expectedGroup.Name() = "Group A";

    EXPECT_CALL(*groupDelegateMock, GetGroup(testing::Eq("tournament-id"), testing::Eq("group-id")))
        .WillOnce(testing::Return(expectedGroup));

    crow::response response = groupController->GetGroup("tournament-id", "group-id");
    auto jsonResponse = crow::json::load(response.body);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ(expectedGroup.Id(), jsonResponse["id"]);
    EXPECT_EQ(expectedGroup.Name(), jsonResponse["name"]);
}

// Test 4: Get group by ID not found -> HTTP 404
TEST_F(GroupControllerTest, GetGroupById_NotFound) {
    EXPECT_CALL(*groupDelegateMock, GetGroup(testing::Eq("tournament-id"), testing::Eq("group-id")))
        .WillOnce(testing::Return(std::unexpected<std::string>("Group not found")));

    crow::response response = groupController->GetGroup("tournament-id", "group-id");

    EXPECT_EQ(crow::NOT_FOUND, response.code);
}

// Test 5: Get all groups with data -> HTTP 200
TEST_F(GroupControllerTest, GetAllGroups_WithData) {
    std::vector<domain::Group> groups;

    domain::Group group1;
    group1.Id() = "id1";
    group1.Name() = "Group A";

    domain::Group group2;
    group2.Id() = "id2";
    group2.Name() = "Group B";

    groups.push_back(group1);
    groups.push_back(group2);

    EXPECT_CALL(*groupDelegateMock, GetGroups(testing::Eq("tournament-id")))
        .WillOnce(testing::Return(groups));

    crow::response response = groupController->GetGroups("tournament-id");
    auto jsonResponse = crow::json::load(response.body);

    EXPECT_EQ(200, response.code);
    EXPECT_EQ(2, jsonResponse.size());
}

// Test 6: Get all groups empty -> HTTP 200
TEST_F(GroupControllerTest, GetAllGroups_Empty) {
    std::vector<domain::Group> groups;

    EXPECT_CALL(*groupDelegateMock, GetGroups(testing::Eq("tournament-id")))
        .WillOnce(testing::Return(groups));

    crow::response response = groupController->GetGroups("tournament-id");
    auto jsonResponse = crow::json::load(response.body);

    EXPECT_EQ(200, response.code);
    EXPECT_EQ(0, jsonResponse.size());
}

// Test 7: Update group success -> HTTP 204
TEST_F(GroupControllerTest, UpdateGroup_Success) {
    EXPECT_CALL(*groupDelegateMock, UpdateGroup(testing::Eq("tournament-id"), ::testing::_))
        .WillOnce(testing::Return(std::expected<void, std::string>()));

    nlohmann::json groupRequestBody = {{"name", "Updated Group"}};
    crow::request groupRequest;
    groupRequest.body = groupRequestBody.dump();

    crow::response response = groupController->UpdateGroup(groupRequest, "tournament-id", "group-id");

    EXPECT_EQ(204, response.code);
}

// Test 8: Update group not found -> HTTP 404
TEST_F(GroupControllerTest, UpdateGroup_NotFound) {
    EXPECT_CALL(*groupDelegateMock, UpdateGroup(testing::Eq("tournament-id"), ::testing::_))
        .WillOnce(testing::Return(std::unexpected<std::string>("Group not found")));

    nlohmann::json groupRequestBody = {{"name", "Updated Group"}};
    crow::request groupRequest;
    groupRequest.body = groupRequestBody.dump();

    crow::response response = groupController->UpdateGroup(groupRequest, "tournament-id", "group-id");

    EXPECT_EQ(crow::NOT_FOUND, response.code);
}

// Test 9: Add team to group success -> HTTP 204
TEST_F(GroupControllerTest, AddTeamToGroup_Success) {
    EXPECT_CALL(*groupDelegateMock, UpdateTeams(testing::Eq("tournament-id"), testing::Eq("group-id"), ::testing::_))
        .WillOnce(testing::Return(std::expected<void, std::string>()));

    nlohmann::json teamRequestBody = {{"id", "team-id"}, {"name", "Team A"}};
    crow::request teamRequest;
    teamRequest.body = teamRequestBody.dump();

    crow::response response = groupController->AddTeamToGroup(teamRequest, "tournament-id", "group-id");

    EXPECT_EQ(204, response.code);
}

// Test 10: Add team to group - team doesn't exist -> HTTP 422
TEST_F(GroupControllerTest, AddTeamToGroup_TeamNotFound) {
    EXPECT_CALL(*groupDelegateMock, UpdateTeams(testing::Eq("tournament-id"), testing::Eq("group-id"), ::testing::_))
        .WillOnce(testing::Return(std::unexpected<std::string>("Team team-id doesn't exist")));

    nlohmann::json teamRequestBody = {{"id", "team-id"}, {"name", "Team A"}};
    crow::request teamRequest;
    teamRequest.body = teamRequestBody.dump();

    crow::response response = groupController->AddTeamToGroup(teamRequest, "tournament-id", "group-id");

    EXPECT_EQ(422, response.code);
}

// Test 11: Add team to group - group is full -> HTTP 422
TEST_F(GroupControllerTest, AddTeamToGroup_GroupFull) {
    EXPECT_CALL(*groupDelegateMock, UpdateTeams(testing::Eq("tournament-id"), testing::Eq("group-id"), ::testing::_))
        .WillOnce(testing::Return(std::unexpected<std::string>("Group at max capacity")));

    nlohmann::json teamRequestBody = {{"id", "team-id"}, {"name", "Team A"}};
    crow::request teamRequest;
    teamRequest.body = teamRequestBody.dump();

    crow::response response = groupController->AddTeamToGroup(teamRequest, "tournament-id", "group-id");

    EXPECT_EQ(422, response.code);
}
