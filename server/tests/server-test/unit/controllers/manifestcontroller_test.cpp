#include "controllerfixturebase.h"
#include "core/http/manifestcontrller.h"
#include "tests/server-test/unit/service/mocks/manifestservice_mock.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;

class ManifestControllerTest : public ControllerFixtureBase<ManifestController, MockManifestService>
{
protected:
    void SetUp() override
    {
        ControllerFixtureBase::SetUp();
        controller = std::make_unique<ManifestController>(
            sc,
            std::unique_ptr<MockManifestService>(std::move(service))
        );
    }

    std::string buildManifestPath(
        uint64_t id,
        const std::string& type,
        const std::string& platform,
        const std::string& arch)
    {
        return "/manifest?id=" + std::to_string(id) +
               "&type=" + type +
               "&platform=" + platform +
               "&arch=" + arch;
    }
};


TEST_F(ManifestControllerTest, SuccessfulGetManifest)
{
    json expectedManifest = {
        {"version", "2.0.1"},
        {"updates", json::array({
            {{"name", "firmware"}, {"size", 1024}}
        })}
    };

    EXPECT_CALL(*servicePtr, getManifest(
        Eq(123u),
        Eq("sensor"),
        Eq("linux"),
        Eq("arm64")
    )).WillOnce(Return(expectedManifest));

    startServer(19201);

    auto res = client->Get(buildManifestPath(123, "sensor", "linux", "arm64"));

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, httplib::OK_200);
    EXPECT_EQ(res->get_header_value("Content-Type"), "application/json");

    json actualManifest = json::parse(res->body);
    EXPECT_EQ(actualManifest, expectedManifest);
}


TEST_F(ManifestControllerTest, ReturnsBadRequestWhenReleaseNotFound)
{
    EXPECT_CALL(*servicePtr, getManifest(_, _, _, _))
        .WillOnce(Throw(std::runtime_error("release not found")));

    startServer(19202);

    auto res = client->Get(buildManifestPath(999, "unknown", "linux", "x86"));

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, httplib::BadRequest_400);
    EXPECT_EQ(res->body, "Release not found");
}


TEST_F(ManifestControllerTest, PassesCorrectParametersToService)
{
    EXPECT_CALL(*servicePtr, getManifest(
        Eq(42u),
        Eq("controller"),
        Eq("windows"),
        Eq("amd64")
    )).WillOnce(Return(json::object()));

    startServer(19203);

    auto res = client->Get(buildManifestPath(42, "controller", "windows", "amd64"));

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, httplib::OK_200);
}


TEST_F(ManifestControllerTest, ReturnsEmptyManifest)
{
    EXPECT_CALL(*servicePtr, getManifest(_, _, _, _))
        .WillOnce(Return(json::object()));

    startServer(19204);

    auto res = client->Get(buildManifestPath(1, "type", "platform", "arch"));

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, httplib::OK_200);

    json actualManifest = json::parse(res->body);
    EXPECT_TRUE(actualManifest.empty());
}
