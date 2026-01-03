#include "controllerfixturebase.h"
#include "core/http/reportcontroller.h"
#include "tests/server-test/unit/service/mocks/reportservice_mock.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;

class ReportControllerTest : public ControllerFixtureBase<ReportController, MockReportService>
{
protected:
    void SetUp() override
    {
        ControllerFixtureBase::SetUp();
        controller = std::make_unique<ReportController>(
            sc,
            std::unique_ptr<MockReportService>(std::move(service))
        );
    }

    httplib::Result postReport(
        httplib::Client& client,
        const json& body)
    {
        return client.Post("/report", body.dump(), "application/json");
    }

    json makeReportBody(
        uint64_t id = 123,
        const std::string& type = "sensor",
        const std::string& platform = "linux",
        const std::string& arch = "arm64")
    {
        return {
            {"id", id},
            {"type", type},
            {"platform", platform},
            {"arch", arch}
        };
    }
};


TEST_F(ReportControllerTest, SuccessfulReport)
{
    EXPECT_CALL(*servicePtr, parseReport(_, _))
        .Times(1);

    startServer(19401);

    auto res = postReport(*client, makeReportBody());

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, httplib::OK_200);
}


TEST_F(ReportControllerTest, PassesCorrectBodyToService)
{
    json expectedBody = {
        {"id", 42},
        {"type", "controller"},
        {"platform", "windows"},
        {"arch", "amd64"},
        {"status", "success"},
        {"version", "1.2.3"}
    };

    EXPECT_CALL(*servicePtr, parseReport(_, Eq(expectedBody)))
        .Times(1);

    startServer(19402);

    auto res = postReport(*client, expectedBody);

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, httplib::OK_200);
}


TEST_F(ReportControllerTest, HandlesEmptyReport)
{
    json emptyReport = json::object();

    EXPECT_CALL(*servicePtr, parseReport(_, Eq(emptyReport)))
        .Times(1);

    startServer(19403);

    auto res = postReport(*client, emptyReport);

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, httplib::OK_200);
}
