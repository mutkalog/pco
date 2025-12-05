#include "database.h"
#include <iostream>

namespace {

const std::string_view RELEASES_TB_CREATION_SQL =
    "CREATE TABLE IF NOT EXISTS releases (\n"
    "id            BIGSERIAL PRIMARY KEY,\n"
    "manifest_raw  TEXT NOT NULL,\n"
    "signature_raw TEXT NOT NULL,\n"
    "version       TEXT NOT NULL,\n"
    "device_type   TEXT NOT NULL,\n"
    "platform      TEXT NOT NULL,\n"
    "arch          TEXT NOT NULL,\n"
    "file_paths    TEXT[] NOT NULL,\n"
    "update_errors INTEGER NOT NULL DEFAULT 0,\n"
    "created_at    TIMESTAMPTZ DEFAULT now(),\n"
    "active        BOOLEAN DEFAULT true,\n"
    "is_canary     BOOLEAN DEFAULT false,\n"
    "canary_percent INT DEFAULT 0\n"
    ");\n";

const std::string_view DEVICES_TB_CREATION_SQL =
    "CREATE TABLE IF NOT EXISTS devices (\n"
    "id            BIGSERIAL PRIMARY KEY,\n"
    "device_type   TEXT NOT NULL,\n"
    "platform      TEXT NOT NULL,\n"
    "arch          TEXT NOT NULL,\n"
    "created_at    TIMESTAMP WITH TIME ZONE DEFAULT now(),\n"
    "last_seen     TIMESTAMP WITH TIME ZONE\n"
    ");\n";

const std::string_view RELEASE_ASSIGNMENTS_TB_CREATION_SQL =
    "CREATE TABLE IF NOT EXISTS release_assignments (\n"
    "release_id BIGINT REFERENCES releases(id),\n"
    "device_id  BIGINT REFERENCES devices(id),\n"
    "status     TEXT DEFAULT 'pending', -- pending, success, failed\n"
    "PRIMARY KEY (release_id, device_id)\n"
    ");\n";

const std::string_view REPORTS_TB_CREATION_SQL =
    "CREATE TABLE IF NOT EXISTS  reports (\n"
    "id BIGSERIAL PRIMARY KEY,\n"
    "device_id BIGSERIAL REFERENCES devices(id) ON DELETE SET NULL,\n"
    "release_id BIGSERIAL REFERENCES releases(id),\n"
    "ts TIMESTAMP WITH TIME ZONE DEFAULT now(),\n"
    "status INT,\n"
    "body JSONB\n"
    ");\n";

const std::string_view RELEASES_TB_INDEX_SQL =
    "CREATE UNIQUE INDEX IF NOT EXISTS releases_unique_idx\n"
    "ON releases(device_type, platform, arch, version);\n";

const std::vector<std::string_view> sqls{
                                        RELEASES_TB_CREATION_SQL,
                                        DEVICES_TB_CREATION_SQL,
                                        RELEASE_ASSIGNMENTS_TB_CREATION_SQL,
                                        REPORTS_TB_CREATION_SQL,
                                        RELEASES_TB_INDEX_SQL
                                   };
}

std::unique_ptr<pqxx::connection> Database::getConnection()
{
    return std::make_unique<pqxx::connection>("dbname=pco user=postgres password=truet host=127.0.0.1 port=5432");
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

        auto pcoDBConn = getConnection();
        pqxx::work txn(*pcoDBConn);

        for (const auto& sql : sqls)
        {
            pqxx::result res = txn.exec(sql.data());
        }

        txn.commit();

        ensureTriggerExists(pcoDBConn.get());
    }
    catch (const std::exception& ex)
    {
        std::cout << "Database: something went wrong: " << ex.what()
                  << ". Aboring" << std::endl;
        throw;
    }
}

void Database::ensureTriggerExists(pqxx::connection* conn)
{
    const char* sql = R"SQL(
        CREATE OR REPLACE FUNCTION inc_update_errors_and_revert()
        RETURNS trigger AS $$
        DECLARE
            prev_id BIGINT;
        BEGIN
            IF NEW.status IS NOT NULL AND NEW.status <> 0 AND NEW.release_id IS NOT NULL THEN

                UPDATE releases
                SET update_errors = update_errors + 1,
                    active = false
                WHERE id = NEW.release_id;

                SELECT id INTO prev_id
                FROM releases
                WHERE device_type = (SELECT device_type FROM releases WHERE id = NEW.release_id)
                  AND platform    = (SELECT platform    FROM releases WHERE id = NEW.release_id)
                  AND arch        = (SELECT arch        FROM releases WHERE id = NEW.release_id)
                  AND string_to_array(version, '.')::int[] <
                      (SELECT string_to_array(version, '.')::int[] FROM releases WHERE id = NEW.release_id)
                ORDER BY string_to_array(version, '.')::int[] DESC
                LIMIT 1;

                IF prev_id IS NOT NULL THEN
                    UPDATE releases
                    SET active = true
                    WHERE id = prev_id;
                END IF;
            END IF;

            RETURN NEW;
        END;
        $$ LANGUAGE plpgsql;

        DO $$
        BEGIN
            IF NOT EXISTS (
                SELECT 1 FROM pg_trigger
                WHERE tgname = 'trg_reports_status'
            ) THEN
                CREATE TRIGGER trg_reports_status
                AFTER INSERT ON reports
                FOR EACH ROW
                EXECUTE FUNCTION inc_update_errors_and_revert();
            END IF;
        END;
        $$;
    )SQL";

    pqxx::work txn(*conn);
    txn.exec(sql);
    txn.commit();
}
