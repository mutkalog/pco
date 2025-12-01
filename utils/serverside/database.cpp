#include "database.h"
#include <iostream>

namespace {

const std::string RELEASES_TB_CREATION_SQL = "CREATE TABLE IF NOT EXISTS releases (\n"
                                                            "id            UUID PRIMARY KEY DEFAULT gen_random_uuid(),\n"
                                                            "manifest_raw  TEXT NOT NULL,\n"
                                                            "signature_raw TEXT NOT NULL,\n"
                                                            "version       TEXT NOT NULL,\n"
                                                            "device_type   TEXT NOT NULL,\n"
                                                            "platform      TEXT NOT NULL,\n"
                                                            "arch          TEXT NOT NULL,\n"
                                                            "file_paths    TEXT[] NOT NULL,\n"
                                                            "created_at    TIMESTAMPTZ DEFAULT now(),\n"
                                                            "active        BOOLEAN DEFAULT true\n"
                                                            ");\n";

const std::vector<std::string> sqls{RELEASES_TB_CREATION_SQL};
}

Database::Database()
{
    try
    {
        {
            pqxx::connection sysBaseConn("dbname=postgres user=postgres password=truet host=127.0.0.1 port=5432");

            pqxx::work txn(sysBaseConn);
            pqxx::result res = txn.exec("SELECT 1 FROM pg_database WHERE datname='pco'");
            if (res.empty())
            {
                txn.exec("CREATE DATABASE pco");
                std::cout << "Database: PostgreSQL database created" << std::endl;
            }

            txn.commit();
            sysBaseConn.close();
        }


        conn_ = std::make_shared<pqxx::connection>("dbname=pco user=postgres password=truet host=127.0.0.1 port=5432");
        pqxx::work txn(*conn_);

        for (const auto& sql : sqls)
        {
            pqxx::result res = txn.exec(sql.c_str());
        }

        txn.commit();
    }
    catch (const std::exception& ex)
    {
        std::cout << "Database: something went wrong: " << ex.what()
                  << ". Aboring" << std::endl;
        throw;
    }
}
