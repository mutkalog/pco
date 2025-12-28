#include "controllerfixturebase.h"
#include "core/http/uploadcontroller.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <tests/server-test/service/mocks/uploadservice_mock.h>
#include <pqxx/pqxx>

using namespace testing;

class UploadControllerTest : public ControllerFixtureBase<UploadController, MockUploadService>
{
protected:
    void SetUp() override
    {
        ControllerFixtureBase::SetUp();
        controller = std::make_unique<UploadController>(
            sc,
            std::unique_ptr<MockUploadService>(std::move(service))
        );
    }

    httplib::Result postMultipart(
        httplib::Client& client,
        const std::string& path,
        const std::string& manifest = R"({"version":"1.0"})",
        const std::string& archive  = "binary_data",
        const httplib::Params& params = {})
    {
        httplib::UploadFormDataItems items;

        items.push_back({
            "manifest",
            manifest,
            "manifest.json",
            "application/json"
        });

        items.push_back({
            "archive",
            archive,
            "update.tar.gz",
            "application/gzip"
        });

        if (params.empty()) {
            return client.Post(path, items);
        }

        std::string full_path = path;
        if (!params.empty()) {
            full_path += "?";
            bool first = true;
            for (const auto& [k, v] : params) {
                if (!first) full_path += "&";
                full_path += k + "=" + v;
                first = false;
            }
        }

    return client.Post(full_path, items);
    }
};


TEST_F(UploadControllerTest, RejectsNonMultipartRequest)
{
    startServer(19001);

    auto res = client->Post("/upload", "plain text", "text/plain");

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, httplib::BadRequest_400);
    EXPECT_EQ(res->body, "Expected multipart/form-data");
}


TEST_F(UploadControllerTest, SuccessfulUploadWithoutCanary)
{
    EXPECT_CALL(*servicePtr, upload(
        _,
        Eq(std::nullopt),
        _,
        _,
        _
    )).Times(1);

    startServer(19002);

    auto res = postMultipart(*client, "/upload");

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, httplib::OK_200);
    EXPECT_EQ(res->body, "Success");
}


TEST_F(UploadControllerTest, SuccessfulUploadWithCanary)
{
    EXPECT_CALL(*servicePtr, upload(
        _,
        Eq(std::make_optional(25)),
        _,
        _,
        _
    )).Times(1);

    startServer(19003);

    httplib::Params params = {{"canary", "true"}, {"percentage", "25"}};
    auto res = postMultipart(*client, "/upload", R"({"version":"1.0"})", "binary_data", params);

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, httplib::OK_200);
}


TEST_F(UploadControllerTest, PassesManifestAndArchiveToService)
{
    std::string expectedManifest = R"({"name":"test-update"})";
    std::string expectedArchive = "gzip_binary_content";

    EXPECT_CALL(*servicePtr, upload(
        _,
        _,
        _,
        Eq(expectedManifest),
        Eq(expectedArchive)
    )).Times(1);

    startServer(19004);

    auto res = postMultipart(*client, "/upload", expectedManifest, expectedArchive);

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, httplib::OK_200);
}


TEST_F(UploadControllerTest, ReturnsConflictOnDuplicateEntity)
{
    EXPECT_CALL(*servicePtr, upload(
        _, _, _, _, _
    )).WillOnce(Throw(pqxx::unique_violation("duplicate key")));

    startServer(19005);

    auto res = postMultipart(*client, "/upload");

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, httplib::Conflict_409);
    EXPECT_EQ(res->body, "Such entity already exist on server");
}


TEST_F(UploadControllerTest, ReturnsInternalErrorOnDatabaseError)
{
    EXPECT_CALL(*servicePtr, upload(
        _, _, _, _, _
    )).WillOnce(Throw(pqxx::sql_error("connection failed", "INSERT")));

    startServer(19006);

    auto res = postMultipart(*client, "/upload");

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, httplib::InternalServerError_500);
    EXPECT_EQ(res->body, "Unexpected error");
}


TEST_F(UploadControllerTest, ReturnsBadRequestOnRuntimeError)
{
    EXPECT_CALL(*servicePtr, upload(
        _, _, _, _, _
    )).WillOnce(Throw(std::runtime_error("manifest validation failed")));

    startServer(19007);

    auto res = postMultipart(*client, "/upload");

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, httplib::BadRequest_400);
    EXPECT_EQ(res->body, "Expected manifest and archive");
}


TEST_F(UploadControllerTest, CanaryWithoutPercentageIsIgnored)
{
    EXPECT_CALL(*servicePtr, upload(
        _,
        Eq(std::nullopt),
        _,
        _,
        _
    )).Times(1);

    startServer(19008);

    httplib::Params params = {{"canary", "true"}};
    auto res = postMultipart(*client, "/upload", R"({})", "data", params);

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, httplib::OK_200);
}


TEST_F(UploadControllerTest, CanaryFalseIsIgnored)
{
    EXPECT_CALL(*servicePtr, upload(
        _,
        Eq(std::nullopt),
        _,
        _,
        _
    )).Times(1);

    startServer(19009);

    httplib::Params params = {{"canary", "false"}, {"percentage", "50"}};
    auto res = postMultipart(*client, "/upload", R"({})", "data", params);

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, httplib::OK_200);
}
