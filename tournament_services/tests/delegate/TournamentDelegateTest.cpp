#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "domain/Tournament.hpp"
#include "persistence/repository/IRepository.hpp"
#include "persistence/repository/TournamentRepository.hpp"
#include "delegate/TournamentDelegate.hpp"

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

class TournamentDelegateTest : public ::testing::Test {
protected:
    std::shared_ptr<TournamentRepositoryMock> tournamentRepositoryMock;
    std::shared_ptr<TournamentDelegate> tournamentDelegate;

    void SetUp() override {
        tournamentRepositoryMock = std::make_shared<TournamentRepositoryMock>();
        tournamentDelegate = std::make_shared<TournamentDelegate>(tournamentRepositoryMock);
    }

    void TearDown() override {
    }
};

// Test 1: Create tournament valid -> returns ID
TEST_F(TournamentDelegateTest, CreateTournament_Valid_ReturnsID) {
    auto tournament = std::make_shared<domain::Tournament>();
    tournament->Name() = "New Tournament";

    EXPECT_CALL(*tournamentRepositoryMock, ExistsByName(testing::Eq("New Tournament")))
        .WillOnce(testing::Return(false));

    EXPECT_CALL(*tournamentRepositoryMock, Create(::testing::_))
        .WillOnce(testing::Return("generated-id"));

    auto result = tournamentDelegate->CreateTournament(tournament);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ("generated-id", result.value());
}

// Test 2: Create tournament duplicate -> returns error
TEST_F(TournamentDelegateTest, CreateTournament_Duplicate_ReturnsError) {
    auto tournament = std::make_shared<domain::Tournament>();
    tournament->Name() = "Duplicate Tournament";

    EXPECT_CALL(*tournamentRepositoryMock, ExistsByName(testing::Eq("Duplicate Tournament")))
        .WillOnce(testing::Return(true));

    auto result = tournamentDelegate->CreateTournament(tournament);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ("Tournament with this name already exists", result.error());
}

// Test 3: Get by ID found -> returns object
TEST_F(TournamentDelegateTest, GetTournamentById_Found_ReturnsObject) {
    auto expectedTournament = std::make_shared<domain::Tournament>();
    expectedTournament->Id() = "existing-id";
    expectedTournament->Name() = "Existing Tournament";

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(testing::Eq("existing-id")))
        .WillOnce(testing::Return(expectedTournament));

    auto result = tournamentDelegate->GetTournament("existing-id");

    ASSERT_NE(nullptr, result);
    EXPECT_EQ("existing-id", result->Id());
    EXPECT_EQ("Existing Tournament", result->Name());
}

// Test 4: Get by ID not found -> returns nullptr
TEST_F(TournamentDelegateTest, GetTournamentById_NotFound_ReturnsNull) {
    EXPECT_CALL(*tournamentRepositoryMock, ReadById(testing::Eq("non-existent-id")))
        .WillOnce(testing::Return(nullptr));

    auto result = tournamentDelegate->GetTournament("non-existent-id");

    EXPECT_EQ(nullptr, result);
}

// Test 5: Get all with data -> returns list
TEST_F(TournamentDelegateTest, GetAllTournaments_WithData_ReturnsList) {
    std::vector<std::shared_ptr<domain::Tournament>> tournaments;

    auto tournament1 = std::make_shared<domain::Tournament>();
    tournament1->Id() = "id1";
    tournament1->Name() = "Tournament 1";

    auto tournament2 = std::make_shared<domain::Tournament>();
    tournament2->Id() = "id2";
    tournament2->Name() = "Tournament 2";

    tournaments.push_back(tournament1);
    tournaments.push_back(tournament2);

    EXPECT_CALL(*tournamentRepositoryMock, ReadAll())
        .WillOnce(testing::Return(tournaments));

    auto result = tournamentDelegate->ReadAll();

    EXPECT_EQ(2, result.size());
    EXPECT_EQ("Tournament 1", result[0]->Name());
    EXPECT_EQ("Tournament 2", result[1]->Name());
}

// Test 6: Get all empty -> returns empty list
TEST_F(TournamentDelegateTest, GetAllTournaments_Empty_ReturnsEmptyList) {
    std::vector<std::shared_ptr<domain::Tournament>> tournaments;

    EXPECT_CALL(*tournamentRepositoryMock, ReadAll())
        .WillOnce(testing::Return(tournaments));

    auto result = tournamentDelegate->ReadAll();

    EXPECT_EQ(0, result.size());
}

// Test 7: Update tournament success -> returns success
TEST_F(TournamentDelegateTest, UpdateTournament_Success_ReturnsSuccess) {
    domain::Tournament tournament;
    tournament.Id() = "existing-id";
    tournament.Name() = "Updated Tournament";

    auto existingTournament = std::make_shared<domain::Tournament>();
    existingTournament->Id() = "existing-id";
    existingTournament->Name() = "Old Name";

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(testing::Eq("existing-id")))
        .WillOnce(testing::Return(existingTournament));

    EXPECT_CALL(*tournamentRepositoryMock, Update(::testing::_))
        .WillOnce(testing::Return("existing-id"));

    auto result = tournamentDelegate->UpdateTournament(tournament);

    ASSERT_TRUE(result.has_value());
}

// Test 8: Update tournament not found -> returns error
TEST_F(TournamentDelegateTest, UpdateTournament_NotFound_ReturnsError) {
    domain::Tournament tournament;
    tournament.Id() = "non-existent-id";
    tournament.Name() = "Updated Tournament";

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(testing::Eq("non-existent-id")))
        .WillOnce(testing::Return(nullptr));

    auto result = tournamentDelegate->UpdateTournament(tournament);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ("Tournament not found", result.error());
}
