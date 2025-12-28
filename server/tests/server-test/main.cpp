#include <gtest/gtest.h>
#include <pqxx/pqxx>


int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    auto rc = RUN_ALL_TESTS();

    pqxx::connection c("dbname=postgres user=postgres host=127.0.0.1 port=5433");
    pqxx::nontransaction ntx(c);
    ntx.exec("DROP DATABASE IF EXISTS pco_test");

    return rc;
}
