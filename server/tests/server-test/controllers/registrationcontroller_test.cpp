#include "controllerfixturebase.h"
#include "core/http/registrationcontroller.h"
#include "tests/server-test/service/mocks/registrationservice_mock.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;

class RegistrationControllerTest : public ControllerFixtureBase<RegistrationController, MockRegistrationService>
{
protected:
    void SetUp() override
    {
        ControllerFixtureBase::SetUp();
        controller = std::make_unique<RegistrationController>(
            sc,
            std::unique_ptr<MockRegistrationService>(std::move(service))
        );
    }

    httplib::Result postRegistration(
        httplib::Client& client,
        const json& body)
    {
        return client.Post("/register", body.dump(), "application/json");
    }

    json makeRegistrationBody(
        const std::string& type = "sensor",
        const std::string& platform = "linux",
        const std::string& arch = "arm64")
    {
        return {
            {"type", type},
            {"platform", platform},
            {"arch", arch}
        };
    }
};


TEST_F(RegistrationControllerTest, SuccessfulRegistration)
{
    uint64_t expectedId = 12345;

    EXPECT_CALL(*servicePtr, registerDevice(_))
        .WillOnce(Return(expectedId));

    startServer(19301);

    auto res = postRegistration(*client, makeRegistrationBody());

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, httplib::OK_200);
    EXPECT_EQ(res->get_header_value("Content-Type"), "application/json");

    json responseBody = json::parse(res->body);
    EXPECT_EQ(responseBody["status"], "registered");
    EXPECT_EQ(responseBody["id"], expectedId);
}


TEST_F(RegistrationControllerTest, ReturnsBadRequestOnInvalidParameters)
{
    EXPECT_CALL(*servicePtr, registerDevice(_))
        .WillOnce(Throw(std::runtime_error("invalid device type")));

    startServer(19302);

    auto res = postRegistration(*client, makeRegistrationBody("invalid_type"));

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, httplib::BadRequest_400);
    EXPECT_EQ(res->body, "Wrong parameters");
}


TEST_F(RegistrationControllerTest, PassesCorrectBodyToService)
{
    json expectedBody = {
        {"type", "controller"},
        {"platform", "windows"},
        {"arch", "amd64"},
        {"extra_field", "some_value"}
    };

    EXPECT_CALL(*servicePtr, registerDevice(Eq(expectedBody)))
        .WillOnce(Return(1u));

    startServer(19303);

    auto res = postRegistration(*client, expectedBody);

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, httplib::OK_200);
}


TEST_F(RegistrationControllerTest, ReturnsCorrectIdFromService)
{
    uint64_t expectedId = 9999999999u;

    EXPECT_CALL(*servicePtr, registerDevice(_))
        .WillOnce(Return(expectedId));

    startServer(19304);

    auto res = postRegistration(*client, makeRegistrationBody());

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, httplib::OK_200);

    json responseBody = json::parse(res->body);
    EXPECT_EQ(responseBody["id"], expectedId);
}
