#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include "stateexecutors/checkingstateexecutor.h"
#include "artifactmanifest.h"

using namespace testing;
class CheckingStateExecutorTest : public CheckingStateExecutor
{
public:
    // using CheckingStateExecutor::process;
    using CheckingStateExecutor::verificateRelease;
    using CheckingStateExecutor::compareVersions;
    using CheckingStateExecutor::compareDeviceType;

    CheckingStateExecutorTest() : CheckingStateExecutor(CHECKING) {}
};

TEST(CheckingStateExecutorTest, CompareVersionsSuccess)
{
    CheckingStateExecutorTest cs;

    std::string received = "2.3.45";
    std::string current  = "2.4.0";
    ASSERT_TRUE(cs.compareVersions(received, current));
}

TEST(CheckingStateExecutorTest, CompareVersionsFail)
{
    CheckingStateExecutorTest cs;

    std::string received = "1.0.0";
    std::string current  = "1.0.0";
    ASSERT_FALSE(cs.compareVersions(received, current));
}

TEST(CheckingStateExecutorTest, CompareDeviceTypeTestSuccessFullData)
{
    CheckingStateExecutorTest cs;
    ArtifactManifest received, current;

    received.release.type     = "Orange Pi 2";
    received.release.platform = "Debian 11";
    received.release.arch     = "ARMv7";

    current.release.type      = "Orange Pi 2";
    current.release.platform  = "Debian 11";
    current.release.arch      = "ARMv7";

    ASSERT_TRUE(cs.compareDeviceType(received, current));
}

TEST(CheckingStateExecutorTest, CompareDeviceTypeTestSuccessFirstUpdate)
{
    CheckingStateExecutorTest cs;
    ArtifactManifest received, current;

    received.release.type     = "Orange Pi 2";
    received.release.platform = "Debian 11";
    received.release.arch     = "ARMv7";

    ASSERT_TRUE(cs.compareDeviceType(received, current));
}

TEST(CheckingStateExecutorTest, CompareDeviceTypeTestFailArchDiffers)
{
    CheckingStateExecutorTest cs;
    ArtifactManifest received, current;

    received.release.type     = "Orange Pi 2";
    received.release.platform = "Debian 11";
    received.release.arch     = "ARMv7";

    current.release.type      = "Orange Pi 2";
    current.release.platform  = "Debian 11";
    current.release.arch      = "ARMvvv7";

    ASSERT_FALSE(cs.compareDeviceType(received, current));
}
