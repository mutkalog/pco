#include "controllerfixturebase.h"
#include "core/http/downloadcontroller.h"
#include "tests/server-test/service/mocks/downloadservice_mock.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;

class DownloadControllerTest : public ControllerFixtureBase<DownloadController, MockDownloadService>
{
protected:
    void SetUp() override
    {
        ControllerFixtureBase::SetUp();
        controller = std::make_unique<DownloadController>(
            sc,
            std::unique_ptr<MockDownloadService>(std::move(service))
        );
    }

    std::string buildDownloadPath(
        uint32_t id,
        const std::string& type,
        const std::string& platform,
        const std::string& arch)
    {
        return "/download?id=" + std::to_string(id) +
               "&type=" + type +
               "&platform=" + platform +
               "&arch=" + arch;
    }
};


TEST_F(DownloadControllerTest, SuccessfulDownload)
{
    std::vector<uint8_t> expectedData = {0x1F, 0x8B, 0x08, 0x00, 0xDE, 0xAD};

    EXPECT_CALL(*servicePtr, getArchive(
        _,
        Eq(123u),
        Eq("sensor"),
        Eq("linux"),
        Eq("arm64")
    )).WillOnce(Return(expectedData));

    startServer(19101);

    auto res = client->Get(buildDownloadPath(123, "sensor", "linux", "arm64"));

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, httplib::OK_200);
    EXPECT_EQ(res->get_header_value("Content-Type"), "application/gzip");

    std::vector<uint8_t> actualData(res->body.begin(), res->body.end());
    EXPECT_EQ(actualData, expectedData);
}


TEST_F(DownloadControllerTest, ReturnsBadRequestWhenReleaseNotFound)
{
    EXPECT_CALL(*servicePtr, getArchive(_, _, _, _, _))
        .WillOnce(Throw(std::runtime_error("release not found")));

    startServer(19102);

    auto res = client->Get(buildDownloadPath(999, "unknown", "linux", "x86"));

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, httplib::BadRequest_400);
    EXPECT_EQ(res->body, "Release not found");
}


TEST_F(DownloadControllerTest, PassesCorrectParametersToService)
{
    EXPECT_CALL(*servicePtr, getArchive(
        _,
        Eq(42u),
        Eq("controller"),
        Eq("windows"),
        Eq("amd64")
    )).WillOnce(Return(std::vector<uint8_t>{}));

    startServer(19103);

    auto res = client->Get(buildDownloadPath(42, "controller", "windows", "amd64"));

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, httplib::OK_200);
}


TEST_F(DownloadControllerTest, ReturnsEmptyArchive)
{
    EXPECT_CALL(*servicePtr, getArchive(_, _, _, _, _))
        .WillOnce(Return(std::vector<uint8_t>{}));

    startServer(19104);

    auto res = client->Get(buildDownloadPath(1, "type", "platform", "arch"));

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, httplib::OK_200);
    EXPECT_TRUE(res->body.empty());
}

