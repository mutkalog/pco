#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include <fstream>

#include "core/stateexecutors/stateexecutor.h"
#include "core/statepersistence.h"


namespace {
const fs::path testStateFile = "/tmp/teststate.json";
const fs::path badStateFile  = "/fakedir/teststate.json";
}


class StatePersistenceTest : public testing::Test
{
protected:
    fs::path persistenceFile;
    std::unique_ptr<StatePersistence> sp;
    std::unique_ptr<UpdateContext> ctx;

    void init(fs::path path)
    {
        persistenceFile = path;

        sp  = std::make_unique<StatePersistence>(persistenceFile);
        ctx = std::make_unique<UpdateContext>(nullptr, nullptr, nullptr, nullptr, nullptr, "", "");
    }

    void TearDown() override
    {
        fs::remove(persistenceFile);
    }
};


TEST_F(StatePersistenceTest, DumpInstallStateSuccess)
{
    init(testStateFile);

    auto busyResources = BusyResources{1, 0, 0};
    auto rollback      = false;
    auto writtenState  = StateExecutor::INSTALLING;

    ctx->busyResources = busyResources;
    ctx->rollback      = rollback;

    sp->dump(writtenState, *ctx);

    json data;
    std::ifstream statefile(persistenceFile);
    ASSERT_TRUE(statefile.is_open());

    statefile >> data;
    ASSERT_EQ(data["state"].get<uint32_t>(), writtenState);

    auto readCtx = data["context"];
    ASSERT_EQ(readCtx["rollback"].get<bool>(), rollback);

    uint32_t writtenBusyResources;
    std::memcpy(&writtenBusyResources, &busyResources, sizeof(busyResources));

    ASSERT_EQ(writtenBusyResources, readCtx["busyResources"].get<uint32_t>());
}


TEST_F(StatePersistenceTest, DumpInstallStateIncorrectFilenameFail)
{
    init(badStateFile);
    ASSERT_THROW(sp->dump(StateExecutor::INSTALLING, *ctx), std::system_error);
}


TEST_F(StatePersistenceTest, LoadInstallStateSuccess)
{
    init(testStateFile);

    std::unordered_map<fs::path, fs::path>
        pathMap = {{fs::path("/opt/pco/myapp/testapp"),
                    fs::path("/opt/pco/myapp/rollback/testapp")}};
    auto busyResources = BusyResources{1, 1, 0};
    auto rollback      = false;
    auto writtenState  = StateExecutor::COMMITTING;

    json stateAndContext;
    ctx->pathToRollbackPathMap = pathMap;
    ctx->busyResources         = busyResources;
    ctx->rollback              = rollback;

    stateAndContext["state"]   = writtenState;
    stateAndContext["context"] = ctx->dumpContext();

    std::ofstream statefile(persistenceFile);
    ASSERT_TRUE(statefile.is_open()) << "Cannot create test state file";

    std::string dump = stateAndContext.dump();
    statefile << dump;
    statefile.flush();
    ASSERT_TRUE(statefile.operator bool()) << "Cannot dump to test state file";

    auto result = sp->load(*ctx);
    ASSERT_TRUE(result.has_value())  << "Failed to read state";
    ASSERT_EQ(*result, writtenState) << "States are not equal";

    uint32_t w; std::memcpy(&w, &busyResources, sizeof(busyResources));
    uint32_t r; std::memcpy(&r, &ctx->busyResources, sizeof(ctx->busyResources));

    ASSERT_EQ(w,                                r) << "BusyResources are not equal";
    ASSERT_EQ(rollback,             ctx->rollback) << "Rollbacks are not equal";
    ASSERT_EQ(pathMap, ctx->pathToRollbackPathMap) << "Pathmaps are not equal";
}


TEST_F(StatePersistenceTest, LoadInstallFileNotExistsSuccess)
{
    init(testStateFile);

    auto result = sp->load(*ctx);

    ASSERT_TRUE(!result.has_value());
}


TEST_F(StatePersistenceTest, LoadInstallFileCorruptedFail)
{
    init(testStateFile);

    auto writtenState  = StateExecutor::PREPARING;

    json stateAndContext;
    stateAndContext["stdfalksjf"]   = writtenState;
    stateAndContext["context"] = "j129dshfjjd,,kjh";

    std::ofstream statefile(persistenceFile);
    ASSERT_TRUE(statefile.is_open()) << "Cannot create test state file";

    std::string dump = stateAndContext.dump();
    statefile << dump;
    statefile.flush();
    ASSERT_TRUE(statefile.operator bool()) << "Cannot dump to test state file";

    ASSERT_THROW(sp->load(*ctx), nlohmann::detail::exception);
}


TEST_F(StatePersistenceTest, ClearFileSuccess)
{
    init(testStateFile);

    auto busyResources = BusyResources{1, 0, 0};
    auto rollback      = false;
    auto writtenState  = StateExecutor::VERIFYING;

    json stateAndContext;
    ctx->busyResources         = busyResources;
    ctx->rollback              = rollback;

    stateAndContext["state"]   = writtenState;
    stateAndContext["context"] = ctx->dumpContext();

    std::ofstream statefile(persistenceFile);
    ASSERT_TRUE(statefile.is_open()) << "Cannot create test state file in arrange stage";

    std::string dump = stateAndContext.dump();
    statefile << dump;
    statefile.flush();
    ASSERT_TRUE(statefile.operator bool())   << "Cannot dump to test state file in arrange stage";
    ASSERT_TRUE(fs::exists(persistenceFile)) << "Cannot create file in arrange stage";

    sp->clear();

    ASSERT_FALSE(fs::exists(persistenceFile)) << "Failed to delete persistence file";
}


TEST_F(StatePersistenceTest, ClearWhenFileDoesNotExistSuccess)
{
    init(testStateFile);

    ASSERT_FALSE(fs::exists(persistenceFile));

    ASSERT_NO_THROW(sp->clear());
}


TEST_F(StatePersistenceTest, ClearIsIdempotentSuccess)
{
    init(testStateFile);

    std::ofstream(persistenceFile) << "{}";

    ASSERT_NO_THROW(sp->clear());
    ASSERT_NO_THROW(sp->clear());
}
